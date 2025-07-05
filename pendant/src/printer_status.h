// printer_status.h

#ifndef PRINTER_STATUS_H
#define PRINTER_STATUS_H
#pragma once
#include <stdint.h>

#define CMD_NONE    0x00
#define CMD_HOME    0x01
#define CMD_PAUSE   0x02
#define CMD_RESUME  0x03
#define CMD_STOP    0x04

#define AXIS_COUNT 6
#define EXTRUDERS 2

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
typedef struct {
    float position[AXIS_COUNT];          // 24 bytes
    float hotend_temps[EXTRUDERS];       // 8 bytes
    float hotend_targets[EXTRUDERS];     // 8 bytes
    float bed_temp;                      // 4 bytes
    float bed_target;                    // 4 bytes
    uint8_t state;                       // 1 byte
    uint32_t elapsed_seconds;            // 4 bytes
    uint8_t feedrate_percentage;         // 1 byte
    uint8_t reserved[6];                 // 6 bytes padding (tùy chỉnh để tổng thành 56 hoặc 64 bytes nếu cần)
} PrinterStatus;

/** 
 * Struct lệnh gửi từ STM32 -> Marlin 
 * Ví dụ lệnh Home, Pause, Resume, Stop, ...
 */
typedef struct {
    uint8_t command_id;  // Loại lệnh, ví dụ CMD_HOME
    uint8_t args[7];     // Dữ liệu mở rộng (padding tổng đủ 8 bytes cho dễ giao tiếp và căn hàng)
} PrinterCommand;
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // PRINTER_STATUS_H