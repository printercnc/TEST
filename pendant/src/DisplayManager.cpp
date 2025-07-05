// DisplayManager.cpp

#include "DisplayManager.h"
#include "Status.h"   // để truy cập printerStatus và EXTRUDERS
#include <stdio.h>    // cho snprintf
#include "printer_status.h"
#include "PrinterI2C.h"

// STM32 HAL I2C dùng cho master
#include "stm32f1xx_hal.h"
extern I2C_HandleTypeDef hi2c1;

#define MARLIN_I2C_ADDR 0x42 // 7-bit address

// I2C Master Gửi LỆNH sang Marlin
//------------------------------------------------------
bool sendPrinterCommand(const PrinterCommand* cmd) {
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(
        &hi2c1,
        MARLIN_I2C_ADDR << 1,               // STM32 dùng địa chỉ 8-bit
        (uint8_t*)cmd, sizeof(PrinterCommand),
        100
    );
    return (ret == HAL_OK);
}
//------------------------------------------------------
// HÀM ĐIỀU KHIỂN
//------------------------------------------------------
void homeAllAxes() {
    PrinterCommand cmd = {0};
    cmd.command_id = 0x01; // Ví dụ lệnh home; điều chỉnh theo project bạn
    memset(cmd.args, 0, sizeof(cmd.args));
    sendPrinterCommand(&cmd);
}

void jogAxis(uint8_t axis, float delta) {
    PrinterCommand cmd = {0};
    cmd.command_id = 0x02; // Ví dụ lệnh jog
    memcpy(cmd.args, &axis, 1); 
    memcpy(cmd.args + 1, &delta, 4);
    sendPrinterCommand(&cmd);
}
// VÍ DỤ Class DisplayManager (dùng U8G2)
//------------------------------------------------------
#include <U8g2lib.h>
extern U8G2_ST7920_128X64_F_SW_SPI u8g2;  // hoặc extern biến được định nghĩa bên ngoài

void DisplayManager::drawStatusScreen(const PrinterStatus* status) {
    char buf[32];
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);

    snprintf(buf, sizeof(buf), "State: %u", status->state);
    u8g2.drawStr(0, 12, buf);

    for (int i = 0; i < 3; i++) {
        snprintf(buf, sizeof(buf), "%c: %.2f", 'X'+i, status->position[i]);
        u8g2.drawStr(0, 24 + i * 12, buf);
    }
  }
    // ... draw more info (temp, feedrate...) as needed

void DisplayManager::drawOffsetsPage(const float offsets[AXIS_COUNT], const char* title) {
  
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 12, title);

  char buf[40];
  // Hiển thị 3 trục chính X,Y,Z
  snprintf(buf, sizeof(buf), "X:%7.3f Y:%7.3f Z:%7.3f",
           offsets[0], offsets[1], offsets[2]);
  u8g2.drawStr(0, 30, buf);

  // Hiển thị các trục mở rộng A,C,E
  int y = 45;
  for (int i = 3; i < AXIS_COUNT; i++) {
    snprintf(buf, sizeof(buf), "%c:%7.3f", axes[i], offsets[i]);
    u8g2.drawStr(0, y, buf);
    y += 12;
  }
  
}

void DisplayManager::drawParameterPage(int selectedParamIdx) {

  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 12, "Parameters:");

  char buf[32];
  int y = 30;
  for (int i = 0; i < PARAM_COUNT; i++) {
    if (i == selectedParamIdx) {
      u8g2.drawBox(0, y - 10, 128, 12); // highlight background
      u8g2.setDrawColor(0);              // text color inverted (black)
    } else {
      u8g2.setDrawColor(1); // normal text color (white)
    }
    snprintf(buf, sizeof(buf), "%s: %.1f", params[i].name, params[i].value);
    u8g2.drawStr(2, y, buf);
    y += 14;
  }
  u8g2.setDrawColor(1); // restore normal text color
  
}

void DisplayManager::drawMachineControlPage(int selectedMenuIndex, char selectedAxis, float currentPosition) {
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 12, "Machine Control:");

  const char* menuItems[] = {"Home All", "X", "Y", "Z", "A", "C"};
  const int menuCount = sizeof(menuItems) / sizeof(menuItems[0]);

  int y = 30;

  for (int i = 0; i < menuCount; i++) {
    if (i == selectedMenuIndex) {
      u8g2.drawBox(0, y - 10, 128, 12);
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(1);
    }
    char buf[20];
    if(i == 0) {
      snprintf(buf, sizeof(buf), "%s", menuItems[i]);
    } else {
      // Hiển thị tên trục và vị trí nếu đang chọn
      if (i == selectedMenuIndex) {
        snprintf(buf, sizeof(buf), "%s: %.3f", menuItems[i], currentPosition);
      } else {
        snprintf(buf, sizeof(buf), "%s", menuItems[i]);
      }
    }
    u8g2.drawStr(2, y, buf);
    y += 14;
  }

  u8g2.setDrawColor(1); // khôi phục màu chữ bình thường
}

void DisplayManager::setParameterValue(int index, float val) {
  if (index < 0 || index >= PARAM_COUNT) return;

  if (val < params[index].minVal)
    val = params[index].minVal;
  if (val > params[index].maxVal)
    val = params[index].maxVal;

  params[index].value = val;
}

void dtostr52_buf(char *buf, size_t size, float val) {
  int whole = (int)val;
  int frac = (int)((val - whole) * 1000);
  if (frac < 0) frac = -frac;
  snprintf(buf, size, "%d.%03d", whole, frac);
}

void DisplayManager::drawHomeStatusScreen() {
  if (!requestPrinterStatus()) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.setCursor(0, 20);
    u8g2.print("Status read error");
    u8g2.sendBuffer();
    return;
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);

  // Vị trí các trục
 char buf[16]; // chọn đủ lớn cho chuỗi
for (int i = 0; i < AXIS_COUNT; i++) {
  dtostr52_buf(buf, sizeof(buf), printerStatus.position[i]);
  
  char line[32];
  snprintf(line, sizeof(line), "%c: %s", axesNames[i], buf);

  u8g2.setCursor(0, 25 + i * 10);
  u8g2.print(line);
}

  // Trạng thái máy
  const char* state_text = "Unknown";
  switch (printerStatus.state) {
    case 0: state_text = "Idle"; break;
    case 1: state_text = "Printing"; break;
  }
  u8g2.setCursor(0, 25 + AXIS_COUNT * 10);
  u8g2.print("State: ");
  u8g2.print(state_text);

  // Thời gian chạy job
  uint32_t seconds = printerStatus.elapsed_seconds;
  uint32_t hours = seconds / 3600;
  uint32_t minutes = (seconds % 3600) / 60;
  snprintf(buf, sizeof(buf), "Time: %u:%02u", hours, minutes);
  u8g2.setCursor(0, 25 + (AXIS_COUNT + 1) * 10);
  u8g2.print(buf);

  // Nhiệt độ hotend (hiển thị T0, T1, ...)
  int xPos = 0;
  int yPos = 25 + (AXIS_COUNT + 2) * 10;
  for (int e = 0; e < EXTRUDERS; e++) {
    snprintf(buf, sizeof(buf), "T%d:%.1f/%.1f ", e,
             printerStatus.hotend_temps[e], printerStatus.hotend_targets[e]);
    u8g2.setCursor(xPos, yPos);
    u8g2.print(buf);
    xPos += u8g2.getStrWidth(buf);
  }

  // Nhiệt độ bed
  yPos += 10;
  u8g2.setCursor(0, yPos);
  snprintf(buf, sizeof(buf), "Bed: %.1f/%.1f", printerStatus.bed_temp, printerStatus.bed_target);
  u8g2.print(buf);

  // Feedrate %
  yPos += 10;
  u8g2.setCursor(0, yPos);
  snprintf(buf, sizeof(buf), "Feedrate: %d%%", printerStatus.feedrate_percentage);
  u8g2.print(buf);

  u8g2.sendBuffer();
}
   
float DisplayManager::getParameterValue(int index) const {
  if (index < 0 || index >= PARAM_COUNT) return 0;
  return params[index].value;
}
ConnectionStatusDisplay::ConnectionStatusDisplay(U8G2_ST7920_128X64_F_SW_SPI& u8g2) 
  : u8g2_ref(u8g2) { }

void ConnectionStatusDisplay::draw(bool connected) {
  if (!connected) {
    unsigned long now = millis();

    if (now - lastToggleMS > blinkInterval) {
      lastToggleMS = now;
      visible = !visible;  // Đảo trạng thái để nhấp nháy
    }

    if (visible) {
      u8g2_ref.setFont(u8g2_font_6x12_tr);
      u8g2_ref.drawStr(0, 12, "Waiting for Marlin");
    }
    // Không gọi clearBuffer hoặc sendBuffer ở đây nữa
  } 
else {
    // Nếu đã kết nối thì không vẽ cảnh báo gì
  }
}

  