#include "DisplayManager.h"

DisplayManager::DisplayManager(U8G2_ST7920_128X64_F_SW_SPI& display)
  : u8g2(display)
{
  // Khởi tạo giá trị giả lập offset G54 / G55
  float g54_init[AXIS_COUNT] = {10.123f, 20.456f, -5.789f, 1.234f, 2.345f, 3.456f};
  float g55_init[AXIS_COUNT] = {5.555f, -10.666f, 2.777f, -1.888f, 4.999f, 0.001f};

  memcpy(g54_offsets, g54_init, sizeof(g54_offsets));
  memcpy(g55_offsets, g55_init, sizeof(g55_offsets));
}

void DisplayManager::drawOffsetsPage(const float offsets[AXIS_COUNT], const char* title) {
  u8g2.clearBuffer();
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
  u8g2.sendBuffer();
}

void DisplayManager::drawParameterPage(int selectedParamIdx) {
  u8g2.clearBuffer();
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
  u8g2.sendBuffer();
}

void DisplayManager::setParameterValue(int index, float val) {
  if (index < 0 || index >= PARAM_COUNT) return;

  if (val < params[index].minVal)
    val = params[index].minVal;
  if (val > params[index].maxVal)
    val = params[index].maxVal;

  params[index].value = val;
}

float DisplayManager::getParameterValue(int index) const {
  if (index < 0 || index >= PARAM_COUNT) return 0;
  return params[index].value;
}