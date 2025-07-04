#ifndef ENCODER_H
#define ENCODER_H

#include "stm32f1xx_hal.h"

// Struct chứa thông tin Encoder chạy trên timer STM32
typedef struct {
    TIM_HandleTypeDef htim;    // Handle timer của HAL
    TIM_TypeDef *Instance;     // Timer Instance, ví dụ TIM2, TIM3...
    int16_t lastCount;         // Giá trị bộ đếm cuối cùng
    int32_t totalCount;        // Tổng giá trị đếm, bao gồm vượt ngưỡng
} Encoder_t;

extern Encoder_t encoderY;

void Encoder_Init(Encoder_t *encoder, TIM_TypeDef *TIMx);

#endif // ENCODER_H