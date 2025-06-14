#include "encoder.h"
#include "stm32f1xx_hal.h"
#include "main.h" // nếu có khai báo My_Error_Handler

extern void My_Error_Handler(void);
extern TIM_HandleTypeDef htim2;  // Biến định nghĩa ở main.c hoặc main.cpp

Encoder_t encoderY = {
    .htim = {0}, 
    .Instance = TIM2,
    .lastCount = 0,
    .totalCount = 0
};

static void Encoder_GPIO_Config_TIM2(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    // TIM2_CH1 = PA0, TIM2_CH2=PA1
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Encoder_Init(Encoder_t *encoder, TIM_TypeDef *TIMx) {
    // Kiểm tra tham số đầu vào hợp lệ
    if (encoder == NULL || TIMx == NULL) {
        My_Error_Handler();
        return;
    }

    encoder->Instance = TIMx;

    if (TIMx == TIM2) {
        Encoder_GPIO_Config_TIM2();

        // Copy handler timer từ main
        encoder->htim = htim2;
    }
    else {
        // Xử lý timer khác nếu cần
    }

    // Cấu hình timer Encoder mode
    TIM_Encoder_InitTypeDef sConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    encoder->htim.Instance = TIMx;
    encoder->htim.Init.Prescaler = 0;
    encoder->htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    encoder->htim.Init.Period = 0xFFFF;
    encoder->htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    encoder->htim.Init.RepetitionCounter = 0;
    encoder->htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    sConfig.EncoderMode = TIM_ENCODERMODE_TI12;

    sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 0;

    sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 0;

    if (HAL_TIM_Encoder_Init(&encoder->htim, &sConfig) != HAL_OK) {
  // Xử lý lỗi đơn giản: return
       My_Error_Handler();
        return;
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(&encoder->htim, &sMasterConfig) != HAL_OK) {
        My_Error_Handler();
        return;
    }
}
