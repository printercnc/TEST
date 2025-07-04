#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <U8g2lib.h>
#include <string.h> // cho memcpy

enum Page {
  PAGE_WARNING = 0,
  PAGE_G54     = 1,
  PAGE_G55     = 2,
  PAGE_MACHINE = 3,
  PAGE_PARAMETER = 4,
  PAGE_HOME_STATUS = 5,
  PAGE_COUNT      = 7,
};
struct Parameter {
  const char* name;
  float value;
  float minVal;
  float maxVal;
  float step;
};

#define PARAM_COUNT 3
#define AXIS_COUNT 6

class DisplayManager {
public:
  DisplayManager(U8G2_ST7920_128X64_F_SW_SPI& display);

  void drawOffsetsPage(const float offsets[AXIS_COUNT], const char* title);
  void drawParameterPage(int selectedParamIdx);

  //void drawHomeStatusPage();

  void drawHomeStatusScreen();

  // Update giá trị parameter
  void setParameterValue(int index, float val);

  // Lấy giá trị parameter
  float getParameterValue(int index) const;

  
  // Thêm 2 hàm getter cho g54_offsets và g55_offsets
  const float* getG54Offsets() const { return g54_offsets; }
  const float* getG55Offsets() const { return g55_offsets; }

void drawMachineControlPage(int selectedMenuIndex, char selectedAxis, float currentPosition);

private:
  U8G2_ST7920_128X64_F_SW_SPI& u8g2;

  float g54_offsets[AXIS_COUNT];
  float g55_offsets[AXIS_COUNT];

  const char axes[AXIS_COUNT] = {'X', 'Y', 'Z', 'A', 'C', 'E'};

  Parameter params[PARAM_COUNT] = {
    {"Step Spd", 5000, 1000, 10000, 100},
    {"Accel", 1500, 100, 3000, 50},
    {"Max Spd", 3000, 500, 5000, 100}
  };
};
 class ConnectionStatusDisplay {
public:
  ConnectionStatusDisplay(U8G2_ST7920_128X64_F_SW_SPI& u8g2);
  void draw(bool connected);              // Thêm khai báo hàm draw

private:
  U8G2_ST7920_128X64_F_SW_SPI& u8g2_ref;

  unsigned long lastToggleMS = 0;
  const unsigned long blinkInterval = 500;
   bool visible = false;
  
};

#endif