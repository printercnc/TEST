
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "stm32f1xx_hal.h"
#include "encoder.h"
#include "Status.h"
#include "DisplayManager.h" 
uint8_t currentPage = PAGE_WARNING;

static int selectedMenuIndex = 0;    // chỉ số hàng được chọn hiện tại
static bool editingPosition = false; // khi cần chỉnh vị trí (ví dụ trang Machine)
static int editingAxisIndex = -1;    // trục đang chỉnh vị trí (nếu có)

#define PIN_CLK PA5   // Clock (SCLK)
#define PIN_DATA PA7  // Data (MOSI / SID)
#define PIN_CS PA4    // Chip Select
#define PIN_RST -1   // Reset (hoặc dùng U8X8_PIN_NONE nếu không có chân reset)
#define PAGE_COUNT 7

// encoder hardware timer handle (TIM2)
extern TIM_HandleTypeDef htim2;
extern Encoder_t encoderY; // Biến encoder đã khai báo trong encoder.cpp

// Khởi tạo chân nút chuyển trang
const uint8_t rowPins[4] = {PB0, PB1, PB3, PB4};
const uint8_t colPins[4] = {PA2, PA3, PB5, PA8};

#define SLAVE_ADDRESS ((uint8_t)0x42) // Địa chỉ I2C của board chính (Marlin)

U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, PIN_CLK, PIN_DATA, PIN_CS, PIN_RST);
DisplayManager display(u8g2);
ConnectionStatusDisplay connStatus(u8g2);

char selectedAxis = 'X';
int selectedAxisIndex = 0;
bool lastButtonState = HIGH;
int jogMultiplierIndex = 0;
const int jogMultipliers[] = {1, 10, 100};

bool dataValid = false;

unsigned long lastCheckConnMS = 0;
const unsigned long checkConnInterval = 1000;

static unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

bool homeCommandSent = false;
bool homeDone = false; 

// Kiểm tra kết nối Marlin qua I2C
bool checkMarlinConnection() {
  Wire.beginTransmission(SLAVE_ADDRESS);
  uint8_t err = Wire.endTransmission();
  return (err == 0);
}

 bool requestOffsetsFromMarlin() {
  const uint8_t slaveAddr = SLAVE_ADDRESS;
  const uint8_t length = 12; // 3 float * 4 bytes

  uint8_t receivedData[length];   // <-- Khai báo biến mảng tại đây
  uint8_t receivedLength = 0;     // <-- Khai báo biến độ dài nhận được

  // Gửi lệnh lấy dữ liệu G54
  Wire.beginTransmission(slaveAddr);
  Wire.write("GET_G54");
  if (Wire.endTransmission() != 0) {
    return false; // lỗi truyền
  }

  // Yêu cầu 12 byte dữ liệu từ slave
  uint8_t bytesRead = Wire.requestFrom(slaveAddr, length);
  if (bytesRead != length) return false;

  uint8_t idx = 0;
  while (Wire.available() && idx < length) {
    receivedData[idx++] = Wire.read();
  }
  receivedLength = idx;

  // parse thành float nếu cần (ví dụ float g54X = *((float*)&receivedData[0]) ...)

  return true; // thành công
}

bool requestPrinterStatus() {
    const uint8_t len = sizeof(printerStatus);
    Wire.beginTransmission(SLAVE_ADDRESS);
    Wire.write("STATUS_REQ");
    if (Wire.endTransmission() != 0) return false;

    uint8_t bytesRead = Wire.requestFrom(SLAVE_ADDRESS, len);
    if (bytesRead != len) return false;

    uint8_t* p = (uint8_t*)&printerStatus;
    for (uint8_t i = 0; i < len && Wire.available(); i++) {
        p[i] = Wire.read();
    }
    return true;
}

// Hàm quét phím 4x4
char scanKeyboard() {
  // Bảng map ký tự theo phím
  // Bạn có thể thiết kế lại phù hợp nút chức năng
  // Ví dụ:
  //  '1' '2' '3' 'A'
  //  '4' '5' '6' 'B'
  //  '7' '8' '9' 'C'
  //  '*' '0' '#' 'D'

  const char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
  };

  for (uint8_t c = 0; c < 4; c++) {
    pinMode(colPins[c], OUTPUT);
    digitalWrite(colPins[c], LOW);
  for (uint8_t r = 0; r < 4; r++) {
      pinMode(rowPins[r], INPUT_PULLUP);
      if (digitalRead(rowPins[r]) == LOW) {
        delay(50); // debounce
        while (digitalRead(rowPins[r]) == LOW) {}
        digitalWrite(colPins[c], HIGH);
        pinMode(colPins[c], INPUT);
        return keymap[r][c];
      }
    }
    digitalWrite(colPins[c], HIGH);
    pinMode(colPins[c], INPUT);
  }
  return 0;
}
  // Gửi lệnh jog G-code qua I2C
void sendJogCommand(char axis, int32_t delta) {
  if (delta == 0) return;

  char buf[48];
  float step = delta * jogMultipliers[jogMultiplierIndex] * 0.01f; // Ví dụ: step nhảy 0.01 mỗi tick
  snprintf(buf, sizeof(buf), "G91\nG0 %c%.3f\nG90\n", axis, step);

  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write((uint8_t*)buf, strlen(buf));
  Wire.endTransmission();
}

void sendHomeCommand(char axis) {
  char buf[32];
  if (axis == 'D') {
    snprintf(buf, sizeof(buf), "G28\n"); // Home tất cả
  } else {
    snprintf(buf, sizeof(buf), "G28 %c\n", axis); // Home trục cụ thể
  }
  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write((uint8_t*)buf, strlen(buf));
  Wire.endTransmission();

  Serial.print("Home command sent: ");
  Serial.println(buf);
  homeCommandSent = true;   // <-- Thêm dòng này
  homeDone = false;         // reset cờ
}

bool readHomeStatus() {
  const uint8_t statusLength = 1; // giả sử Marlin trả 1 byte trạng thái
  Wire.requestFrom((uint8_t)SLAVE_ADDRESS, (uint8_t)statusLength);

  if (Wire.available() >= statusLength) {
    uint8_t status = Wire.read();
    Serial.print("Received home status: ");
    Serial.println(status, HEX);
    return (status == 0x01); // 0x01 = home done, tùy bạn định nghĩa
  }
  return false; // chưa nhận được trạng thái
}

// Chỉ dùng khi đang trang Machine Control
static int machineMenuIndex = 0;  // 0: Home All, 1: X, 2:Y, ... 6: E
static float axisPositions[AXIS_COUNT] = {0.0f}; // vị trí các trục giả lập, cập nhật khi xoay encoder

const char axesNames[] = {'X','Y','Z','A','C','E'};
const int machineMenuCount = 6;

void handleMenuNavigation(char key, int menuItemCount, int& selectedIndex, bool& editing, int& editingAxis) {
    switch (key) {
        case '5': // lên
            if (!editing) {
                selectedIndex--;
                if (selectedIndex < 0)
                    selectedIndex = menuItemCount - 1;
            } else {
                // Trong chỉnh sửa: tăng vị trí trục +0.01 (ví dụ)
        if (editingAxis >= 0 && editingAxis < AXIS_COUNT) {
          axisPositions[editingAxis] += 0.01f;
            }
          }
            break;

        case '0': // xuống
            if (!editing) {
                selectedIndex++;
                if (selectedIndex >= menuItemCount)
                    selectedIndex = 0;
            } else {
                // Trong chỉnh sửa: giảm vị trí trục -0.01
        if (editingAxis >= 0 && editingAxis < AXIS_COUNT) {
          axisPositions[editingAxis] -= 0.01f;
            }
          }
            break;

        case '8': // enter
            if (editing) {
                // Lưu xong, thoát chỉnh sửa
                editing = false;
                editingAxis = -1;
            } else {
                // Bắt đầu chỉnh sửa hoặc gửi lệnh tương ứng
        if (selectedIndex == 0) {
          // Home All
          sendHomeCommand('D');  // Gửi lệnh home tất cả
          Serial.println("Sent Home All command");
        } else {
          // Bắt đầu chỉnh sửa vị trí trục (chỉ 5 trục, bỏ trục E)
          editing = true;
          editingAxis = selectedIndex - 1;  
          Serial.print("Start editing axis ");
          Serial.println(axesNames[editingAxis]);
        }
      }
            break;
        default:
            break;
    }
}

void handleMachineControlSelection(int& selectedIndex, bool& editing, int& editingAxis, float axisPositions[], const char axesNames[]) {
    if (editing) {
        // Khi đang chỉnh vị trí => điều chỉnh giá trị vị trí qua encoder hoặc nút lên/xuống
        // (có thể để trong handleMenuNavigation hoặc bổ sung)
    } else {
        if (selectedIndex == 0) {
            // Home All
            sendHomeCommand('D'); // home tất cả
            Serial.println("Sent Home All command");
        } else {
            // Bắt đầu chỉnh vị trí trục
            editing = true;
            editingAxis = selectedIndex - 1;
            Serial.print("Start editing axis ");
            Serial.println(axesNames[editingAxis]);
        }
    }
}

void readEncoder() {
  uint32_t count = __HAL_TIM_GET_COUNTER(&encoderY.htim);
  int32_t delta = (int32_t)(count - encoderY.lastCount);
  if (delta != 0) {
    Serial.print("Encoder delta: ");
    Serial.println(delta);
    encoderY.lastCount = count;
    sendJogCommand(selectedAxis, delta);
  }
}

void setupHardware() {

     // Khởi tạo encoder từ biến toàn cục encoderY trong encoder.cpp
    Encoder_Init(&encoderY, TIM2);
    HAL_TIM_Encoder_Start(&encoderY.htim, TIM_CHANNEL_ALL);
    // Khởi tạo pin bàn phím (các chân row, col)
    for (int c = 0; c < 4; c++) {
        pinMode(colPins[c], INPUT);}
    for (int r = 0; r < 4; r++){
        pinMode(rowPins[r], INPUT_PULLUP);}
    }

void setup() {
  Wire.begin();
  setupHardware();
  u8g2.begin();
  Serial.begin(115200);
}

void loop() {
 unsigned long now = millis();
  // Kiểm tra kết nối mỗi 1 giây
  if(now - lastCheckConnMS > checkConnInterval) {
    dataValid = checkMarlinConnection();
    lastCheckConnMS = now;
  if(dataValid && currentPage == PAGE_WARNING) {
      currentPage = PAGE_G54;
    }
  }

   if (homeCommandSent && !homeDone) {
    homeDone = readHomeStatus();
    if (homeDone) {
      Serial.println("Home command completed by Marlin");
       currentPage = PAGE_HOME_STATUS; 
      homeCommandSent = false; // reset cờ
    }
  }

  // Khi đã kết nối hoặc chưa kết nối, đều cho phép chuyển trang bằng phím '*'
char key = scanKeyboard();
if (key != 0) {
  if (key == '*') {
    currentPage = (currentPage + 1) % PAGE_COUNT;
    Serial.print("Page changed by keyboard '*', currentPage = ");
    Serial.println(currentPage);
  }
  if (dataValid) {
     if (key != 0) {
    // Gọi hàm xử lý menu nếu đang ở trang MACHINE và key là 0,5,8
    if (currentPage == PAGE_MACHINE) {
      handleMenuNavigation(key, machineMenuCount, selectedMenuIndex, editingPosition, editingAxisIndex);
    }
  }
      switch (key) {
        case '*':  // chuyển trang
          currentPage = (currentPage + 1) % PAGE_COUNT;
          Serial.print("Page changed by keyboard '*', currentPage = ");
          Serial.println(currentPage);
          break;

        case '1': case 'X':  selectedAxis = 'X'; selectedAxisIndex=0; break;
        case '2': case 'Y':  selectedAxis = 'Y'; selectedAxisIndex=1; break;
        case '3': case 'Z':  selectedAxis = 'Z'; selectedAxisIndex=2; break;
        case 'A':            selectedAxis = 'A'; selectedAxisIndex=3; break;
        case 'C':            selectedAxis = 'C'; selectedAxisIndex=4; break;
        case '#':            selectedAxis = 'E'; selectedAxisIndex=5; break;

        case 'B':  // thay đổi cấp độ bước nhảy encoder
          jogMultiplierIndex = (jogMultiplierIndex + 1) % (sizeof(jogMultipliers) / sizeof(int));
          break;

          case 'D':  // Nút home all
  sendHomeCommand('D');   // Gửi lệnh home tất cả
  Serial.println("Sent Home All command");
  break;
      }
    }
  }

  // Vẽ màn hình
  u8g2.clearBuffer();

  if (!dataValid) {
    // Khi chưa kết nối, vẫn cho phép chuyển trang bằng nút encoder trên LCD
    // Vẽ giao diện trang hiện tại bên dưới cảnh báo
    switch(currentPage) {
       case PAGE_WARNING:
        connStatus.draw(false);   // Vẽ chữ "Waiting for Marlin" nhấp nháy
        break;
      case PAGE_G54:
        display.drawOffsetsPage(display.getG54Offsets(), "Offset G54:");
        break;
      case PAGE_G55:
        display.drawOffsetsPage(display.getG55Offsets(), "Offset G55:");
        break;
      case PAGE_MACHINE:
        display.drawMachineControlPage(selectedMenuIndex, selectedAxis, axisPositions[selectedAxisIndex]);
        break;
      case PAGE_PARAMETER:
        display.drawParameterPage(0);
        break;
      default:
        connStatus.draw(false);
        break;
        case PAGE_HOME_STATUS:
     display.drawHomeStatusScreen();
     break;
    }

    if
     (currentPage == PAGE_HOME_STATUS) {
    if (requestPrinterStatus()) {
     display.drawHomeStatusScreen();
  } else {
    // Nếu không nhận được dữ liệu trạng thái từ Marlin
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.setCursor(0, 20);
    u8g2.print("Status read error");
    u8g2.sendBuffer();
  }

    }
     } else {
    // Đã kết nối -> vẽ trang dữ liệu theo trang hiện tại
    switch (currentPage) {
      case PAGE_G54:
        display.drawOffsetsPage(display.getG54Offsets(), "Offset G54:");
        break;

      case PAGE_G55:
        display.drawOffsetsPage(display.getG55Offsets(), "Offset G55:");
        break;

        case PAGE_MACHINE:
        display.drawMachineControlPage(selectedMenuIndex, selectedAxis, axisPositions[selectedAxisIndex]);
        break;

      case PAGE_PARAMETER:
        // Bạn có thể quản lý selected param index bằng biến để chỉnh sửa tham số
        static int paramSelectedIndex = 0;
        display.drawParameterPage(paramSelectedIndex);
        // TODO: bổ sung xử lý chỉnh sửa tham số nếu cần
        break;
        case PAGE_HOME_STATUS:
     display.drawHomeStatusScreen();
     break;

      default:
        // Nếu người dùng chuyển sang trang cảnh báo trong trạng thái connected thì vẽ rỗng hoặc trang ưa thích
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(0, 12, "Connected");
        break;
    }
  }

  // Gửi nội dung buffer ra màn hình
  u8g2.sendBuffer();

  // Đọc encoder 2 để gửi lệnh jog chỉ khi đã kết nối
  if (dataValid) {
    readEncoder();
  }

  delay(10);
}
void My_Error_Handler() {
    while (1) {
        // Có thể đèn LED nhấp nháy hoặc các xử lý debug khác
    }
  }