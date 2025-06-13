#ifndef MAIN_H
#define MAIN_H

#include "stm32f1xx_hal.h"

// Khai báo hàm xử lý lỗi bạn tự định nghĩa (đổi tên để tránh trùng với macro STM32)
void My_Error_Handler(void);

// Khai báo handler Timer htim2
extern TIM_HandleTypeDef htim2;

#endif