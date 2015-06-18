/**
  ******************************************************************************
  * @file    stm322xg_eval.h ##### MODIFIED FOR T2 ####
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32_T2_H
#define __STM32_T2_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "stm32f4xx_sdio.h"
#include "stm32f4xx_dma.h"

#define SD_DETECT_PIN                    GPIO_Pin_0                  /* PC.00 T2 */
#define SD_DETECT_GPIO_PORT              GPIOC                       /* GPIOC */
#define SD_DETECT_GPIO_CLK               RCC_AHB1Periph_GPIOC

#define SDIO_FIFO_ADDRESS                ((uint32_t)0x40012C80)
/**
  * @brief  SDIO Intialization Frequency (400KHz max)
  */
#define SDIO_INIT_CLK_DIV                ((uint8_t)0x76) // (48000000 / 400000) - 2
//#define SDIO_INIT_CLK_DIV                ((uint8_t)0x94) // (60000000 / 400000) - 2
/**
  * @brief  SDIO Data Transfer Frequency (25MHz max)
  */
#define SDIO_TRANSFER_CLK_DIV            ((uint8_t)0x0) // (48000000 / 24000000) - 2

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
#elif defined SD_SDIO_DMA_STREAM6
 #define SD_SDIO_DMA_STREAM            DMA2_Stream6
 #define SD_SDIO_DMA_CHANNEL           DMA_Channel_4
 #define SD_SDIO_DMA_FLAG_FEIF         DMA_FLAG_FEIF6
 #define SD_SDIO_DMA_FLAG_DMEIF        DMA_FLAG_DMEIF6
 #define SD_SDIO_DMA_FLAG_TEIF         DMA_FLAG_TEIF6
 #define SD_SDIO_DMA_FLAG_HTIF         DMA_FLAG_HTIF6
 #define SD_SDIO_DMA_FLAG_TCIF         DMA_FLAG_TCIF6
#endif /* SD_SDIO_DMA_STREAM3 */


void SD_LowLevel_DeInit(void);
void SD_LowLevel_Init(void);
void SD_LowLevel_DMA_TxConfig(uint32_t *BufferSRC, uint32_t BufferSize);
void SD_LowLevel_DMA_RxConfig(uint32_t *BufferDST, uint32_t BufferSize);
uint32_t SD_DMAEndOfTransferStatus(void);
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32_T2_H */
