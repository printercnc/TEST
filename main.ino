+----------------------------+
| LCD + Encoder + Keypad      |
| - Giao diện người dùng     | <-- Code VSCode / Arduino dành cho STM32
| - Hiển thị thông tin        |
| - Điều khiển trục Jog       |
| - Hiển thị G54..G59         |
| - Quản lý file GCode gửi    |
+----------------------------+
             |
             | UART hoặc I2C (ví dụ I2C Master)
             |
+----------------------------+
| Board chính Marlin          |
| - Điều khiển trục           |
| - Xử lý GCode, in 3D        |
+----------------------------+
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- Phần cứng kết nối ---
#define PIN_CS    PA4
#define PIN_RST   PA5
#define PIN_SCK   PA5  // SPI Clock
#define PIN_MOSI  PA7  // SPI MOSI

#define PIN_ENC_A PA0
#define PIN_ENC_B PA1
#define PIN_ENC_BTN PA2

// Bàn phím 4x4
const uint8_t rowPins[4] = { PB0, PB1, PB2, PB3 };
const uint8_t colPins[4] = { PB4, PB5, PB6, PB7 };

// Địa chỉ I2C của board chính (Marlin)
#define SLAVE_ADDRESS 8

// --- U8g2 khởi tạo
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, PIN_CS, PIN_RST, PIN_SCK);

// --- Variables ---
volatile int encoderPos = 0;
volatile int lastEncoded = 0;

int lastEncA = HIGH;
int lastEncB = HIGH;
int encoderButtonState = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Giá trị jog từng bước
const int JOG_STEP = 1;  // mm hoặc đơn vị tương đương

// Menu trạng thái
enum MenuStateEnum {
  MENU_STATUS,
  MENU_G54_OFFSET,
  MENU_SEND_GCODE,
  MENU_COUNT
};

MenuStateEnum currentMenu = MENU_STATUS;

// Offset G54..G59 giả lập
float g54_offset[undefined
