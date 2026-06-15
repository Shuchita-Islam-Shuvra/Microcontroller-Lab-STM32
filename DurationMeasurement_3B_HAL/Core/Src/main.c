/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file : main.c
 * @brief : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE BEGIN 0 */
#include <string.h>
#include <stdio.h>

void DWT_Init(void)
{
 CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
 DWT->CYCCNT = 0U;
 DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline uint32_t DWT_GetCycles(void)
{
 return DWT->CYCCNT;
}

void Profile_Print(const char *label, uint32_t cycles, uint32_t tim2_us)
{
 char buf[160];
 uint32_t ns = (uint32_t)((uint64_t)cycles * 1000U / 180U);
 uint32_t us = ns / 1000U;
 uint32_t ms = us / 1000U;

 snprintf(buf, sizeof(buf),
 "%-30s | DWT:%8lu cyc |%7lu ns |%5lu us |%3lu ms | TIM2:%5lu us\r\n",
 label,
 (unsigned long)cycles,
 (unsigned long)ns,
 (unsigned long)us,
 (unsigned long)ms,
 (unsigned long)tim2_us);

 HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), 1000);
}

#define PROFILE(label, block) \
 do { \
 uint32_t _t0_dwt = DWT_GetCycles(); \
 uint32_t _t0_tim2 = __HAL_TIM_GET_COUNTER(&htim2); \
 { block } \
 uint32_t _t1_dwt = DWT_GetCycles(); \
 uint32_t _t1_tim2 = __HAL_TIM_GET_COUNTER(&htim2); \
 Profile_Print(label, \
 _t1_dwt - _t0_dwt, \
 _t1_tim2 - _t0_tim2); \
 } while(0)

#define SORT_N 100U
static int sort_arr[SORT_N];

void BubbleSort_PrepareWorstCase(void)
{
 for(uint32_t i = 0; i < SORT_N; i++)
 sort_arr[i] = (int)(SORT_N - i);
}

void BubbleSort(void)
{
 int temp;
 for(uint32_t i = 0; i < SORT_N - 1U; i++) {
 for(uint32_t j = 0; j < SORT_N - 1U - i; j++) {
 if(sort_arr[j] > sort_arr[j+1U]) {
 temp = sort_arr[j];
 sort_arr[j] = sort_arr[j+1U];
 sort_arr[j+1U] = temp;
 }
 }
 }
}

static uint32_t isqrt(uint32_t n)
{
 if(n == 0U) return 0U;
 uint32_t x = n;
 uint32_t y = (x + 1U) / 2U;
 while(y < x) {
 x = y;
 y = (x + n/x) / 2U;
 }
 return x;
}

static volatile uint32_t isqrt_result;

void IsqrtBenchmark(void)
{
 for(uint32_t i = 0; i < 1000U; i++)
 isqrt_result = isqrt(i * 7U + 1U);
}

static uint8_t src_buf[512U];
static uint8_t dst_buf[512U];

void MemCopy_ByteByByte(void)
{
 for(uint32_t i = 0; i < 512U; i++)
 dst_buf[i] = src_buf[i];
}

/* USER CODE END 0 */

/* USER CODE END 0 */

/**
 * @brief The application entry point.
 * @retval int
 */
int main(void)
{

 /* USER CODE BEGIN 1 */

 /* USER CODE END 1 */

 /* MCU Configuration--------------------------------------------------------*/

 /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
 HAL_Init();

 /* USER CODE BEGIN Init */

 /* USER CODE END Init */

 /* Configure the system clock */
 SystemClock_Config();

 /* USER CODE BEGIN SysInit */

 /* USER CODE END SysInit */

 /* Initialize all configured peripherals */
 MX_GPIO_Init();
 MX_TIM2_Init();
 MX_USART2_UART_Init();
 /* USER CODE BEGIN 2 */
 /* USER CODE BEGIN 2 */

 HAL_TIM_Base_Start(&htim2);
 DWT_Init();

 for(uint32_t i = 0; i < 512U; i++)
 src_buf[i] = (uint8_t)(i & 0xFF);

 HAL_UART_Transmit(&huart2,
 (uint8_t*)"\r\n===== Lab-02 Task 3: Duration Measurement & Code Profiling (HAL) =====\r\n",
 74, 1000);
 HAL_UART_Transmit(&huart2,
 (uint8_t*)"Block | DWT Cycles | ns | us | ms | TIM2 us\r\n",
 82, 1000);
 HAL_UART_Transmit(&huart2,
 (uint8_t*)"-------------------------------|------------|---------|-------|-----|--------\r\n",
 82, 1000);

 BubbleSort_PrepareWorstCase();
 PROFILE("[1] Bubble sort N=100 (worst)", BubbleSort(););

 PROFILE("[2] HAL_Delay(100)", HAL_Delay(100););

 PROFILE("[3] HAL_UART_Transmit 48B",
 HAL_UART_Transmit(&huart2,
 (uint8_t*)"PROFILING: STM32F446RE USART2 @ 115200 baud OK!",
 48, 1000);
 );

 PROFILE("[4] isqrt() x1000 inputs", IsqrtBenchmark(););

 PROFILE("[5] MemCopy byte x512", MemCopy_ByteByByte(););

 HAL_UART_Transmit(&huart2,
 (uint8_t*)"\r\n===== Task 3: Duration Measurement & Code Profiling (HAL) Complete =====\r\n",
 83, 1000);

 /* USER CODE END 2 */

 /* USER CODE END 2 */

 /* Infinite loop */
 /* USER CODE BEGIN WHILE */
 while (1)
 {
 /* USER CODE END WHILE */

 /* USER CODE BEGIN 3 */
 }
 /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
 RCC_OscInitTypeDef RCC_OscInitStruct = {0};
 RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

 /** Configure the main internal regulator output voltage
 */
 __HAL_RCC_PWR_CLK_ENABLE();
 __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

 /** Initializes the RCC Oscillators according to the specified parameters
 * in the RCC_OscInitTypeDef structure.
 */
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
 if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
 {
 Error_Handler();
 }

 /** Activate the Over-Drive mode
 */
 if (HAL_PWREx_EnableOverDrive() != HAL_OK)
 {
 Error_Handler();
 }

 /** Initializes the CPU, AHB and APB buses clocks
 */
 RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
 |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
 RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
 RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
 RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
 RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

 if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
 {
 Error_Handler();
 }
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

 /* USER CODE BEGIN TIM2_Init 0 */

 /* USER CODE END TIM2_Init 0 */

 TIM_ClockConfigTypeDef sClockSourceConfig = {0};
 TIM_MasterConfigTypeDef sMasterConfig = {0};

 /* USER CODE BEGIN TIM2_Init 1 */

 /* USER CODE END TIM2_Init 1 */
 htim2.Instance = TIM2;
 htim2.Init.Prescaler = 89;
 htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
 htim2.Init.Period = 4294967295;
 htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
 htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
 if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
 {
 Error_Handler();
 }
 sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
 if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
 {
 Error_Handler();
 }
 sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
 sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
 if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
 {
 Error_Handler();
 }
 /* USER CODE BEGIN TIM2_Init 2 */

 /* USER CODE END TIM2_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

 /* USER CODE BEGIN USART2_Init 0 */

 /* USER CODE END USART2_Init 0 */

 /* USER CODE BEGIN USART2_Init 1 */

 /* USER CODE END USART2_Init 1 */
 huart2.Instance = USART2;
 huart2.Init.BaudRate = 115200;
 huart2.Init.WordLength = UART_WORDLENGTH_8B;
 huart2.Init.StopBits = UART_STOPBITS_1;
 huart2.Init.Parity = UART_PARITY_NONE;
 huart2.Init.Mode = UART_MODE_TX_RX;
 huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
 huart2.Init.OverSampling = UART_OVERSAMPLING_16;
 if (HAL_UART_Init(&huart2) != HAL_OK)
 {
 Error_Handler();
 }
 /* USER CODE BEGIN USART2_Init 2 */

 /* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
 /* USER CODE BEGIN MX_GPIO_Init_1 */

 /* USER CODE END MX_GPIO_Init_1 */

 /* GPIO Ports Clock Enable */
 __HAL_RCC_GPIOA_CLK_ENABLE();

 /* USER CODE BEGIN MX_GPIO_Init_2 */

 /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
 /* USER CODE BEGIN Error_Handler_Debug */
 /* User can add his own implementation to report the HAL error return state */
 __disable_irq();
 while (1)
 {
 }
 /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
 /* USER CODE BEGIN 6 */
 /* User can add his own implementation to report the file name and line number,
 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
 /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
