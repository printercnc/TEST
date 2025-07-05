// PrinterI2C.cpp (STM32 MASTER)
#include "PrinterI2C.h"
#include "printer_status.h"
#include <string.h>

#define STATUS_REQUEST_CODE  0xFE

PrinterI2C::PrinterI2C(I2C_HandleTypeDef *hi2c, uint16_t address)
    : m_i2cHandle(hi2c), m_slaveAddress(address << 1) {}

// --- Gửi lệnh PrinterCommand (8 bytes kiểu struct) ---
HAL_StatusTypeDef PrinterI2C::sendCommand(const PrinterCommand &cmd)
{
    uint8_t buf[sizeof(PrinterCommand)];
    memcpy(buf, &cmd, sizeof(PrinterCommand));
    return HAL_I2C_Master_Transmit(m_i2cHandle, m_slaveAddress, buf, sizeof(buf), HAL_MAX_DELAY);
}

// --- Đọc PrinterStatus từ Marlin ---
HAL_StatusTypeDef PrinterI2C::receiveStatus(PrinterStatus &status)
{
    // Gửi 1 byte lệnh yêu cầu STATUS
    uint8_t req = STATUS_REQUEST_CODE;
    HAL_StatusTypeDef res = HAL_I2C_Master_Transmit(m_i2cHandle, m_slaveAddress, &req, 1, HAL_MAX_DELAY);
    if (res != HAL_OK) return res;

    // Nhận về đúng sizeof(PrinterStatus)
    uint8_t buf[sizeof(PrinterStatus)];
    res = HAL_I2C_Master_Receive(m_i2cHandle, m_slaveAddress, buf, sizeof(buf), HAL_MAX_DELAY);
    if (res == HAL_OK)
        memcpy(&status, buf, sizeof(PrinterStatus));
    return res;
}