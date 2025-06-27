
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "stm32f1xx_hal.h"
#include "encoder.h"
#include "DisplayManager.h" 
#include "EncoderButtonPageSwitcher.h"

#define PIN_CLK PA5   // Clock (SCLK)
#define PIN_DATA PA7  // Data (MOSI / SID)
#define PIN_CS PA4    // Chip Select
#define PIN_RST -1   // Reset (hoặc dùng U8X8_PIN_NONE nếu không có chân reset)
#define ENC_BTN_PIN PB6
#define PAGE_COUNT 4

uint8_t currentPage = PAGE_WARNING;  

EncoderButtonPageSwitcher pageSwitcher(ENC_BTN_PIN, PAGE_COUNT);
// encoder hardware timer handle (TIM2)
extern TIM_HandleTypeDef htim2;
extern Encoder_t encoderY; // Biến encoder đã khai báo trong encoder.cpp

// Khởi tạo chân nút chuyển trang
const uint8_t rowPins[4] = {PB0, PB1, PB2, PB3};
const uint8_t colPins[4] = {PB4, PB5, PA3, PA8};

#define SLAVE_ADDRESS 0x42 // Địa chỉ I2C của board chính (Marlin)

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

// Kiểm tra kết nối Marlin qua I2C
bool checkMarlinConnection() {
  Wire.beginTransmission(SLAVE_ADDRESS);
  uint8_t err = Wire.endTransmission();
  return (err == 0);
}
 
// Hàm xử lý debounce và trả về true khi nút encoder được nhấn (true chỉ báo sự kiện nhấn 1 lần)
bool encoderButtonPressed() {
  static bool lastButtonState = HIGH;
  bool reading = digitalRead(ENC_BTN_PIN);
  bool pressed = false;

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
}

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastButtonState == HIGH) {
       Serial.println("Encoder button pressed detected!");
      pressed = true;
    }
  }

  lastButtonState = reading;
  return pressed;
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
        // Khởi tạo chân nút encoder RepRap (đảm bảo cổng và mode đúng)
  pinMode(ENC_BTN_PIN, INPUT_PULLUP);
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
      pageSwitcher.resetPage(currentPage);
    }
  }
 int btnState = digitalRead(ENC_BTN_PIN);
  if (btnState != lastButtonState) {
  lastDebounceTime = millis();
  lastButtonState = btnState;  // **Cập nhật ngay đây**
}
  if ((millis() - lastDebounceTime) > debounceDelay) {
  if (btnState == LOW) {
    // Xử lý nhấn nút ở đây
    currentPage = (currentPage + 1) % PAGE_COUNT;
    Serial.print("Page changed: ");
    Serial.println(currentPage);
  }
}
else {
    // Đã kết nối, chuyển trang qua phím '*' bàn phím 4x4
    char key = scanKeyboard();
    if (key == '*') {
      currentPage = (currentPage + 1) % PAGE_COUNT;
      Serial.print("Page changed by keyboard '*', currentPage = ");
      Serial.println(currentPage);
    }
  }
  // Xóa buffer trước khi vẽ
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
      case PAGE_PARAMETER:
        display.drawParameterPage(0);
        break;
      default:
        connStatus.draw(false);
        break;
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

      case PAGE_PARAMETER:
        // Bạn có thể quản lý selected param index bằng biến để chỉnh sửa tham số
        static int paramSelectedIndex = 0;
        display.drawParameterPage(paramSelectedIndex);
        // TODO: bổ sung xử lý chỉnh sửa tham số nếu cần
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

  // Quét bàn phím và xử lý các phím
  if (dataValid) {
     char key = scanKeyboard();
     if (key != 0) {
        switch(key) {
        case '*': // chuyển trang
        currentPage = (currentPage + 1) % PAGE_COUNT;
        Serial.print("Keyboard '*' pressed, currentPage = ");
        Serial.println(currentPage);
        break;

      case '1': case 'X':  selectedAxis = 'X'; selectedAxisIndex=0; break;
      case '2': case 'Y':  selectedAxis = 'Y'; selectedAxisIndex=1; break;
      case '3': case 'Z':  selectedAxis = 'Z'; selectedAxisIndex=2; break;
      case 'A':            selectedAxis = 'A'; selectedAxisIndex=3; break;
      case 'C':            selectedAxis = 'C'; selectedAxisIndex=4; break;
      case '#':            selectedAxis = 'E'; selectedAxisIndex=5; break;

      case 'B': // thay đổi cấp độ bước nhảy encoder
        jogMultiplierIndex = (jogMultiplierIndex + 1) % (sizeof(jogMultipliers) / sizeof(int));
        break;
    }
  }
  }
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
