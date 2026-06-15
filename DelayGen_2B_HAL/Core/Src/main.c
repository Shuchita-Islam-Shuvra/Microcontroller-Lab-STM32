/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * CSE 2206 — Microcontroller & Embedded System Lab-02
  * Task 2: Delay Generation (ms, s, Hours) — HAL
  * Platform : STM32F446RE Nucleo-64
  * Clock    : SYSCLK = 180 MHz, APB1 = 45 MHz, TIM6_CLK = 90 MHz
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include <string.h>
#include <stdio.h>

/* Private handles */
TIM_HandleTypeDef htim6;
UART_HandleTypeDef huart2;

/* Private function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN 0 */

/* ─── Delay Functions ───*/

void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim6, 0);
    while(__HAL_TIM_GET_COUNTER(&htim6) < us) {}
}

void delay_ms(uint32_t ms)
{
    for(uint32_t i = 0; i < ms; i++)
        delay_us(1000);
}

void delay_s(uint32_t sec)
{
    for(uint32_t i = 0; i < sec; i++)
        delay_ms(1000);
}

void delay_hms(uint8_t h, uint8_t m, uint8_t s)
{
    uint32_t total = (uint32_t)h * 3600U
                   + (uint32_t)m * 60U
                   + (uint32_t)s;
    delay_s(total);
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
  MX_TIM6_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  /* TIM6 চালু করো */
  HAL_TIM_Base_Start(&htim6);

  char buf[80];

  /* Header */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"\r\n===== Lab-02 Task 2: Delay Generation (HAL) =====\r\n",
      58, 1000);

  /* ── Demo 1: delay_ms(500) ── */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Starting 500ms delay...\r\n", 25, 1000);
  delay_ms(500);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Done. [500ms elapsed]\r\n\r\n", 25, 1000);

  /* ── Demo 2: delay_ms(1000) ── */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Starting 1000ms delay...\r\n", 26, 1000);
  delay_ms(1000);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Done. [1000ms elapsed]\r\n\r\n", 26, 1000);

  /* ── Demo 3: LED toggle 10 times ── */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"LED Toggle Demo: 10 x (250ms ON / 250ms OFF)\r\n", 47, 1000);

  for(int i = 0; i < 5; i++)
  {
      /* LED ON */
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
      snprintf(buf, sizeof(buf), "  Toggle %2d/10: LED ON\r\n", i+1);
      HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 1000);
      delay_ms(250);

      /* LED OFF */
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
      snprintf(buf, sizeof(buf), "  Toggle %2d/10: LED OFF\r\n", i+1);
      HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 1000);
      delay_ms(250);
  }
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"LED Toggle Done.\r\n\r\n", 20, 1000);

  /* ── Demo 4: delay_s(3) ── */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Starting 3s delay...\r\n", 22, 1000);
  delay_s(3);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Done. [3s elapsed]\r\n\r\n", 22, 1000);

  /* ── Demo 5: delay_hms(0,0,5) ── */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Starting delay_hms(0,0,5) [5 seconds]...\r\n", 43, 1000);
  delay_hms(0, 0, 5);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Done. [5s elapsed]\r\n\r\n", 22, 1000);

  HAL_UART_Transmit(&huart2,
      (uint8_t*)"===== Task 2: Delay Generation (HAL) Complete =====\r\n", 60, 1000);

  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
  }
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
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    Error_Handler();

  if(HAL_PWREx_EnableOverDrive() != HAL_OK)
    Error_Handler();

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                   | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    Error_Handler();
}

/* =========================================================
 * MX_TIM6_Init — 1us tick
 * PSC=89, ARR=65535
 * ========================================================= */
static void MX_TIM6_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim6.Instance               = TIM6;
  htim6.Init.Prescaler         = 89;
  htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim6.Init.Period            = 65535;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if(HAL_TIM_Base_Init(&htim6) != HAL_OK)
    Error_Handler();

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
  if(HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
    Error_Handler();
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
  if(HAL_UART_Init(&huart2) != HAL_OK)
    Error_Handler();
}

/* =========================================================
 * MX_GPIO_Init — PA5 = LD2
 * ========================================================= */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

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

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
