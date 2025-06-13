//+----------------------------+
//| LCD + Encoder + Keypad      |
//| - Giao diện người dùng     | <-- Code VSCode / Arduino dành cho STM32
//| - Hiển thị thông tin        |
//| - Điều khiển trục Jog       |
//| - Hiển thị G54..G59         |
//| - Quản lý file GCode gửi    |
//+----------------------------+
            // |
             //| UART hoặc I2C (ví dụ I2C Master)
             //|
//+----------------------------+
//| Board chính Marlin          |
//| - Điều khiển trục           |
//| - Xử lý GCode, in 3D        |
//+----------------------------+
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "stm32f1xx_hal.h"
#include "encoder.h"
Encoder_t encoderY;
// --- Hiển thị giao diện ---
int currentPage = 1;
const int maxPage = 2;

// --- Phần cứng kết nối ---
#define PIN_CS    PA4
#define PIN_RST   PA6
#define PIN_SCK   PA5  // SPI Clock
#define PIN_MOSI  PA7  // SPI MOSI
// Khởi tạo encoder pins
#define PIN_ENC_BTN PA2
// Encoder pin đã khai báo trong Encoder lib - hiện dùng TIM2 (PA0, PA1)


// Khởi tạo chân nút chuyển trang
const uint8_t rowPins[4] = {PB0, PB1, PB2, PB3};
const uint8_t colPins[4] = {PB4, PB5, PB6, PB7};

#define SLAVE_ADDRESS 8 // Địa chỉ I2C của board chính (Marlin)

U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, PA4, PA6, PA5);

// encoder hardware timer handle (TIM2)
extern TIM_HandleTypeDef htim2;

volatile int32_t encoderPos = 0;
volatile int32_t encoderPrev = 0;

// Trục chọn, mặc định trục X
char selectedAxis = 'X';
char axes[] = {'X', 'Y', 'Z', 'A', 'C', 'E'};
int selectedAxisIndex = 0;

// Cấp độ bước
int jogMultipliers[] = {1, 10, 100};
int jogMultiplierIndex = 0;

// --- Giả định khai báo/macro đầu file ---
#define EXTRUDERS 1        // Hoặc theo code bạn
#define E_AXIS 4           // Vị trí trục E trong mảng offset (ví dụ 4 hoặc 5...)
#define EXT_AXIS_START 3   // Trục mở rộng bắt đầu từ vị trí 3 (A trục 3, C trục 4,...)
#define EXT_AXIS_MAX_COUNT 3


#if defined(E_AXIS) && (EXTRUDERS > 0) && (E_AXIS >= 0)
  #define HAS_E_AXIS 1
#else
  #define HAS_E_AXIS 0
#endif

char ext_axis_names[EXT_AXIS_MAX_COUNT] = {'A', 'C', 'D'}; // bạn thêm tên các trục mở rộng kèm E nếu cần
int ext_axis_count = 2; // số trục mở rộng chưa E (A, C)

// Biến vị trí các trục hiện tại Giả lập offset G54/G55
float g54_offsets[6] = {10.123, 20.456, -5.789, 1.234, 2.345, 3.456};
float g55_offsets[6] = {5.555, -10.666, 2.777, -1.888, 4.999, 0.001};

//-------------------
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

void readEncoder();
void drawScreen();

void setupHardware() {
    Wire.begin();
    u8g2.begin();

    // Khởi tạo encoder
    Encoder_Init(&encoderY, TIM2);
    HAL_TIM_Encoder_Start(&encoderY.htim, TIM_CHANNEL_ALL);

    // Khởi tạo pin bàn phím (các chân row, col)
    for (int c = 0; c < 4; c++) {
        pinMode(colPins[c], INPUT);
    }
    for (int r = 0; r < 4; r++) {
        pinMode(rowPins[r], INPUT_PULLUP);
    }

    // Khởi tạo nút encoder
    pinMode(PIN_ENC_BTN, INPUT_PULLUP);
}

void setup() {
    setupHardware();
}

void loop() {
    // Quét bàn phím, xử lý chọn trục, trang, cấp độ bước
    char key = scanKeyboard();
    if (key != 0) {
        switch (key) {
            case 'X': case '1':
                selectedAxis = 'X';
                selectedAxisIndex = 0;
                break;
            case 'Y': case '2':
                selectedAxis = 'Y';
                selectedAxisIndex = 1;
                break;
            case 'Z': case '3':
                selectedAxis = 'Z';
                selectedAxisIndex = 2;
                break;
            case 'A':
                selectedAxis = 'A';
                selectedAxisIndex = 3;
                break;
            case 'C':
                selectedAxis = 'C';
                selectedAxisIndex = 4;
                break;
            case 'E':
                selectedAxis = 'E';
                selectedAxisIndex = E_AXIS;
                break;
            case '*':
                currentPage = (currentPage % maxPage) + 1;
                break;
            case 'B':  // Ví dụ đổi cấp độ bước
                jogMultiplierIndex = (jogMultiplierIndex + 1) % (sizeof(jogMultipliers) / sizeof(int));
                break;
            // Thêm nút khác nếu cần
        }
    }

    // Đọc encoder, gửi lệnh jog nếu có delta
    readEncoder();

    // Hiển thị màn hình
    drawScreen();

    delay(50);
}

// --- Gửi lệnh jog dạng G-code theo trục, delta ---
void sendJogCommand(char axis, int32_t delta) {
  if (delta == 0) return;
  
  char buf[48];
  float step = delta * jogMultipliers[jogMultiplierIndex] * 0.01f; // độ chính xác bước
  // Lưu ý: giữa 'G0' và trục phải có khoảng trắng, ví dụ "G0 X10.000"
  snprintf(buf, sizeof(buf), "G91\nG0 %c%.3f\nG90\n", axis, step);

  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write((uint8_t*)buf, strlen(buf));
  Wire.endTransmission();
}

// --- Hàm đọc giá trị encoder và cập nhật vị trí ---
void readEncoder() {
    uint32_t count = __HAL_TIM_GET_COUNTER(&encoderY.htim);
    int32_t delta = (int32_t)(count - encoderY.lastCount);

    if (delta != 0) {
        encoderY.lastCount = count;
        sendJogCommand(selectedAxis, delta);
    }
}



void drawScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);

  float* offsets = (currentPage == 1) ? g54_offsets : g55_offsets;
  const char* title = (currentPage == 1) ? "Offset G54:" : "Offset G55:";
  u8g2.drawStr(0, 12, title);

  char buf[32];
  snprintf(buf, sizeof(buf), "X:%7.3f Y:%7.3f Z:%7.3f", offsets[0], offsets[1], offsets[2]);
  u8g2.drawStr(0, 24, buf);

  snprintf(buf, sizeof(buf), "E:%7.3f", offsets[4]);
  u8g2.drawStr(0, 36, buf);

  for (int i = 0; i < ext_axis_count; i++) {
    snprintf(buf, sizeof(buf), "%c:%7.3f", ext_axis_names[i], offsets[EXT_AXIS_START + i]);
    u8g2.drawStr(0, 48 + i * 12, buf);
  }

  u8g2.sendBuffer();
}

// main.cpp

void My_Error_Handler(void) {
    while (1) {}
}

