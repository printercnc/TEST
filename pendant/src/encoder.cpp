#include "encoder.h"

// Khai báo static nội bộ cho các timer handlers
static TIM_HandleTypeDef htimX;
static TIM_HandleTypeDef htimY;
static TIM_HandleTypeDef htimZ;
static TIM_HandleTypeDef htimA;
static TIM_HandleTypeDef htimC;
static TIM_HandleTypeDef htimE;

// Khai báo biến encoder cho các trục
Encoder_t encoderX = {htimX, TIM3, 0, 0};
Encoder_t encoderY = {htimY, TIM2, 0, 0};
Encoder_t encoderZ = {htimZ, TIM4, 0, 0};
Encoder_t encoderA = {htimA, TIM1, 0, 0};
Encoder_t encoderC = {htimC, TIM5, 0, 0};  // TIM5 thường là 32-bit, tiện lợi
Encoder_t encoderE = {htimE, TIM8, 0, 0};  // TIM8 nếu có, hoặc bạn có thể đổi

// Hàm private: Cấu hình GPIO cho từng timer
static void Encoder_GPIO_Config(TIM_TypeDef *TIMx) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Enable clock và cấu hình GPIO theo TIMER
  if (TIMx == TIM1) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();

    // TIM1_CH1 = PA8 , TIM1_CH2=PA9
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (TIMx == TIM2) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    // TIM2_CH1=PA0, TIM2_CH2=PA1
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (TIMx == TIM3) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();

    // TIM3_CH1=PA6, TIM3_CH2=PA7
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (TIMx == TIM4) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    // TIM4_CH1=PB6, TIM4_CH2=PB7
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
  else if (TIMx == TIM5) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM5_CLK_ENABLE();

    // TIM5_CH1=PA0, TIM5_CH2=PA1  (Same as TIM2 pins)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (TIMx == TIM8) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_TIM8_CLK_ENABLE();

    // TIM8_CH1=PC6 TIM8_CH2=PC7
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  }
  else {
    // Nếu timer không đáp ứng hoặc không có pin theo yêu cầu
    // Bạn có thể thêm hoặc báo lỗi tùy nhu cầu
  }
}

void Encoder_Init(Encoder_t *encoder, TIM_TypeDef *TIMx) {
  encoder->Instance = TIMx;

  // Cấu hình GPIO
  Encoder_GPIO_Config(TIMx);

  // Cấu hình timer encoder mode
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
    // Xử lý lỗi tùy ý bạn
    while (1);
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&encoder->htim, &sMasterConfig) != HAL_OK) {
    while (1);
  }

  // Reset bộ đếm
  __HAL_TIM_SET_COUNTER(&encoder->htim, 0);

  // Khởi động encoder timer
  HAL_TIM_Encoder_Start(&encoder->htim, TIM_CHANNEL_ALL);

  // Reset giá trị đếm ảo
  encoder->lastCount = 0;
  encoder->totalCount = 0;
}

void Encoder_Start(Encoder_t *encoder) {
  HAL_TIM_Encoder_Start(&encoder->htim, TIM_CHANNEL_ALL);
}

int32_t Encoder_Read(Encoder_t *encoder) {
  int16_t current = (int16_t) __HAL_TIM_GET_COUNTER(&encoder->htim);
  int16_t diff = current - encoder->lastCount;
  encoder->lastCount = current;
  encoder->totalCount += diff;
  return encoder->totalCount;
}

void Encoders_GPIO_Clock_Init(void) {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  __HAL_RCC_TIM1_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_TIM4_CLK_ENABLE();
  __HAL_RCC_TIM5_CLK_ENABLE();
  __HAL_RCC_TIM8_CLK_ENABLE();
}

void Encoders_Init_All(void) {
  Encoders_GPIO_Clock_Init();

  Encoder_Init(&encoderX, TIM3);
  Encoder_Init(&encoderY, TIM2);
  Encoder_Init(&encoderZ, TIM4);
  Encoder_Init(&encoderA, TIM1);
  Encoder_Init(&encoderC, TIM5);
  Encoder_Init(&encoderE, TIM8);
}