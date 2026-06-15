/* USER CODE BEGIN Header */
/**
  ============================================================
  CSE 2206 — Microcontroller & Embedded System Lab-02
  Task 4: PWM Generation and LED Brightness Control — HAL
  Platform : STM32F446RE Nucleo-64
  TIM3 CH1 → PA6 → PWM @ 1 kHz
  ============================================================
*/
/* USER CODE END Header */

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ── HAL handles ─────────────────────────────────────────── */
UART_HandleTypeDef huart2;
TIM_HandleTypeDef  htim3;   /* TIM3 CH1 → PA6 → PWM 1 kHz */

/* ── Private prototypes ──────────────────────────────────── */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);

/* ── Sine LUT — 256 entries, values 0..100 ───────────────── */
static uint8_t sine_lut[256];

static void SineLUT_Init(void)
{
    for (uint16_t i = 0; i < 256U; i++)
    {
        float val = 50.0f * (1.0f + sinf(2.0f * (float)M_PI * (float)i / 256.0f));
        sine_lut[i] = (uint8_t)(val + 0.5f);
    }
}

/* ── PWM duty helper ─────────────────────────────────────── */
/* ARR = 999 → CCR = pct × 10  (0%=0, 100%=1000) */
static void PWM_SetDuty(uint8_t pct)
{
    if (pct > 100U) pct = 100U;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (uint32_t)pct * 10U);
}

/* ── UART helper ─────────────────────────────────────────── */
static void UART_Print(const char *s)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)s, (uint16_t)strlen(s), 1000U);
}


int main(void)
{

    SCB->CPACR |= (0xFU << 20U);
    __DSB();
    __ISB();

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM3_Init();

    SineLUT_Init();


    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    char buf[80];

    /* ── Header ───────────────────────────────────────────── */
    UART_Print("\r\n======================================\r\n");
    UART_Print("  Lab-02 Task 4: PWM Generation & LED Brightness Control (HAL) \r\n");
    UART_Print("  PA6, 1 kHz, PSC=89, ARR=999\r\n");
    UART_Print("======================================\r\n");

    /* ── Part 1: Duty-Cycle Sweep 0% to 100% ─────────────── */
    UART_Print("\r\n-- Part 1: Duty-Cycle Sweep --\r\n");
    UART_Print("Duty    | CCR1\r\n");
    UART_Print("--------|------\r\n");

    for (uint8_t pct = 0; pct <= 100U; pct += 10U)
    {
        PWM_SetDuty(pct);

        uint32_t ccr1 = (uint32_t)pct * 10U;
        snprintf(buf, sizeof(buf),
                 "Duty = %3u%%  |  CCR1 = %4lu\r\n",
                 pct, (unsigned long)ccr1);
        UART_Print(buf);

        HAL_Delay(300U);
    }

    /* ── Part 2: Sine Breathing — 5 cycles ───────────────── */
    UART_Print("\r\n-- Part 2: Sine Breathing (5 cycles) --\r\n");

    for (uint8_t cycle = 1U; cycle <= 5U; cycle++)
    {
        snprintf(buf, sizeof(buf), "Breath cycle %u/5\r\n", cycle);
        UART_Print(buf);

        for (uint16_t i = 0U; i < 256U; i++)
        {
            PWM_SetDuty(sine_lut[i]);
            HAL_Delay(8U);   /* 256 × 8ms = 2048ms per cycle */
        }
    }

    /* ── Part 3: Final hold at 50% ───────────────────────── */
    PWM_SetDuty(50U);
    snprintf(buf, sizeof(buf),
             "\r\n-- Final Hold --\r\nDuty = 50%%   CCR1 = 500\r\n");
    UART_Print(buf);

    UART_Print("\r\n====== Task 4: PWM Generation & LED Brightness Control (HAL) Complete ======\r\n");

    while (1) {}
}

/* =========================================================
 * SystemClock_Config — 180 MHz
 * HSI(16) / PLLM(8) x PLLN(180) / PLLP(2) = 180 MHz
 * APB1 = 45 MHz → TIM3 clock = 90 MHz
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
    RCC_OscInitStruct.PLL.PLLM           = 8U;
    RCC_OscInitStruct.PLL.PLLN           = 180U;
    RCC_OscInitStruct.PLL.PLLP           = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ           = 2U;
    RCC_OscInitStruct.PLL.PLLR           = 2U;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    if (HAL_PWREx_EnableOverDrive() != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;   /* HCLK  = 180 MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;     /* PCLK1 =  45 MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;     /* PCLK2 =  90 MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
        Error_Handler();
}


static void MX_TIM3_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = 89U;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = 999U;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) Error_Handler();

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0U;             /* শুরুতে 0% duty */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
        Error_Handler();
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (htim->Instance == TIM3)
    {

        __HAL_RCC_TIM3_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA6 → TIM3_CH1 (AF2) */
        GPIO_InitStruct.Pin       = GPIO_PIN_6;        /* PA6 */
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;   /* Push-pull AF */
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;     /* AF2 = TIM3 */
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

/* =========================================================
 * MX_USART2_UART_Init — 115200 8N1
 * PA2 = TX (AF7), PA3 = RX (AF7)
 * ========================================================= */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200U;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /* PA6 configured in HAL_TIM_PWM_MspInit */
}

/* =========================================================
 * Error_Handler
 * ========================================================= */
void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
