/* USER CODE BEGIN Header */
/**
  ============================================================
  CSE 2206 — Assignment 3 Part A: BME280 via SPI1 — HAL
  STM32F446RE Nucleo-64

  CubeMX Configuration:
  SPI1: Full-Duplex Master, NSS=Disabled, PSC=32, CPOL=Low, CPHA=1Edge
  PA4:  GPIO_Output (CS)
  USART2: Async, 115200, 8N1
  TIM6: PSC=8999, ARR=9999, NVIC Enable
  Clock: HCLK=180MHz

  Wiring:
  BME280 VCC → 3.3V
  BME280 GND → GND
  BME280 SCL → D13 (PA5) SCK
  BME280 SDA → D11 (PA7) MOSI
  BME280 CSB → D10 (PA4) CS
  BME280 SDO → D12 (PA6) MISO
  ============================================================
*/
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim6;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#define BME280_REG_CHIP_ID    0xD0
#define BME280_REG_RESET      0xE0
#define BME280_REG_CTRL_MEAS  0xF4
#define BME280_REG_CONFIG     0xF5
#define BME280_REG_PRESS_MSB  0xF7
#define CHIP_ID_BME280        0x60
#define CHIP_ID_BMP280        0x58

#define CS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

static uint16_t dig_T1;
static  int16_t dig_T2, dig_T3;
static uint16_t dig_P1;
static  int16_t dig_P2, dig_P3, dig_P4, dig_P5;
static  int16_t dig_P6, dig_P7, dig_P8, dig_P9;
static  int32_t t_fine;

static volatile uint32_t ticks   = 0;
static volatile uint8_t  do_read = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM6_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* ── SPI helpers ─────────────────────────────────────────── */
static uint8_t SPI_ReadByte(uint8_t reg)
{
    uint8_t tx = reg | 0x80;
    uint8_t rx = 0;
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &tx, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive (&hspi1, &rx, 1, HAL_MAX_DELAY);
    CS_HIGH();
    return rx;
}

static void SPI_WriteByte(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = { reg & 0x7F, data };
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
    CS_HIGH();
}

static void SPI_ReadBurst(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t tx = reg | 0x80;
    CS_LOW();
    HAL_SPI_Transmit(&hspi1, &tx, 1,   HAL_MAX_DELAY);
    HAL_SPI_Receive (&hspi1, buf, len, HAL_MAX_DELAY);
    CS_HIGH();
}

/* ── Calibration ─────────────────────────────────────────── */
static void Load_Calibration(void)
{
    uint8_t c[24];
    SPI_ReadBurst(0x88, c, 24);
    dig_T1 = (uint16_t)(c[1]  << 8 | c[0]);
    dig_T2 =  (int16_t)(c[3]  << 8 | c[2]);
    dig_T3 =  (int16_t)(c[5]  << 8 | c[4]);
    dig_P1 = (uint16_t)(c[7]  << 8 | c[6]);
    dig_P2 =  (int16_t)(c[9]  << 8 | c[8]);
    dig_P3 =  (int16_t)(c[11] << 8 | c[10]);
    dig_P4 =  (int16_t)(c[13] << 8 | c[12]);
    dig_P5 =  (int16_t)(c[15] << 8 | c[14]);
    dig_P6 =  (int16_t)(c[17] << 8 | c[16]);
    dig_P7 =  (int16_t)(c[19] << 8 | c[18]);
    dig_P8 =  (int16_t)(c[21] << 8 | c[20]);
    dig_P9 =  (int16_t)(c[23] << 8 | c[22]);
}

/* ── Compensation ────────────────────────────────────────── */
static int32_t Compensate_Temperature(int32_t adc_T)
{
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1)))
             * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1))
             * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12)
             * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;
}

static uint32_t Compensate_Pressure(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8)
         + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
    if (var1 == 0) return 0;
    p    = 1048576 - adc_P;
    p    = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p    = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (uint32_t)p / 256;
}

/* ── Read sensor ─────────────────────────────────────────── */
static void Read_TempPressure(int32_t *temp_c100, uint32_t *press_pa)
{
    /* Forced mode */
    SPI_WriteByte(BME280_REG_CTRL_MEAS, 0x57);
    uint32_t timeout = 200;
    while ((SPI_ReadByte(0xF3) & 0x08) && --timeout)
        HAL_Delay(1);

    uint8_t raw[6];
    SPI_ReadBurst(BME280_REG_PRESS_MSB, raw, 6);

    int32_t adc_P = ((int32_t)raw[0] << 12)
                  | ((int32_t)raw[1] <<  4)
                  | ((int32_t)raw[2] >>  4);
    int32_t adc_T = ((int32_t)raw[3] << 12)
                  | ((int32_t)raw[4] <<  4)
                  | ((int32_t)raw[5] >>  4);

    *temp_c100 = Compensate_Temperature(adc_T);
    *press_pa  = Compensate_Pressure(adc_P);
}

/* ── TIM6 Callback — 1 second ───────────────────────────── */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        ticks++;
        do_read = 1;
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_TIM6_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  char msg[128];

  /* CS HIGH */
  CS_HIGH();
  HAL_Delay(100);

  /* Test A2: UART */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"[A2] UART OK\r\n", 14, 1000);

  /* Banner */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"========================================\r\n", 42, 1000);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"BME280 via SPI1 HAL -- CSE 2206 Lab A\r\n", 40, 1000);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"========================================\r\n", 42, 1000);

  /* Test A1: Chip ID */
  uint8_t chip_id = SPI_ReadByte(BME280_REG_CHIP_ID);
  sprintf(msg, "[A1] ChipID=0x%02X (expect 0x60)\r\n", chip_id);
  HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 1000);

  if (chip_id == CHIP_ID_BME280)
      HAL_UART_Transmit(&huart2,
          (uint8_t*)"BME280 detected.\r\n", 18, 1000);
  else if (chip_id == CHIP_ID_BMP280)
      HAL_UART_Transmit(&huart2,
          (uint8_t*)"BMP280 detected.\r\n", 18, 1000);
  else {
      sprintf(msg, "ERROR: Unknown chip 0x%02X\r\n", chip_id);
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 1000);
      while(1);
  }

  /* Soft reset */
  SPI_WriteByte(BME280_REG_RESET, 0xB6);
  HAL_Delay(10);

  /* Load calibration */
  Load_Calibration();
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Calibration loaded OK.\r\n", 24, 1000);

  /* Config: filter=2x */
  SPI_WriteByte(BME280_REG_CONFIG, 0x10);

  /* Start TIM6 interrupt */
  HAL_TIM_Base_Start_IT(&htim6);

  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Starting readings...\r\n\r\n", 24, 1000);

  /* USER CODE END 2 */

  /* Infinite loop */
  while (1)
  {
    /* USER CODE BEGIN 3 */
    if (do_read)
    {
        do_read = 0;

        int32_t  temp;
        uint32_t press;
        Read_TempPressure(&temp, &press);

        int32_t  t_int  =  temp / 100;
        int32_t  t_frac = (temp < 0 ? -temp : temp) % 100;
        float    temp_F = (temp / 100.0f) * 9.0f / 5.0f + 32.0f;
        uint32_t p_int  = press / 100;
        uint32_t p_frac = press % 100;

        /* Test A3: Tick */
        sprintf(msg, "[A3] Tick:%lu\r\n", (unsigned long)ticks);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 1000);

        /* Sensor data */
        sprintf(msg,
            "[SPI-HAL] Temp:%ld.%02ldC/%.2fF Pres:%lu.%02luhPa\r\n",
            (long)t_int, (long)t_frac, temp_F,
            (unsigned long)p_int, (unsigned long)p_frac);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 1000);

        /* Test A4: Plausibility */
        float temp_c = temp / 100.0f;
        float pres_h = (float)press;
        if (temp_c < 15.0f || temp_c > 40.0f
         || pres_h < 90000 || pres_h > 110000)
            HAL_UART_Transmit(&huart2,
                (uint8_t*)"[A4] Plausibility FAIL\r\n", 24, 1000);
        else
            HAL_UART_Transmit(&huart2,
                (uint8_t*)"[A4] Plausibility PASS\r\n", 24, 1000);

        HAL_UART_Transmit(&huart2,
            (uint8_t*)"----------------------------------------\r\n",
            42, 1000);
    }
    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  */
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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    Error_Handler();

  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
    Error_Handler();

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                   |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    Error_Handler();
}

/**
  * @brief SPI1 Initialization
  */
static void MX_SPI1_Init(void)
{
  hspi1.Instance               = SPI1;
  hspi1.Init.Mode              = SPI_MODE_MASTER;
  hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
  hspi1.Init.NSS               = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial     = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
    Error_Handler();
}

/**
  * @brief TIM6 Initialization — 1 second interrupt
  */
static void MX_TIM6_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim6.Instance               = TIM6;
  htim6.Init.Prescaler         = 8999;
  htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim6.Init.Period            = 9999;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
    Error_Handler();

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
    Error_Handler();
}

/**
  * @brief USART2 Initialization — 115200 8N1
  */
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
  if (HAL_UART_Init(&huart2) != HAL_OK)
    Error_Handler();
}

/**
  * @brief GPIO Initialization
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* PA4 = CS — Output, initially HIGH */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  GPIO_InitStruct.Pin   = GPIO_PIN_4;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
