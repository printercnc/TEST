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
#include "encoder.h"

// --- Phần cứng kết nối ---
#define PIN_CS    PA4
#define PIN_RST   PA6
#define PIN_SCK   PA5  // SPI Clock
#define PIN_MOSI  PA7  // SPI MOSI

#define PIN_ENC_A PA0
#define PIN_ENC_B PA1
#define PIN_ENC_BTN PA2

// --- Giả định khai báo/macro đầu file ---
#define EXTRUDERS 1        // Hoặc theo code bạn
#define E_AXIS 4           // Vị trí trục E trong mảng offset (ví dụ 4 hoặc 5...)
#define EXT_AXIS_START 3   // Trục mở rộng bắt đầu từ vị trí 3 (A trục 3, C trục 4,...)
#define EXT_AXIS_MAX_COUNT 3
char ext_axis_names[EXT_AXIS_MAX_COUNT] = {'A', 'C', 'D'}; // bạn thêm tên các trục mở rộng kèm E nếu cần
int ext_axis_count = 2; // số trục mở rộng chưa E (A, C)

#if defined(E_AXIS) && (EXTRUDERS > 0) && (E_AXIS >= 0)
  #define HAS_E_AXIS 1
#else
  #define HAS_E_AXIS 0
#endif

const int btnPagePin = 7;  // Chân nút chuyển trang (thay 7 nếu chân khác)
#include "encoder.h"

void setup() {
  Serial.begin(115200);
  HAL_Init();

  Encoders_Init_All();
}

void loop() {
  int32_t posX = Encoder_Read(&encoderX);
  int32_t posY = Encoder_Read(&encoderY);
  int32_t posZ = Encoder_Read(&encoderZ);
  int32_t posA = Encoder_Read(&encoderA);
  int32_t posC = Encoder_Read(&encoderC);
  int32_t posE = Encoder_Read(&encoderE);

  Serial.print("X:"); Serial.print(posX);
  Serial.print(" Y:"); Serial.print(posY);
  Serial.print(" Z:"); Serial.print(posZ);
  Serial.print(" A:"); Serial.print(posA);
  Serial.print(" C:"); Serial.print(posC);
  Serial.print(" E:"); Serial.println(posE);

  delay(100);
}

  // Khởi tạo chân nút chuyển trang
  pinMode(btnPagePin, INPUT_PULLUP);

  // Khởi tạo encoder pins
  #define PIN_ENC_A PA0
#define PIN_ENC_B PA1

volatile long encoderPos = 0;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encoderISR, CHANGE);
}

void loop() {
  static long lastPos = 0;
  if (lastPos != encoderPos) {
    Serial.print("Encoder Position: ");
    Serial.println(encoderPos);
    lastPos = encoderPos;
  }
  delay(5);
}

void encoderISR() {
  bool encA = digitalRead(PIN_ENC_A);
  bool encB = digitalRead(PIN_ENC_B);

  if (encA == encB) encoderPos++;
  else encoderPos--;
}
  
// Bàn phím 4x4
const uint8_t rowPins[4] = { PB0, PB1, PB2, PB3 };
const uint8_t colPins[4] = { PB4, PB5, PB6, PB7 };

// Địa chỉ I2C của board chính (Marlin)
#define SLAVE_ADDRESS 8

// --- Khởi tạo màn hình U8G2 ---
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, PIN_CS, PIN_RST, PIN_SCK);


// --- Variables ---
volatile int encoderPos = 0;
int lastEncA = HIGH;
int lastEncB = HIGH;

// Trang hiện tại, 1=G54, 2=G55
int currentPage = 1;
const int maxPage = 2;

// Biến vị trí các trục hiện tại Giả lập offset G54/G55
float g54_offsets[6] = {10.123, 20.456, -5.789, 1.234, 2.345, 3.456};
float g55_offsets[6] = {5.555, -10.666, 2.777, -1.888, 4.999, 0.001};

// --- Hàm chuyển trang khi bấm nút, bạn cài đặt thêm ở nơi cần
void drawScreen();
void checkPageButton();
void readEncoder();
void checkEncoderButton();
void sendOffsetsToSlave(float* offsets, int count);

void setup() {
  Serial.begin(115200);

  pinMode(btnPagePin, INPUT_PULLUP);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_BTN, INPUT_PULLUP);

  u8g2.begin();

  Wire.begin(); // I2C Master

  drawScreen();
}

void loop() {
  checkPageButton();
  readEncoder();
  checkEncoderButton();

  // Gửi offset qua I2C tới Slave
  if (currentPage == 1) {
    sendOffsetsToSlave(g54_offsets, 6);
  } else {
    sendOffsetsToSlave(g55_offsets, 6);
  }

  delay(100);
}

void checkPageButton() {
  static bool lastBtnState = HIGH;
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;

  int reading = digitalRead(btnPagePin);
  if (reading != lastBtnState) lastDebounceTime = millis();

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastBtnState == HIGH) {
      currentPage++;
      if (currentPage > maxPage) currentPage = 1;
      drawScreen();
    }
  }
  lastBtnState = reading;
}

void readEncoder() {
  int encA = digitalRead(PIN_ENC_A);
  int encB = digitalRead(PIN_ENC_B);

  if (encA != lastEncA) {
    if (encA == LOW) {
      if (encB == HIGH) {
        encoderPos++;
        Serial.print("Encoder Pos: ");
        Serial.println(encoderPos);
      } else {
        encoderPos--;
        Serial.print("Encoder Pos: ");
        Serial.println(encoderPos);
      }
    }
  }
  lastEncA = encA;
  lastEncB = encB;
}

void checkEncoderButton() {
  int reading = digitalRead(PIN_ENC_BTN);
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;

  static int lastButtonState = HIGH;

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastButtonState == HIGH) {
      Serial.println("Encoder Button Pressed");
    }
  }
  lastButtonState = reading;
}

void sendOffsetsToSlave(float* offsets, int count) {
  Wire.beginTransmission(SLAVE_ADDRESS);
  for (int i = 0; i < count; i++) {
    union {
      float f;
      byte b[4];
    } u;
    u.f = offsets[i];
    Wire.write(u.b, 4);
  }
  Wire.endTransmission();
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
    u8g2.sendBuffer();
}

  for (int i = 0; i < ext_axis_count; i++) {
    snprintf(buf, sizeof(buf), "%c:%7.3f", ext_axis_names[i], offsets[EXT_AXIS_START + i]);
    u8g2.drawStr(0, 48 + i * 12, buf);
  }

  u8g2.sendBuffer();
}