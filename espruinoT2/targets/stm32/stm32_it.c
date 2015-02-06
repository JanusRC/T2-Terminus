/**
  ******************************************************************************
  * @file    stm32_it.c
  * @author  MCD Application Team
  * @version V3.4.0
  * @date    29-June-2012
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "platform_config.h"
#include "stm32_it.h"
#ifdef USB
#if defined(STM32F1) || defined(STM32F3)
 #include "usb_utils.h"
 #include "usb_lib.h"
 #include "usb_istr.h"
 #include "usb_pwr.h"
#endif
#ifdef STM32F4
 #include "usb_core.h"
 #include "usbd_core.h"
 #include "usbd_cdc_core.h"
 #include "usb_dcd_int.h"
#endif
#endif
#include "jshardware.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/******************************************************************************/
/*            Cortex-M Processor Exceptions Handlers                         */
/******************************************************************************/

/*******************************************************************************
* Function Name  : NMI_Handler
* Description    : This function handles NMI exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NMI_Handler(void)
{
}

/*******************************************************************************
* Function Name  : HardFault_Handler
* Description    : This function handles Hard Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : MemManage_Handler
* Description    : This function handles Memory Manage exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : BusFault_Handler
* Description    : This function handles Bus Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
__attribute__ ((naked)) void BusFault_Handler(void) {
  /* NAKED function so we can be sure that SP is correct when we
   * run our asm code below */

  // DO NOT clear the busfault active flag - it causes a hard fault!

  /* Instead, we must increase the value of the PC, so that when we
   * return, we don't return to the same instruction.
   *
   * Registers are stacked as follows: r0,r1,r2,r3,r12,lr,pc,xPSR
   * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0337e/Babedgea.html
   *
   * So we want PC - the 6th down * 4 bytes = 24
   *
   * Then we add 2 - which IS DANGEROUS because we're assuming that the op
   * is 2 bytes, but it COULD be 4.
   */
  __asm__(
      "ldr r0, [sp, #24]\n"  // load the PC
      "add r0, #2\n"         // increase by 2 - dangerous, see above
      "str r0, [sp, #24]\n"  // save the PC back
      "bx lr\n"              // Return (function is naked so we must do this explicitly)
  );
}

/*******************************************************************************
* Function Name  : UsageFault_Handler
* Description    : This function handles Usage Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/*******************************************************************************
* Function Name  : SVC_Handler
* Description    : This function handles SVCall exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVC_Handler(void)
{
}

/*******************************************************************************
* Function Name  : DebugMon_Handler
* Description    : This function handles Debug Monitor exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DebugMon_Handler(void)
{
}

/*******************************************************************************
* Function Name  : PendSV_Handler
* Description    : This function handles PendSVC exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void PendSV_Handler(void)
{
}

/*******************************************************************************
* Function Name  : SysTick_Handler
* Description    : This function handles SysTick Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SysTick_Handler(void)
{
  jshDoSysTick();
}

void EXTI0_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line0) == SET) {
      jshPushIOWatchEvent(EV_EXTI0);
      EXTI_ClearITPendingBit(EXTI_Line0);
    }
}
void EXTI1_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line1) == SET) {
      jshPushIOWatchEvent(EV_EXTI1);
      EXTI_ClearITPendingBit(EXTI_Line1);
    }
}
void EXTI2_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line2) == SET) {
      jshPushIOWatchEvent(EV_EXTI2);
      EXTI_ClearITPendingBit(EXTI_Line2);
    }
}
void EXTI3_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line3) == SET) {
      jshPushIOWatchEvent(EV_EXTI3);
      EXTI_ClearITPendingBit(EXTI_Line3);
    }
}
void EXTI4_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line4) == SET) {
      jshPushIOWatchEvent(EV_EXTI4);
      EXTI_ClearITPendingBit(EXTI_Line4);
    }
}
void EXTI9_5_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line5) == SET) {
      jshPushIOWatchEvent(EV_EXTI5);
      EXTI_ClearITPendingBit(EXTI_Line5);
    }
    if (EXTI_GetITStatus(EXTI_Line6) == SET) {
      jshPushIOWatchEvent(EV_EXTI6);
      EXTI_ClearITPendingBit(EXTI_Line6);
    }
    if (EXTI_GetITStatus(EXTI_Line7) == SET) {
      jshPushIOWatchEvent(EV_EXTI7);
      EXTI_ClearITPendingBit(EXTI_Line7);
    }
    if (EXTI_GetITStatus(EXTI_Line8) == SET) {
      jshPushIOWatchEvent(EV_EXTI8);
      EXTI_ClearITPendingBit(EXTI_Line8);
    }
    if (EXTI_GetITStatus(EXTI_Line9) == SET) {
      jshPushIOWatchEvent(EV_EXTI9);
      EXTI_ClearITPendingBit(EXTI_Line9);
    }
}
void EXTI15_10_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line10) == SET) {
      jshPushIOWatchEvent(EV_EXTI10);
      EXTI_ClearITPendingBit(EXTI_Line10);
    }
    if (EXTI_GetITStatus(EXTI_Line11) == SET) {
      jshPushIOWatchEvent(EV_EXTI11);
      EXTI_ClearITPendingBit(EXTI_Line11);
    }
    if (EXTI_GetITStatus(EXTI_Line12) == SET) {
      jshPushIOWatchEvent(EV_EXTI12);
      EXTI_ClearITPendingBit(EXTI_Line12);
    }
    if (EXTI_GetITStatus(EXTI_Line13) == SET) {
      jshPushIOWatchEvent(EV_EXTI13);
      EXTI_ClearITPendingBit(EXTI_Line13);
    }
    if (EXTI_GetITStatus(EXTI_Line14) == SET) {
      jshPushIOWatchEvent(EV_EXTI14);
      EXTI_ClearITPendingBit(EXTI_Line14);
    }
    if (EXTI_GetITStatus(EXTI_Line15) == SET) {
      jshPushIOWatchEvent(EV_EXTI15);
      EXTI_ClearITPendingBit(EXTI_Line15);
    }
}

void RTC_IRQHandler(void) {
#ifdef STM32F1
  RTC_ClearITPendingBit(RTC_IT_ALR);
#else
  RTC_ClearITPendingBit(RTC_IT_ALRA);
#endif
}

void RTCAlarm_IRQHandler(void) {
  EXTI_ClearITPendingBit(EXTI_Line17);
}

#ifdef STM32F4
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_pwr.h"
void RTC_WKUP_IRQHandler(void)
{
  if (RTC_GetITStatus(RTC_IT_WUT) != RESET) {
    EXTI_ClearITPendingBit(EXTI_Line22);
    RTC_ClearITPendingBit(RTC_IT_WUT);
    RTC_ClearFlag(RTC_FLAG_WUTF);
  }
}
#endif

static void USART_IRQHandler(USART_TypeDef *USART, IOEventFlags device) {
  if(USART_GetITStatus(USART, USART_IT_RXNE) != RESET) {
     /* Clear the USART Receive interrupt */
     USART_ClearITPendingBit(USART, USART_IT_RXNE);
     /* Read one byte from the receive data register */
     jshPushIOCharEvent(device, (char)USART_ReceiveData(USART));
   }
   /* If overrun condition occurs, clear the ORE flag and recover communication */
   if (USART_GetFlagStatus(USART, USART_FLAG_ORE) != RESET)
   {
     (void)USART_ReceiveData(USART);
   }
   if(USART_GetITStatus(USART, USART_IT_TXE) != RESET) {
     /* If we have other data to send, send it */
     int c = jshGetCharToTransmit(device);
     if (c >= 0) {
       USART_SendData(USART, (uint16_t)c);
     } else
       USART_ITConfig(USART, USART_IT_TXE, DISABLE);
   }
}

void USART1_IRQHandler(void) {
  USART_IRQHandler(USART1, EV_SERIAL1);
}

void USART2_IRQHandler(void) {
  USART_IRQHandler(USART2, EV_SERIAL2);
}

void USART3_IRQHandler(void) {
  USART_IRQHandler(USART3, EV_SERIAL3);
}

void UART4_IRQHandler(void) {
  USART_IRQHandler(UART4, EV_SERIAL4);
}

void UART5_IRQHandler(void) {
  USART_IRQHandler(UART5, EV_SERIAL5);
}

#ifdef STM32F4
void USART6_IRQHandler(void) {
  USART_IRQHandler(USART6, EV_SERIAL6);
}
#endif


static void SPI_IRQHandler(SPI_TypeDef *SPIx, IOEventFlags device) {
   while (SPI_I2S_GetITStatus(SPIx, SPI_I2S_IT_RXNE) != RESET) {
      // Read one byte/word from the receive data register
      jshSPIPush(device, SPI_I2S_ReceiveData(SPIx));
    }
}

#if SPIS>=1
void SPI1_IRQHandler(void) {
  SPI_IRQHandler(SPI1, EV_SPI1);
}
#endif

#if SPIS>=2
void SPI2_IRQHandler(void) {
  SPI_IRQHandler(SPI2, EV_SPI2);
}
#endif

#if SPIS>=3
void SPI3_IRQHandler(void) {
  SPI_IRQHandler(SPI3, EV_SPI3);
}
#endif


/** The 'utility' timer - used for pulse generation and shifting data */
// void UTIL_TIMER_IRQHandler(void)
// Defined in jshardware.c

#ifdef USB

#ifdef STM32F1
/*******************************************************************************
* Function Name  : USB_IRQHandler
* Description    : This function handles USB Low Priority interrupts
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  USB_Istr();
}

void USBWakeUp_IRQHandler(void)
{
  EXTI_ClearITPendingBit(EXTI_Line18);
}
#endif // STM32F1

#ifdef STM32F3
#if defined (USB_INT_DEFAULT)
void USB_LP_CAN1_RX0_IRQHandler(void)
#elif defined (USB_INT_REMAP)
void USB_LP_IRQHandler(void)
#endif
{
   USB_Istr();
}

#if defined (USB_INT_DEFAULT)
void USBWakeUp_IRQHandler(void)
#elif defined (USB_INT_REMAP)
void USBWakeUp_RMP_IRQHandler(void)
#endif
{
  /* Initiate external resume sequence (1 step) */
  Resume(RESUME_EXTERNAL);
  EXTI_ClearITPendingBit(EXTI_Line18);
}
#endif // STM32F3

#ifdef STM32F4
/******************************************************************************/
/*                 STM32 Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32xxx.s).                                            */
/******************************************************************************/

/*******************************************************************************
* Function Name  : PPP_IRQHandler
* Description    : This function handles PPP interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @brief  This function handles EXTI15_10_IRQ Handler.
  * @param  None
  * @retval None
  */
#ifdef USE_USB_OTG_FS
void OTG_FS_WKUP_IRQHandler(void)
{
  if(USB_OTG_dev.cfg.low_power)
  {
    *(uint32_t *)(0xE000ED10) &= 0xFFFFFFF9 ;
    SystemInit();
    USB_OTG_UngateClock(&USB_OTG_dev);
  }
  EXTI_ClearITPendingBit(EXTI_Line18);
}
#endif

/**
  * @brief  This function handles EXTI15_10_IRQ Handler.
  * @param  None
  * @retval None
  */
#ifdef USE_USB_OTG_HS
void OTG_HS_WKUP_IRQHandler(void)
{
  if(USB_OTG_dev.cfg.low_power)
  {
    *(uint32_t *)(0xE000ED10) &= 0xFFFFFFF9 ;
    SystemInit();
    USB_OTG_UngateClock(&USB_OTG_dev);
  }
  EXTI_ClearITPendingBit(EXTI_Line20);
}
#endif

/**
  * @brief  This function handles OTG_HS Handler.
  * @param  None
  * @retval None
  */
#ifdef USE_USB_OTG_HS
void OTG_HS_IRQHandler(void)
#else
void OTG_FS_IRQHandler(void)
#endif
{
  USBD_OTG_ISR_Handler (&USB_OTG_dev);
}

#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED

extern uint32_t USBD_OTG_EP1IN_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
extern uint32_t USBD_OTG_EP1OUT_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);

/**
  * @brief  This function handles EP1_IN Handler.
  * @param  None
  * @retval None
  */
void OTG_HS_EP1_IN_IRQHandler(void)
{
  USBD_OTG_EP1IN_ISR_Handler (&USB_OTG_dev);
}

/**
  * @brief  This function handles EP1_OUT Handler.
  * @param  None
  * @retval None
  */
void OTG_HS_EP1_OUT_IRQHandler(void)
{
  USBD_OTG_EP1OUT_ISR_Handler (&USB_OTG_dev);
}
#endif

#endif // STM32F4
#endif // USB

#ifdef USE_FILESYSTEM_SDIO
#include "sdio_sdcard.h"

/*******************************************************************************
* Function Name  : SDIO_IRQHandler
* Description    : This function handles SDIO interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SDIO_IRQHandler(void)
{
  SD_ProcessIRQSrc();
}
#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

