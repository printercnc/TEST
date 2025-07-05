// PrinterI2C.cpp
#include "PrinterI2C.h"
#include "printer_status.h"
#include <string.h> // cho memcpy

PrinterI2C::PrinterI2C(I2C_HandleTypeDef *hi2c, uint16_t address)
  : m_i2cHandle(hi2c), m_slaveAddress(address << 1) {}

// Gửi PrinterCommand (8 bytes)
HAL_StatusTypeDef PrinterI2C::sendCommand(const PrinterCommand &cmd)
{
    uint8_t buf[sizeof(PrinterCommand)];
    // Copy struct vào buffer
    memcpy(buf, &cmd, sizeof(PrinterCommand));
    return HAL_I2C_Master_Transmit(m_i2cHandle, m_slaveAddress, buf, sizeof(buf), HAL_MAX_DELAY);
}

// Nhận PrinterStatus (16 bytes)
HAL_StatusTypeDef PrinterI2C::receiveStatus(PrinterStatus &status)
{
    uint8_t buf[sizeof(PrinterStatus)];
    HAL_StatusTypeDef res = HAL_I2C_Master_Receive(m_i2cHandle, m_slaveAddress, buf, sizeof(buf), HAL_MAX_DELAY);
    if (res == HAL_OK)
    {
        memcpy(&status, buf, sizeof(PrinterStatus));
    }
    return res;
}