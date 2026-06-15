/* USER CODE BEGIN Header */
/**
  ============================================================
  CSE 2206 — Microcontroller & Embedded System Lab-02
  Task 6: Option B — Servo Motor Control — HAL
  Platform : STM32F446RE Nucleo-64
  TIM2 CH1 → PA0 → Servo Signal
  50 Hz PWM (20ms period)
  1ms = 0 deg, 1.5ms = 90 deg, 2ms = 180 deg

  Timer Calculation:
  fTIM2_CLK = 90 MHz
  PSC = 89   → tick = 1 us
  ARR = 19999 → period = 20000 us = 20ms = 50Hz

  CCR = pulse_us (directly!)
  0 deg   → CCR = 1000
  90 deg  → CCR = 1500
  180 deg → CCR = 2000
  ============================================================
*/
/* USER CODE END Header */

#include "main.h"
#include <string.h>
#include <stdio.h>

/* Private handles */
UART_HandleTypeDef huart2;

/* Private function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void TIM2_Servo_Init(void);

/* USER CODE BEGIN 0 */

/* ─── Servo Set Angle ───────────────────────────────────── */
/*
 * pulse_us = 1000 + (angle/180) * 1000
 * CCR = pulse_us directly (1 tick = 1 us)
 *
 * 0 deg   → 1000 us → CCR = 1000
 * 90 deg  → 1500 us → CCR = 1500
 * 180 deg → 2000 us → CCR = 2000
 */
void Servo_SetAngle(uint16_t angle)
{
    if (angle > 180U) angle = 180U;
    uint32_t ccr = 1000U + ((uint32_t)angle * 1000U / 180U);
    TIM2->CCR1 = ccr;
}

/* USER CODE END 0 */

/* =========================================================
 * main()
 * ========================================================= */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  TIM2_Servo_Init();

  /* USER CODE BEGIN 2 */

  char buf[80];

  /* Header */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"\r\n==========================================\r\n", 46, 1000);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"====== Lab-02 Task 6: Servo Motor Control (HAL) ======\r\n", 57, 1000);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"  TIM2 CH1, PA0, 50Hz, PSC=89, ARR=19999\r\n", 43, 1000);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"==========================================\r\n", 43, 1000);

  HAL_UART_Transmit(&huart2,
      (uint8_t*)"\r\nAngle  | Pulse (us) | CCR\r\n", 28, 1000);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"--------|------------|------\r\n", 29, 1000);

  /* ── Sweep: 0 to 180 in 10 degree steps ── */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"\r\n-- Sweep 0 to 180 degrees --\r\n", 31, 1000);

  for (uint16_t angle = 0; angle <= 180; angle += 10)
  {
      uint32_t pulse_us = 1000U + ((uint32_t)angle * 1000U / 180U);

      Servo_SetAngle(angle);

      snprintf(buf, sizeof(buf),
               "%3u deg  | %7lu us  | %4lu\r\n",
               angle,
               (unsigned long)pulse_us,
               (unsigned long)pulse_us);
      HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 1000);

      HAL_Delay(500);
  }

  /* ── Key positions ── */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"\r\n-- Key Positions --\r\n", 22, 1000);

  /* 0 degrees */
  Servo_SetAngle(0);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Position:   0 deg | 1000 us | CCR = 1000\r\n", 43, 1000);
  HAL_Delay(1000);

  /* 90 degrees */
  Servo_SetAngle(90);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Position:  90 deg | 1500 us | CCR = 1500\r\n", 43, 1000);
  HAL_Delay(1000);

  /* 180 degrees */
  Servo_SetAngle(180);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Position: 180 deg | 2000 us | CCR = 2000\r\n", 43, 1000);
  HAL_Delay(1000);

  /* Back to center */
  Servo_SetAngle(90);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"\r\nFinal: Back to center (90 deg)\r\n", 34, 1000);

  HAL_UART_Transmit(&huart2,
      (uint8_t*)"\r\n====== Task 6: Servo Motor Control (HAL) Complete ======\r\n", 58, 1000);

  /* USER CODE END 2 */

  /* Continuous sweep loop */
  while (1)
  {
    /* USER CODE BEGIN 3 */
    Servo_SetAngle(0);
    HAL_Delay(2000);
    Servo_SetAngle(90);
    HAL_Delay(2000);
    Servo_SetAngle(180);
    HAL_Delay(2000);
    /* USER CODE END 3 */
  }
}

/* =========================================================
 * TIM2_Servo_Init
 * Direct register + HAL GPIO
 * PA0 → AF1 (TIM2_CH1)
 * PSC=89, ARR=19999 → 50Hz, 1 tick = 1us
 * ========================================================= */
static void TIM2_Servo_Init(void)
{
    /* Enable clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* PA0 → AF1 = TIM2_CH1 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin       = GPIO_PIN_0;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* TIM2 direct register config */
    TIM2->CR1  &= ~TIM_CR1_CEN;     /* Stop timer */
    TIM2->PSC   = 89U;              /* 90MHz/90 = 1MHz = 1us tick */
    TIM2->ARR   = 19999U;           /* 20000us = 20ms = 50Hz */
    TIM2->CCR1  = 1500U;            /* Start at 90 deg center */

    /* PWM Mode 1 on CH1 */
    TIM2->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM2->CCMR1 |=  TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2; /* 110 = PWM1 */
    TIM2->CCMR1 |=  TIM_CCMR1_OC1PE;  /* Preload enable */

    /* Enable CH1 output, active high */
    TIM2->CCER |=  TIM_CCER_CC1E;
    TIM2->CCER &= ~TIM_CCER_CC1P;

    /* Auto-reload preload */
    TIM2->CR1 |= TIM_CR1_ARPE;

    /* Force update */
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR  = 0U;

    /* Start timer */
    TIM2->CR1 |= TIM_CR1_CEN;
}

/* =========================================================
 * SystemClock_Config — 180 MHz
 * ========================================================= */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM           = 8;
  RCC_OscInitStruct.PLL.PLLN           = 180;
  RCC_OscInitStruct.PLL.PLLP           = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ           = 2;
  RCC_OscInitStruct.PLL.PLLR           = 2;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  HAL_PWREx_EnableOverDrive();

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                   | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/* =========================================================
 * MX_USART2_UART_Init — 115200 8N1
 * ========================================================= */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance          = USART2;
  huart2.Init.BaudRate     = 115200;
  huart2.Init.WordLength   = UART_WORDLENGTH_8B;
  huart2.Init.StopBits     = UART_STOPBITS_1;
  huart2.Init.Parity       = UART_PARITY_NONE;
  huart2.Init.Mode         = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart2);
}

/* =========================================================
 * MX_GPIO_Init — PA5 = LD2, PA2/PA3 = USART2
 * ========================================================= */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* PA5 = LD2 output */
  GPIO_InitStruct.Pin   = GPIO_PIN_5;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
