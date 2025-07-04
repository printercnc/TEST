#include "stm32f1xx_hal.h"
#include "main.h"

TIM_HandleTypeDef htim2;

void MX_TIM2_Init(void)
{
    // Cấu hình encoder
    TIM_Encoder_InitTypeDef sConfig = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;  // Thường đặt 0 nếu dùng encoder timer
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFF; // Chu kỳ max 16-bit
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    // Cấu hình các kênh encoder
    sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
    sConfig.IC1Polarity = TIM_INPUTCHANNELPOLARITY_RISING;
    sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC1Filter = 0;

    sConfig.IC2Polarity = TIM_INPUTCHANNELPOLARITY_RISING;
    sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
    sConfig.IC2Filter = 0;

    if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
    {
        My_Error_Handler();
        return;
    }
}