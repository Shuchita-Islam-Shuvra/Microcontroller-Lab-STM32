/* USER CODE BEGIN Header */
/**
  ============================================================
  CSE 2206 — Assignment 3 Part B: BMP280 via I2C — HAL
  STM32F446RE Nucleo-64

  CubeMX Configuration:
  I2C1: Standard Mode 100kHz, PB6=SCL, PB7=SDA
  USART2: Async, 115200, 8N1
  TIM6: PSC=8999, ARR=9999, NVIC Enable
  Clock: HCLK=180MHz

  Wiring:
  BMP280 VCC → 3.3V
  BMP280 GND → GND
  BMP280 SCL → PB6 (D10)
  BMP280 SDA → PB7 (CN7-21)
  BMP280 CSB → 3.3V  (I2C mode)
  BMP280 SDO → GND   (address 0x76)
  ============================================================
*/
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
/* USER CODE END Includes */

/* Private variables */
I2C_HandleTypeDef  hi2c1;
TIM_HandleTypeDef  htim6;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#define BMP280_I2C_ADDR      (0x76 << 1)  /* HAL uses 8-bit address */
#define BMP280_REG_CHIP_ID    0xD0
#define BMP280_REG_RESET      0xE0
#define BMP280_REG_STATUS     0xF3
#define BMP280_REG_CTRL_MEAS  0xF4
#define BMP280_REG_CONFIG     0xF5
#define BMP280_REG_PRESS_MSB  0xF7
#define CHIP_ID_BME280        0x60
#define CHIP_ID_BMP280        0x58

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5;
    int16_t  dig_P6, dig_P7, dig_P8, dig_P9;
} BMP280_Calib_t;

static BMP280_Calib_t calib;
static int32_t t_fine;
static volatile uint32_t ticks   = 0;
static volatile uint8_t  do_read = 0;
/* USER CODE END PV */

/* Private function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM6_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN 0 */

/* ── I2C helpers using HAL ──────────────────────────────── */
static void I2C_WriteByte(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = { reg, data };
    HAL_I2C_Master_Transmit(&hi2c1, BMP280_I2C_ADDR,
                             buf, 2, HAL_MAX_DELAY);
}

static uint8_t I2C_ReadByte(uint8_t reg)
{
    uint8_t val = 0;
    HAL_I2C_Master_Transmit(&hi2c1, BMP280_I2C_ADDR,
                             &reg, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive (&hi2c1, BMP280_I2C_ADDR,
                             &val, 1, HAL_MAX_DELAY);
    return val;
}

static void I2C_ReadBytes(uint8_t reg, uint8_t *buf, uint8_t len)
{
    HAL_I2C_Master_Transmit(&hi2c1, BMP280_I2C_ADDR,
                             &reg, 1, HAL_MAX_DELAY);
    HAL_I2C_Master_Receive (&hi2c1, BMP280_I2C_ADDR,
                             buf, len, HAL_MAX_DELAY);
}

/* ── Calibration ─────────────────────────────────────────── */
static void BMP280_ReadCalib(void)
{
    uint8_t raw[24];
    I2C_ReadBytes(0x88, raw, 24);

    calib.dig_T1 = (uint16_t)(raw[1]  << 8 | raw[0]);
    calib.dig_T2 =  (int16_t)(raw[3]  << 8 | raw[2]);
    calib.dig_T3 =  (int16_t)(raw[5]  << 8 | raw[4]);
    calib.dig_P1 = (uint16_t)(raw[7]  << 8 | raw[6]);
    calib.dig_P2 =  (int16_t)(raw[9]  << 8 | raw[8]);
    calib.dig_P3 =  (int16_t)(raw[11] << 8 | raw[10]);
    calib.dig_P4 =  (int16_t)(raw[13] << 8 | raw[12]);
    calib.dig_P5 =  (int16_t)(raw[15] << 8 | raw[14]);
    calib.dig_P6 =  (int16_t)(raw[17] << 8 | raw[16]);
    calib.dig_P7 =  (int16_t)(raw[19] << 8 | raw[18]);
    calib.dig_P8 =  (int16_t)(raw[21] << 8 | raw[20]);
    calib.dig_P9 =  (int16_t)(raw[23] << 8 | raw[22]);
}

/* ── Compensation ────────────────────────────────────────── */
static int32_t BMP280_CompensateTemp(int32_t adc_T)
{
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1)))
             * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1))
             * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12)
             * ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;
}

static uint32_t BMP280_CompensatePress(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8)
         + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;
    if (var1 == 0) return 0;
    p    = 1048576 - adc_P;
    p    = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p    = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    return (uint32_t)(p >> 8);
}

/* ── Read and compensate ─────────────────────────────────── */
static void BMP280_ReadAll(int32_t *temp_c100, uint32_t *press_pa)
{
    uint8_t raw[6];
    I2C_ReadBytes(BMP280_REG_PRESS_MSB, raw, 6);

    int32_t adc_P = ((int32_t)raw[0] << 12)
                  | ((int32_t)raw[1] <<  4)
                  | ((int32_t)raw[2] >>  4);
    int32_t adc_T = ((int32_t)raw[3] << 12)
                  | ((int32_t)raw[4] <<  4)
                  | ((int32_t)raw[5] >>  4);

    *temp_c100 = BMP280_CompensateTemp(adc_T);
    *press_pa  = BMP280_CompensatePress(adc_P);
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

int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM6_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  char msg[128];
  HAL_Delay(100);

  /* UART check */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"[B2] UART OK\r\n", 14, 1000);

  /* Banner */
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"========================================\r\n", 42, 1000);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"BMP280 via I2C HAL -- CSE 2206 Lab B\r\n", 39, 1000);
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"========================================\r\n", 42, 1000);

  /* Test B1: Chip ID */
  uint8_t chip_id = I2C_ReadByte(BMP280_REG_CHIP_ID);
  sprintf(msg, "[B1] ChipID=0x%02X (expect 0x60 or 0x58)\r\n", chip_id);
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

  /* Test B2: I2C ACK */
  HAL_StatusTypeDef ack = HAL_I2C_IsDeviceReady(
      &hi2c1, BMP280_I2C_ADDR, 3, 100);
  if (ack == HAL_OK)
      HAL_UART_Transmit(&huart2,
          (uint8_t*)"[B2] I2C ACK OK\r\n", 17, 1000);
  else
      HAL_UART_Transmit(&huart2,
          (uint8_t*)"[B2] I2C ACK FAIL\r\n", 19, 1000);

  /* Soft reset */
  I2C_WriteByte(BMP280_REG_RESET, 0xB6);
  HAL_Delay(10);

  /* Calibration */
  BMP280_ReadCalib();
  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Calibration loaded OK.\r\n", 24, 1000);

  /* Config: IIR filter x16 */
  I2C_WriteByte(BMP280_REG_CONFIG, 0x90);

  /* Normal mode: osrs_t=x2, osrs_p=x16 */
  I2C_WriteByte(BMP280_REG_CTRL_MEAS, 0x57);
  HAL_Delay(100);

  /* Start TIM6 */
  HAL_TIM_Base_Start_IT(&htim6);

  HAL_UART_Transmit(&huart2,
      (uint8_t*)"Starting readings...\r\n\r\n", 24, 1000);

  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */
    if (do_read)
    {
        do_read = 0;

        int32_t  temp;
        uint32_t press;
        BMP280_ReadAll(&temp, &press);

        int32_t  t_int  =  temp / 100;
        int32_t  t_frac = (temp < 0 ? -temp : temp) % 100;
        float    temp_F = (temp / 100.0f) * 9.0f / 5.0f + 32.0f;
        uint32_t p_int  = press / 100;
        uint32_t p_frac = press % 100;

        /* Test B3: Tick */
        sprintf(msg, "[B3] Tick:%lu\r\n", (unsigned long)ticks);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 1000);

        /* Sensor data */
        sprintf(msg,
            "[I2C-HAL] Temp:%ld.%02ldC/%.2fF Pres:%lu.%02luhPa\r\n",
            (long)t_int, (long)t_frac, temp_F,
            (unsigned long)p_int, (unsigned long)p_frac);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 1000);

        /* Test B4: Plausibility */
        float temp_c = temp / 100.0f;
        if (temp_c < 15.0f || temp_c > 40.0f
         || press < 90000  || press > 110000)
            HAL_UART_Transmit(&huart2,
                (uint8_t*)"[B4] Plausibility FAIL\r\n", 24, 1000);
        else
            HAL_UART_Transmit(&huart2,
                (uint8_t*)"[B4] Plausibility PASS\r\n", 24, 1000);

        HAL_UART_Transmit(&huart2,
            (uint8_t*)"----------------------------------------\r\n",
            42, 1000);
    }
    /* USER CODE END 3 */
  }
}

/* SystemClock_Config — 180 MHz via HSI */
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

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                   |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

/* MX_I2C1_Init — PB6=SCL, PB7=SDA, 100kHz */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance             = I2C1;
  hi2c1.Init.ClockSpeed      = 100000;
  hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1     = 0;
  hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    Error_Handler();
}

/* MX_TIM6_Init — 1 second interrupt */
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
  HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig);

  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

/* MX_USART2_UART_Init — 115200 8N1 */
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

/* MX_GPIO_Init */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
}

/* TIM6 IRQ Handler */
void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim6);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
