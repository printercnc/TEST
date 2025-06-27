#include "EncoderButtonPageSwitcher.h"

void EncoderButtonPageSwitcher::resetPage(uint8_t page) {
  _currentPage = page % _pageCount;
  _pageChangedFlag = false;
}

EncoderButtonPageSwitcher::EncoderButtonPageSwitcher(uint8_t pin, uint8_t pageCount, unsigned long debounceDelay)
  : _pin(pin), _pageCount(pageCount), _debounceDelay(debounceDelay),
    _currentPage(0), _lastButtonState(HIGH), _lastDebounceTime(0), _pageChangedFlag(false)
{
  pinMode(_pin, INPUT_PULLUP);
}

bool EncoderButtonPageSwitcher::_readButton() {
  return digitalRead(_pin);
}

void EncoderButtonPageSwitcher::update(bool dataValid) {
  if (dataValid) return;  // chỉ xử lý khi chưa kết nối

  bool reading = _readButton();

  if (reading != _lastButtonState) {
    _lastDebounceTime = millis();
  }

  if ((millis() - _lastDebounceTime) > _debounceDelay) {
    if (reading == LOW && _lastButtonState == HIGH) {
      _currentPage = (_currentPage + 1) % _pageCount;
      _pageChangedFlag = true;
    }
  } 

  _lastButtonState = reading;
}

bool EncoderButtonPageSwitcher::pageChanged() {
  bool flag = _pageChangedFlag;
  _pageChangedFlag = false;
  return flag;
}

uint8_t EncoderButtonPageSwitcher::getCurrentPage() const {
  return _currentPage;
}

