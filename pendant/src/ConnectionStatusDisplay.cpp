#include "ConnectionStatusDisplay.h"

ConnectionStatusDisplay::ConnectionStatusDisplay(U8G2_ST7920_128X64_F_SW_SPI& u8g2) 
  : u8g2_ref(u8g2) { }

void ConnectionStatusDisplay::update(bool connected) {
  if (!connected) {
    unsigned long now = millis();

    if (now - lastToggleMS > blinkInterval) {
      lastToggleMS = now;
      visible = !visible;  // Đảo trạng thái hiện/ẩn để nhấp nháy
    }

    u8g2_ref.clearBuffer();
    u8g2_ref.setFont(u8g2_font_6x12_tr);

    if (visible) {
      u8g2_ref.drawStr(0, 30, "Waiting for Marlin");
    }
    u8g2_ref.sendBuffer();

  } else {
    // Nếu đã kết nối thì không hiển thị gì ở đây, để main xử lý giao diện chính
  }
}