/*
 * helper.h
 *
 * Created on: Apr 24, 2026
 * Author: User
 */

#ifndef HELPER_H
#define HELPER_H

#include <stdint.h>

/* Clock */
void SystemClock_Config(void);

/* USART2 */
void USART2_Init(void);
void USART2_SendString(const char *s);

/* TIM6 + Delays */
void TIM6_Init(void);
void delay_us(uint16_t us);
void delay_ms(uint32_t ms);

#endif /* HELPER_H */
