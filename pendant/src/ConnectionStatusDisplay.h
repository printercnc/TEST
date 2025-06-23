#ifndef CONNECTIONSTATUSDISPLAY_H
#define CONNECTIONSTATUSDISPLAY_H

#include <U8g2lib.h>

class ConnectionStatusDisplay {
public:
    ConnectionStatusDisplay(U8G2_ST7920_128X64_F_SW_SPI& u8g2);

    // Gọi trong loop, truyền trạng thái kết nối (true = có kết nối)
    void update(bool connected);

private:
    U8G2_ST7920_128X64_F_SW_SPI& u8g2_ref;

    unsigned long lastToggleMS = 0;
    bool visible = true;  // trạng thái hiển thị để tạo nhấp nháy
    const unsigned long blinkInterval = 500; // 500ms nhấp nháy
};

#endif