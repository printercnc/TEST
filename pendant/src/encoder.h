#ifndef ENCODER_H
#define ENCODER_H

#include "stm32f1xx_hal.h"

// Định nghĩa cấu trúc cho mỗi encoder trục
typedef struct {
  TIM_HandleTypeDef htim;  // Handler timer
  TIM_TypeDef *Instance;   // TIMx instance
  int32_t lastCount;       // Giá trị đếm trước đó (để xử lý overflow)
  int32_t totalCount;      // Tổng số bước đã đếm
} Encoder_t;

// Khai báo các encoder trục
extern Encoder_t encoderX;
extern Encoder_t encoderY;
extern Encoder_t encoderZ;
extern Encoder_t encoderA;
extern Encoder_t encoderC;
extern Encoder_t encoderE;

// Hàm khởi tạo timer và encoder mode
void Encoder_Init(Encoder_t *encoder, TIM_TypeDef *TIMx);

// Bắt encoder (khởi động timer ở encoder mode)
void Encoder_Start(Encoder_t *encoder);

// Đọc giá trị đã đếm, xử lý overflow, trả về tổng bước
int32_t Encoder_Read(Encoder_t *encoder);

// Hàm khởi tạo Clock và GPIO cho tất cả encoder
void Encoders_GPIO_Clock_Init(void);

// Khởi tạo tất cả encoder
void Encoders_Init_All(void);

#endif