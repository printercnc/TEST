#ifndef ENCODER_BUTTON_PAGE_SWITCHER_H
#define ENCODER_BUTTON_PAGE_SWITCHER_H

#include <Arduino.h>

class EncoderButtonPageSwitcher {
public:
  // pin nút, số trang, debounce delay (default 50ms)
 EncoderButtonPageSwitcher(uint8_t pin, uint8_t pageCount, unsigned long debounceDelay = 50);


  // gọi liên tục trong loop, truyền trạng thái kết nối marlin
  void update(bool dataValid);

  // kiểm tra có vừa chuyển trang không (true 1 lần mỗi lần chuyển)
  bool pageChanged();

  // lấy trang hiện tại
  uint8_t getCurrentPage() const;
  
   // Thêm khai báo hàm resetPage ở đây:
  void resetPage(uint8_t page);

private:
  uint8_t _pin;
  uint8_t _pageCount;
  unsigned long _debounceDelay;

  uint8_t _currentPage;
  bool _lastButtonState;
  unsigned long _lastDebounceTime;
  bool _pageChangedFlag;

  bool _readButton();
};

#endif