#ifndef ENCODER_H
#define ENCODER_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

// Struct chứa thông tin Encoder chạy trên timer STM32
typedef struct {
    TIM_HandleTypeDef htim;    // Handle timer của HAL
    TIM_TypeDef *Instance;     // Timer Instance, ví dụ TIM2, TIM3...
    int16_t lastCount;         // Giá trị bộ đếm cuối cùng
    int32_t totalCount;        // Tổng giá trị đếm, bao gồm vượt ngưỡng
} Encoder_t;

// Hàm khởi tạo encoder với Timer cụ thể
void Encoder_Init(Encoder_t *encoder, TIM_TypeDef *TIMx);

// Hàm bắt đầu Encoder (nếu cần tách riêng)
void Encoder_Start(Encoder_t *encoder);

// Đọc giá trị tổng cộng bộ đếm encoder (cập nhật nội bộ)
int32_t Encoder_Read(Encoder_t *encoder);

#endif // ENCODER_H