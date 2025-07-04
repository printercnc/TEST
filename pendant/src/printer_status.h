#ifndef PRINTER_STATUS_H
#define PRINTER_STATUS_H

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
    float position[AXIS_COUNT];          // Vị trí trục: X, Y, Z, A, C, E
    float hotend_temps[EXTRUDERS];            // Nhiệt độ nozzle hiện tại cho mỗi extruder
    float hotend_targets[EXTRUDERS];               // Nhiệt mục tiêu nozzle cho mỗi extruder
    float bed_temp;                      // Nhiệt độ bàn hiện tại
    float bed_target;                    // Nhiệt bàn mục tiêu
    uint8_t state;                      // Trạng thái máy (ví dụ: 0=idle, 1=printing)
    uint32_t elapsed_seconds;
    uint8_t feedrate_percentage;           // Tỷ lệ cấp liệu (0-100%)
} PrinterStatus;

/** 
 * Struct lệnh gửi từ STM32 -> Marlin 
 * Ví dụ lệnh Home, Pause, Resume, Stop, ...
 */
typedef struct {
    uint8_t command_id;                   // Mã lệnh, vd: 0=none,1=home,2=pause,...
    uint8_t args[7];                     // Dữ liệu mở rộng / tham số nếu cần (padding)
} PrinterCommand;
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // PRINTER_STATUS_H