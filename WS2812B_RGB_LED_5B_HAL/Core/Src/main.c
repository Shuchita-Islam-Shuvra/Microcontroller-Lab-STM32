/* USER CODE BEGIN Header */
/**
 * ============================================================
 * CSE 2206 — Microcontroller & Embedded System Lab-02
 * Task 5: WS2812B RGB LED — HAL Version (FIXED)
 * Platform : STM32F446RE Nucleo-64
 * ============================================================
 */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
#define WS_ARR 225U
#define WS_T1H 150U
#define WS_T0H 75U
#define WS_RESET 50U
#define NUM_LEDS 5U
#define PWM_BUF_SIZE ((NUM_LEDS * 24U) + WS_RESET)

static uint16_t pwmData[PWM_BUF_SIZE];

typedef struct { uint8_t r, g, b; } LED_t;
static LED_t g_leds[NUM_LEDS];
static volatile uint8_t txDone = 0;
/* USER CODE END PV */

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN 0 */

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
 if (htim->Instance == TIM1)
 {
 HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);

 /* Force PA8 LOW after transfer */
 GPIOA->BSRR = (1U << (8U + 16U)); /* Reset PA8 → LOW */

 txDone = 1;
 }
}

static void WS2812B_Send(void)
{
 uint32_t idx = 0;

 for (uint32_t led = 0; led < NUM_LEDS; led++)
 {
 uint32_t color = ((uint32_t)g_leds[led].g << 16)
 | ((uint32_t)g_leds[led].r << 8)
 | (uint32_t)g_leds[led].b;

 for (int bit = 23; bit >= 0; bit--)
 pwmData[idx++] = (color & (1U << bit)) ? WS_T1H : WS_T0H;
 }

 /* Reset pulse — 50 zero entries */
 for (uint32_t i = 0; i < WS_RESET; i++)
 pwmData[idx++] = 0U;

 txDone = 0;
 HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1,
 (uint32_t *)pwmData, idx);

 while (!txDone) {}
}

static void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b)
{
 for (uint32_t i = 0; i < NUM_LEDS; i++)
 g_leds[i] = (LED_t){r, g, b};
 WS2812B_Send();
}

static void WS2812_SetHue(uint16_t H)
{
 H = H % 360U;
 uint8_t seg = (uint8_t)(H / 60U);
 uint8_t frac = (uint8_t)(H % 60U);
 uint8_t q = (uint8_t)(255U * (60U - frac) / 60U);
 uint8_t t = (uint8_t)(255U * frac / 60U);
 uint8_t r, g, b;

 switch (seg) {
 case 0: r=255; g=t; b=0; break;
 case 1: r=q; g=255; b=0; break;
 case 2: r=0; g=255; b=t; break;
 case 3: r=0; g=q; b=255; break;
 case 4: r=t; g=0; b=255; break;
 case 5: r=255; g=0; b=q; break;
 default: r=0; g=0; b=0; break;
 }
 WS2812_SetAll(r, g, b);
}

typedef struct { const char *name; uint8_t r, g, b; } Colour_t;
static const Colour_t palette[] = {
 {"Red", 255, 0, 0},
 {"Green", 0, 255, 0},
 {"Blue", 0, 0, 255},
 {"Yellow", 255, 255, 0},
 {"Cyan", 0, 255, 255},
 {"Magenta", 255, 0, 255},
 {"White", 255, 255, 255},
 {"Warm White", 255, 200, 80},
 {"DU Blue", 31, 56, 100},
 {"Off", 0, 0, 0},
};
#define PALETTE_COUNT (sizeof(palette)/sizeof(palette[0]))

/* USER CODE END 0 */

int main(void)
{
 HAL_Init();
 SystemClock_Config();
 MX_GPIO_Init();
 MX_DMA_Init();
 MX_TIM1_Init();
 MX_USART2_UART_Init();

 /* USER CODE BEGIN 2 */
 char buf[128];

 /* Force PA8 LOW at startup */
 GPIOA->BSRR = (1U << (8U + 16U));

 HAL_UART_Transmit(&huart2,
 (uint8_t*)"\r\n===== Lab-02 Task 5: WS2812B RGB LED: Color Mixing & Animation (HAL) =====\r\n", 88, 1000);

 WS2812_SetAll(0, 0, 0);
 HAL_Delay(200);

 /* ── Req 1: Colour Palette ── */
 HAL_UART_Transmit(&huart2,
 (uint8_t*)"\r\n[Req 1] Colour palette - 1s per colour\r\n", 42, 1000);

 for (uint32_t i = 0; i < PALETTE_COUNT; i++)
 {
 WS2812_SetAll(palette[i].r, palette[i].g, palette[i].b);

 snprintf(buf, sizeof(buf),
 "Colour: %-12s R=%3u G=%3u B=%3u GRB=[%02X %02X %02X]\r\n",
 palette[i].name,
 palette[i].r, palette[i].g, palette[i].b,
 palette[i].g, palette[i].r, palette[i].b);
 HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 1000);

 HAL_Delay(1000);
 }

 /* ── Req 2: Hue Sweep ── */
 HAL_UART_Transmit(&huart2,
 (uint8_t*)"\r\n[Req 2] Hue sweep (0-359, step 3, 25ms)\r\n", 43, 1000);

 for (uint16_t h = 0; h < 360U; h += 3U)
 {
 WS2812_SetHue(h);
 HAL_Delay(25);
 }
 WS2812_SetAll(0, 0, 0);
 HAL_UART_Transmit(&huart2,
 (uint8_t*)"Hue sweep complete.\r\n", 21, 1000);

 /* ── Req 3: Colour Chase ── */
 HAL_UART_Transmit(&huart2,
 (uint8_t*)"\r\n[Req 3] Colour chase - 4 LEDs, 10 rounds\r\n", 44, 1000);

 for (int round = 0; round < 10; round++)
 {
 for (uint32_t active = 0; active < NUM_LEDS; active++)
 {
 for (uint32_t k = 0; k < NUM_LEDS; k++)
 g_leds[k] = (LED_t){0, 0, 0};

 g_leds[active] = (LED_t){255, 0, 0};
 WS2812B_Send();

 snprintf(buf, sizeof(buf),
 " Round %d - Active LED: %lu/%u\r\n",
 round + 1, (unsigned long)(active + 1U), (unsigned)NUM_LEDS);
 HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 1000);

 HAL_Delay(200);
 }
 }

 WS2812_SetAll(0, 0, 0);
 HAL_UART_Transmit(&huart2,
 (uint8_t*)"\r\n===== Task 5: WS2812B RGB LED: Color Mixing & Animation (HAL) Complete =====\r\n", 93, 1000);

 /* USER CODE END 2 */

 while (1) {}
}

void SystemClock_Config(void)
{
 RCC_OscInitTypeDef RCC_OscInitStruct = {0};
 RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

 __HAL_RCC_PWR_CLK_ENABLE();
 __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

 RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
 RCC_OscInitStruct.HSIState = RCC_HSI_ON;
 RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
 RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
 RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
 RCC_OscInitStruct.PLL.PLLM = 8;
 RCC_OscInitStruct.PLL.PLLN = 180;
 RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
 RCC_OscInitStruct.PLL.PLLQ = 2;
 RCC_OscInitStruct.PLL.PLLR = 2;
 if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
 if (HAL_PWREx_EnableOverDrive() != HAL_OK) Error_Handler();

 RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
 | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
 RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
 RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
 RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
 RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
 if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) Error_Handler();
}

static void MX_TIM1_Init(void)
{
 TIM_ClockConfigTypeDef sClockSourceConfig = {0};
 TIM_MasterConfigTypeDef sMasterConfig = {0};
 TIM_OC_InitTypeDef sConfigOC = {0};
 TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

 htim1.Instance = TIM1;
 htim1.Init.Prescaler = 0;
 htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
 htim1.Init.Period = WS_ARR;
 htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
 htim1.Init.RepetitionCounter = 0;
 htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
 if (HAL_TIM_Base_Init(&htim1) != HAL_OK) Error_Handler();

 sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
 if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) Error_Handler();
 if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) Error_Handler();

 sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
 sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
 if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) Error_Handler();

 sConfigOC.OCMode = TIM_OCMODE_PWM1;
 sConfigOC.Pulse = 0;
 sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
 sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
 sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
 sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET; /* ← PA8 LOW when idle */
 sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
 if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) Error_Handler();

 sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
 sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
 sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
 sBreakDeadTimeConfig.DeadTime = 0;
 sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
 sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
 sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
 if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK) Error_Handler();

 HAL_TIM_MspPostInit(&htim1);
}

static void MX_DMA_Init(void)
{
 __HAL_RCC_DMA2_CLK_ENABLE();

 hdma_tim1_ch1.Instance = DMA2_Stream1;
 hdma_tim1_ch1.Init.Channel = DMA_CHANNEL_6;
 hdma_tim1_ch1.Init.Direction = DMA_MEMORY_TO_PERIPH;
 hdma_tim1_ch1.Init.PeriphInc = DMA_PINC_DISABLE;
 hdma_tim1_ch1.Init.MemInc = DMA_MINC_ENABLE;
 hdma_tim1_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
 hdma_tim1_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
 hdma_tim1_ch1.Init.Mode = DMA_NORMAL;
 hdma_tim1_ch1.Init.Priority = DMA_PRIORITY_HIGH;
 hdma_tim1_ch1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
 if (HAL_DMA_Init(&hdma_tim1_ch1) != HAL_OK) Error_Handler();

 __HAL_LINKDMA(&htim1, hdma[TIM_DMA_ID_CC1], hdma_tim1_ch1);

 HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
 HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
}

static void MX_USART2_UART_Init(void)
{
 huart2.Instance = USART2;
 huart2.Init.BaudRate = 115200;
 huart2.Init.WordLength = UART_WORDLENGTH_8B;
 huart2.Init.StopBits = UART_STOPBITS_1;
 huart2.Init.Parity = UART_PARITY_NONE;
 huart2.Init.Mode = UART_MODE_TX_RX;
 huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
 huart2.Init.OverSampling = UART_OVERSAMPLING_16;
 if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
 __HAL_RCC_GPIOA_CLK_ENABLE();

 GPIO_InitTypeDef GPIO_InitStruct = {0};

 /* PA5: onboard LED */
 GPIO_InitStruct.Pin = GPIO_PIN_5;
 GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
 GPIO_InitStruct.Pull = GPIO_NOPULL;
 GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
 HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

 /* PA8: explicitly set LOW before TIM1 takes over */
 GPIO_InitStruct.Pin = GPIO_PIN_8;
 GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
 GPIO_InitStruct.Pull = GPIO_PULLDOWN;
 GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
 HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

void DMA2_Stream1_IRQHandler(void)
{
 HAL_DMA_IRQHandler(&hdma_tim1_ch1);
}

void Error_Handler(void)
{
 __disable_irq();
 while (1) {}
}
