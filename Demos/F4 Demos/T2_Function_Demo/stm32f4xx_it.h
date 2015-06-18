///******************************************************************************
//
//  TERMINUS2 Interrupt Handlers
//
//  Clive Turvey - Connor Winfield Corp (NavSync and Janus RC Groups)
//     cturvey@conwin.com  cturvey@navsync.com  cturvey@gmail.com
//
//******************************************************************************

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#include "stm32f4xx.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#endif /* __STM32F4xx_IT_H */

