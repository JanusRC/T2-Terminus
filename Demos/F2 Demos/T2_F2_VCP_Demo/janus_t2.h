/**
  ******************************************************************************
  *  TERMINUS T2 PATCHES
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __JANUS_T2_H
#define __JANUS_T2_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f2xx.h"

typedef enum
{
  LED1 = 0,
  LED2 = 1,
  LED3 = 2
} Led_TypeDef;

typedef enum
{
  COM1 = 0,
  COM2 = 1,
  COM3 = 2
} COM_TypeDef;

#if !defined (USE_JANUS_T2)
 #define USE_JANUS_T2
#endif

/** @addtogroup T2_LOW_LEVEL_LED
  * @{
  */
#define LEDn                             3

#define LED1_PIN                         GPIO_Pin_12 // RED_LED PE.12 T2
#define LED1_GPIO_PORT                   GPIOE
#define LED1_GPIO_CLK                    RCC_AHB1Periph_GPIOE

#define LED2_PIN                         GPIO_Pin_13 // YLW_LED PE.13 T2
#define LED2_GPIO_PORT                   GPIOE
#define LED2_GPIO_CLK                    RCC_AHB1Periph_GPIOE

#define LED3_PIN                         GPIO_Pin_14 // GRN_LED PE.14 T2
#define LED3_GPIO_PORT                   GPIOE
#define LED3_GPIO_CLK                    RCC_AHB1Periph_GPIOE

/** @addtogroup T2_LOW_LEVEL_COM
  * @{
  */
#define COMn                             3

/**
 * @brief Definition for COM port1, connected to USART3 (RS232)
 */
#define EVAL_COM1                        USART3
#define EVAL_COM1_CLK                    RCC_APB1Periph_USART3
#define EVAL_COM1_TX_PIN                 GPIO_Pin_10 // PB.10 T2
#define EVAL_COM1_TX_GPIO_PORT           GPIOB
#define EVAL_COM1_TX_GPIO_CLK            RCC_AHB1Periph_GPIOB
#define EVAL_COM1_TX_SOURCE              GPIO_PinSource10
#define EVAL_COM1_TX_AF                  GPIO_AF_USART3
#define EVAL_COM1_RX_PIN                 GPIO_Pin_11	// PB.11 T2
#define EVAL_COM1_RX_GPIO_PORT           GPIOB
#define EVAL_COM1_RX_GPIO_CLK            RCC_AHB1Periph_GPIOB
#define EVAL_COM1_RX_SOURCE              GPIO_PinSource11
#define EVAL_COM1_RX_AF                  GPIO_AF_USART3
#define EVAL_COM1_IRQn                   USART3_IRQn
#define EVAL_COM1_IRQHandler             USART3_IRQHandler

/**
 * @brief Definition for COM port2, connected to USART6 (MODEM)
 */
#define EVAL_COM2                        USART6
#define EVAL_COM2_CLK                    RCC_APB2Periph_USART6
#define EVAL_COM2_TX_PIN                 GPIO_Pin_6 // PC.6 T2
#define EVAL_COM2_TX_GPIO_PORT           GPIOC
#define EVAL_COM2_TX_GPIO_CLK            RCC_AHB1Periph_GPIOC
#define EVAL_COM2_TX_SOURCE              GPIO_PinSource6
#define EVAL_COM2_TX_AF                  GPIO_AF_USART6
#define EVAL_COM2_RX_PIN                 GPIO_Pin_7	// PC.7 T2
#define EVAL_COM2_RX_GPIO_PORT           GPIOC
#define EVAL_COM2_RX_GPIO_CLK            RCC_AHB1Periph_GPIOC
#define EVAL_COM2_RX_SOURCE              GPIO_PinSource7
#define EVAL_COM2_RX_AF                  GPIO_AF_USART6
#define EVAL_COM2_IRQn                   USART6_IRQn
#define EVAL_COM2_IRQHandler             USART6_IRQHandler

/**
 * @brief Definition for COM port3, connected to USART1 (GPS)
 */
#define EVAL_COM3                        USART1
#define EVAL_COM3_CLK                    RCC_APB2Periph_USART1
#define EVAL_COM3_TX_PIN                 GPIO_Pin_6 // PB.6 T2
#define EVAL_COM3_TX_GPIO_PORT           GPIOB
#define EVAL_COM3_TX_GPIO_CLK            RCC_AHB1Periph_GPIOB
#define EVAL_COM3_TX_SOURCE              GPIO_PinSource6
#define EVAL_COM3_TX_AF                  GPIO_AF_USART1
#define EVAL_COM3_RX_PIN                 GPIO_Pin_7	// PB.7 T2
#define EVAL_COM3_RX_GPIO_PORT           GPIOB
#define EVAL_COM3_RX_GPIO_CLK            RCC_AHB1Periph_GPIOB
#define EVAL_COM3_RX_SOURCE              GPIO_PinSource7
#define EVAL_COM3_RX_AF                  GPIO_AF_USART1
#define EVAL_COM3_IRQn                   USART1_IRQn
#define EVAL_COM3_IRQHandler             USART6_IRQHandler

/**
  * @}
  */

/** @addtogroup T2_LOW_LEVEL_SD_FLASH
  * @{
  */
/**
  * @brief  SD FLASH SDIO Interface
  */
#define SD_DETECT_PIN                    GPIO_Pin_0                  /* PC.00 T2 */
#define SD_DETECT_GPIO_PORT              GPIOC                       /* GPIOC */
#define SD_DETECT_GPIO_CLK               RCC_AHB1Periph_GPIOC

#define SDIO_FIFO_ADDRESS                ((uint32_t)0x40012C80)
/**
  * @brief  SDIO Intialization Frequency (400KHz max)
  */
#define SDIO_INIT_CLK_DIV                ((uint8_t)0x76)
/**
  * @brief  SDIO Data Transfer Frequency (25MHz max)
  */
#define SDIO_TRANSFER_CLK_DIV            ((uint8_t)0x0)

#define SD_SDIO_DMA                   DMA2
#define SD_SDIO_DMA_CLK               RCC_AHB1Periph_DMA2

#define SD_SDIO_DMA_STREAM3	          3
//#define SD_SDIO_DMA_STREAM6           6

#ifdef SD_SDIO_DMA_STREAM3
 #define SD_SDIO_DMA_STREAM            DMA2_Stream3
 #define SD_SDIO_DMA_CHANNEL           DMA_Channel_4
 #define SD_SDIO_DMA_FLAG_FEIF         DMA_FLAG_FEIF3
 #define SD_SDIO_DMA_FLAG_DMEIF        DMA_FLAG_DMEIF3
 #define SD_SDIO_DMA_FLAG_TEIF         DMA_FLAG_TEIF3
 #define SD_SDIO_DMA_FLAG_HTIF         DMA_FLAG_HTIF3
 #define SD_SDIO_DMA_FLAG_TCIF         DMA_FLAG_TCIF3
 #define SD_SDIO_DMA_IRQn              DMA2_Stream3_IRQn
 #define SD_SDIO_DMA_IRQHANDLER        DMA2_Stream3_IRQHandler
#elif defined SD_SDIO_DMA_STREAM6
 #define SD_SDIO_DMA_STREAM            DMA2_Stream6
 #define SD_SDIO_DMA_CHANNEL           DMA_Channel_4
 #define SD_SDIO_DMA_FLAG_FEIF         DMA_FLAG_FEIF6
 #define SD_SDIO_DMA_FLAG_DMEIF        DMA_FLAG_DMEIF6
 #define SD_SDIO_DMA_FLAG_TEIF         DMA_FLAG_TEIF6
 #define SD_SDIO_DMA_FLAG_HTIF         DMA_FLAG_HTIF6
 #define SD_SDIO_DMA_FLAG_TCIF         DMA_FLAG_TCIF6
 #define SD_SDIO_DMA_IRQn              DMA2_Stream6_IRQn
 #define SD_SDIO_DMA_IRQHANDLER        DMA2_Stream6_IRQHandler
#endif /* SD_SDIO_DMA_STREAM3 */


void STM_EVAL_LEDInit(Led_TypeDef Led);
void STM_EVAL_LEDOn(Led_TypeDef Led);
void STM_EVAL_LEDOff(Led_TypeDef Led);
void STM_EVAL_LEDToggle(Led_TypeDef Led);

void STM_EVAL_COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct);

void SD_LowLevel_DeInit(void);
void SD_LowLevel_Init(void);
void SD_LowLevel_DMA_TxConfig(uint32_t *BufferSRC, uint32_t BufferSize);
void SD_LowLevel_DMA_RxConfig(uint32_t *BufferDST, uint32_t BufferSize);

#ifdef __cplusplus
}
#endif

#endif /* __JANUS_T2_H */
