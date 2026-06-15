/*
 * ============================================================
 * CSE 2206 — Microcontroller & Embedded System Lab-02
 * Task 6 Option B — Servo Motor Control (TIM2 Channel 1)
 * Platform : STM32F446RE Nucleo-64
 * Output   : PA0 → TIM2 Channel 1 (AF1) → Servo Signal
 *
 * fTIM2_CLK = 90 MHz
 * PSC = 1799 → tick = 90MHz/1800 = 50kHz
 * ARR = 999  → f_PWM = 50kHz/1000 = 50 Hz (20ms period)
 *
 * CCR formula:
 *   t_us = 1000 + (angle/180) * 1000
 *   CCR  = t_us * 50000 / 1000000
 *        = t_us / 20
 * ============================================================
 */

#include <stm32f446xx.h>
#include <stdint.h>
#include <stdio.h>

/* =========================================================
 * System Clock — fCPU = 180 MHz
 * ========================================================= */
void SystemClock_Config(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR      |= PWR_CR_VOS;

    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY));

    FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |=  FLASH_ACR_LATENCY_5WS;

    RCC->PLLCFGR  = 0;
    RCC->PLLCFGR |= (8U   << RCC_PLLCFGR_PLLM_Pos);
    RCC->PLLCFGR |= (180U << RCC_PLLCFGR_PLLN_Pos);
    RCC->PLLCFGR |= (0U   << RCC_PLLCFGR_PLLP_Pos);
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSI;
    RCC->PLLCFGR |= (2U   << RCC_PLLCFGR_PLLQ_Pos);

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    PWR->CR |= PWR_CR_ODEN;
    while (!(PWR->CSR & PWR_CSR_ODRDY));
    PWR->CR |= PWR_CR_ODSWEN;
    while (!(PWR->CSR & PWR_CSR_ODSWRDY));

    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |=  RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

/* =========================================================
 * USART2 — 115200 8N1
 * ========================================================= */
static void USART2_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    GPIOA->MODER  &= ~(3U << (2*2));
    GPIOA->MODER  |=  (2U << (2*2));
    GPIOA->AFR[0] &= ~(0xFU << (4*2));
    GPIOA->AFR[0] |=  (7U   << (4*2));

    GPIOA->MODER  &= ~(3U << (3*2));
    GPIOA->MODER  |=  (2U << (3*2));
    GPIOA->AFR[0] &= ~(0xFU << (4*3));
    GPIOA->AFR[0] |=  (7U   << (4*3));

    USART2->BRR = (24U << 4) | 7U;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void USART2_SendString(const char *s)
{
    while (*s) {
        while (!(USART2->SR & USART_SR_TXE)) {}
        USART2->DR = (uint8_t)(*s++);
    }
    while (!(USART2->SR & USART_SR_TC)) {}
}

/* =========================================================
 * TIM6 — delay functions
 * ========================================================= */
static void TIM6_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    __NOP(); __NOP(); __NOP(); __NOP();

    TIM6->CR1 &= ~TIM_CR1_CEN;
    TIM6->PSC  = 89U;
    TIM6->ARR  = 0xFFFFU;
    TIM6->EGR  = TIM_EGR_UG;
    TIM6->SR   = 0U;
    TIM6->CR1 |= TIM_CR1_CEN;
}

static void delay_us(uint16_t us)
{
    TIM6->CNT = 0U;
    while ((uint16_t)TIM6->CNT < us) {}
}

static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)
        delay_us(1000U);
}

/* =========================================================
 * TIM2 CH1 PWM — PA0 (AF1)
 *
 * fTIM2 = 90 MHz
 * PSC = 1799 → tick_freq = 90MHz/1800 = 50kHz
 * ARR = 999  → PWM_freq = 50kHz/1000 = 50Hz (20ms)
 *
 * CCR = t_us * tick_freq / 1000000
 *     = t_us * 50000 / 1000000
 *     = t_us / 20
 * ========================================================= */
#define SERVO_ARR   999U
#define SERVO_PSC   1799U
#define SERVO_TICK  50000U   /* tick frequency in Hz */

static void TIM2_Servo_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    __NOP(); __NOP();

    /* PA0 → AF1 (TIM2_CH1) */
    GPIOA->MODER  &= ~(3U << (0*2));
    GPIOA->MODER  |=  (2U << (0*2));
    GPIOA->OTYPER &= ~(1U << 0);
    GPIOA->OSPEEDR|=  (3U << (0*2));
    GPIOA->PUPDR  &= ~(3U << (0*2));
    GPIOA->AFR[0] &= ~(0xFU << (4*0));
    GPIOA->AFR[0] |=  (1U   << (4*0));   /* AF1 = TIM2 */

    TIM2->CR1  &= ~TIM_CR1_CEN;
    TIM2->PSC   = SERVO_PSC;
    TIM2->ARR   = SERVO_ARR;

    /* PWM Mode 1 on CH1 */
    TIM2->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM2->CCMR1 |=  (6U << TIM_CCMR1_OC1M_Pos);
    TIM2->CCMR1 |=  TIM_CCMR1_OC1PE;

    /* Enable CH1 output */
    TIM2->CCER  |=  TIM_CCER_CC1E;
    TIM2->CCER  &= ~TIM_CCER_CC1P;

    TIM2->CR1   |=  TIM_CR1_ARPE;
    TIM2->EGR    =  TIM_EGR_UG;
    TIM2->SR     =  0U;

    /* Start at 90° (neutral) */
    TIM2->CCR1   = 75U;

    TIM2->CR1   |=  TIM_CR1_CEN;
}

/* =========================================================
 * Servo_SetAngle
 *
 * angle: 0 to 180 degrees
 *
 * Formula:
 *   t_us = 1000 + (angle * 1000) / 180
 *   CCR  = t_us * SERVO_TICK / 1000000
 *        = t_us / 20
 * ========================================================= */
static void Servo_SetAngle(uint8_t angle, char *buf)
{
    if (angle > 180U) angle = 180U;

    uint32_t t_us = 1000U + ((uint32_t)angle * 1000U) / 180U;
    uint32_t ccr  = t_us * SERVO_TICK / 1000000U;

    TIM2->CCR1 = ccr;

    snprintf(buf, 80,
        "Angle = %3u deg | Pulse = %4lu us | CCR = %3lu\r\n",
        angle,
        (unsigned long)t_us,
        (unsigned long)ccr);
    USART2_SendString(buf);
}

/* =========================================================
 * Main
 * ========================================================= */
int main(void)
{
    char buf[80];

    SystemClock_Config();
    USART2_Init();
    TIM6_Init();
    TIM2_Servo_Init();

    USART2_SendString("\r\n===== Lab-02 Task 6: Servo Motor Control (Bare-Metal) =====\r\n");
    USART2_SendString("Angle     | Pulse (us) | CCR\r\n");
    USART2_SendString("----------+------------+----\r\n");

    /* Sweep 0° to 180° in 18 steps of 10° each */
    for (uint8_t angle = 0; angle <= 180U; angle += 10U)
    {
        Servo_SetAngle(angle, buf);
        delay_ms(500U);   /* 500ms per step */
    }

    delay_ms(1000U);

    /* Sweep back 180° to 0° */
    USART2_SendString("\r\n-- Sweep back: 180 to 0 --\r\n");
    for (int16_t angle = 180; angle >= 0; angle -= 10)
    {
        Servo_SetAngle((uint8_t)angle, buf);
        delay_ms(500U);
    }

    USART2_SendString("\r\n===== Task 6: Servo Motor Control (Bare-Metal) Complete =====\r\n");

    while (1) {}
}
