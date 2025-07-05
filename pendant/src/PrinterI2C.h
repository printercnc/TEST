// PrinterI2C.h
#pragma once
#include "stm32f1xx_hal.h"  // hoặc file HAL tương ứng
#include "printer_status.h" 

class PrinterI2C {
public:
  PrinterI2C(I2C_HandleTypeDef *hi2c, uint16_t address);
  HAL_StatusTypeDef sendCommand(const PrinterCommand &cmd);
  HAL_StatusTypeDef receiveStatus(PrinterStatus &status);

private:
  I2C_HandleTypeDef *m_i2cHandle;
  uint16_t m_slaveAddress;
};