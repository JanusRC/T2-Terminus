//****************************************************************************
// T2 GPIO Handling
//	*30P Header
//	*LEDs
//****************************************************************************
#include "main.h"

//****************************************************************************

void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOE, ENABLE);

  /* Configure PE.12 (RED_LED), PE.13 (YLW_LED), PE.14 (GRN_LED) */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  WriteLED(0); /* All LEDs Off */

#ifdef REV2
  /* Configure PC.13 (Ignition) as input Optically Isolated Input */
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
#else // REV1
 /* Configure PE.08 (Ignition) as input Optically Isolated Input */
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
#endif

	//****************
  // Modem Control
	//****************
	/* Ensure that the Modem module is powered down over a reset */

  /* Configure PC.01 (ENABLE_SUPPLY) and PC.03 (RESET*) as output open-drain */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_ResetBits(GPIOC, GPIO_InitStructure.GPIO_Pin); // Plug_in terminal power supply is disabled

}

//******************************************************************************

void WriteLED(int i)
{
  if (i & 1) // RED
  {
    GPIO_SetBits(GPIOE, GPIO_Pin_12);   // PE.12 High
  }
  else
  {
    GPIO_ResetBits(GPIOE, GPIO_Pin_12); // PE.12 Low
  }

  if (i & 2) // YELLOW
  {
    GPIO_SetBits(GPIOE, GPIO_Pin_13);   // PE.13 High
  }
  else
  {
    GPIO_ResetBits(GPIOE, GPIO_Pin_13); // PE.13 Low
  }

  if (i & 4) // GREEN
  {
    GPIO_SetBits(GPIOE, GPIO_Pin_14);   // PE.14 High
  }
  else
  {
    GPIO_ResetBits(GPIOE, GPIO_Pin_14); // PE.14 Low
  }
}

//******************************************************************************

// TERMINUS2 Pin assignments from schematic PB2409R0-D01

// PA.00 ADC123_CH0/WAKEUP GPIO_7 (EXT)
// PA.01 ADC123_IN1 GPIO_8 (EXT)
// PA.02 ADC123_IN2 GPIO_9 (EXT)
// PA.03 ADC123_IN3 GPIO_10 (EXT)
// PA.04 ADC12_IN4 GPIO_11 (EXT)
// PA.05 ADC12_IN5 GPIO_12 (EXT)
// PA.06 ADC12_IN6 GPIO_13 (EXT)
// PA.07 ADC12_IN7 GPIO_14 (EXT)
// PA.08 OE (CAN/RS485)
// PA.09 OTG_FX_VBUS VBUS_FS (USB)
// PA.10 OTG_FS_ID USB_FS_ID (USB)
// PA.11 OTG_FS_DM USB_FS_DM (USB)
// PA.12 OTG_FS_DP USB_FS_DP (USB)
// PA.13 TMS/SWDIO (JTAG)
// PA.14 TCK/SWDCLK (JTAG)
// PA.15 SPI1_NSS GPIO_1 (EXT) * TDI (JTAG)

// PB.00 ADC12_IN8 CL1+VOUT (CURRENT)
// PB.01 ADC12_IN9 CL2+VOUT (CURRENT)
// PB.02 BOOT1 P10
// PB.03 SPI1_SCK GPIO_2 (EXT) * TDO (JTAG)
// PB.04 SPI1_MISO GPIO_3 (EXT)
// PB.05 SPI1_MOSI GPIO_4 (EXT)
// PB.06 USART1_TX GPS_RX (CF)
// PB.07 USART1_RX GPS_TX (CF)
// PB.08 I2C1_SCL GPIO_5 (EXT)
// PB.09 I2C1_SDA GPIO_6 (EXT)
// PB.10 USART3_TX TX (SERIAL)
// PB.11 USART3_RX RX(SERIAL)
// PB.12 OTG_HS_USBIB USB_ID (CF)
// PB.13 OTG_HS_VBUS VBUS_OUT (CF)
// PB.14 OTG_HS_DM USB_D- (CF)
// PB.15 OTG_HS_DP USB_D+ (CF)

// PC.00 CD (SDCARD)
// PC.01 ENABLE_PLUGIN (CF)
// PC.02 ON_OFF (CF)
// PC.03 RESET (CF)
// PC.04 SERVICE (CF)
// PC.05 ENABLE_VBUS (CF)
// PC.06 USART6_TX TXD (CF)
// PC.07 USART6_RX RXD (CF)
// PC.08 SDIO_D0 D0 (SDCARD)
// PC.09 SDIO_D1 D1 (SDCARD)
// PC.10 SDIO_D2 D2 (SDCARD)
// PC.11 SDIO_D3 D3 (SDCARD)
// PC.12 SDIO_CLK CLK (SDCARD)
// PC.13 PWRMON (CF) [R0 BOARD] - IGNITION (EXT) [R1 BOARD]
// PC.14 GSM_LED (CF) [R0 BOARD] * OSC32_IN (32KHZ)
// PC.15 OSC32_OUT (32KHZ)

// PD.00 CAN1_RX (CAN)
// PD.01 CAN1_TX (CAN)
// PD.02 SDIO_CMD (SDCARD)
// PD.03 DSR (CF)
// PD.04 USART2_RTS (RS485)
// PD.05 USART2_TX TX (RS485)
// PD.06 USART2_RX RX (RS485)
// PD.07 RING (CF)
// PD.08 DCD (CF)
// PD.09 DTR (CF)
// PD.10 GPS_RESET (CF)
// PD.11 USART3_CTS CTS (SERIAL)
// PD.12 USART3_RTS RTS (SERIAL)
// PD.13 ITRIP1 (CURRENT)
// PD.14 RESET_CL1 (CURRENT)
// PD.15 ITRIP2 (CURRENT)

// PE.00 RESET_CL2 (CURRENT)
// PE.01 SA0/SDO (ACCEL)
// PE.02 CS (ACCEL)
// PE.03 PS (ACCEL)
// PE.04 INT1 (ACCEL)
// PE.05 INT2 (ACCEL)
// PE.06 ENABLE (CAN)
// PE.07 ENABLE (ADR)
// PE.08 IGNITION (EXT) [R0 BOARD]
// PE.09
// PE.10
// PE.11
// PE.12 RED_LED
// PE.13 YLW_LED
// PE.14 GRN_LED
// PE.15 INVALID (SERIAL)

// PF.00 I2C2_SDA SDA/SDI/SDO (ACCEL)
// PF.01 I2C2_SCL SCL/SPC (ACCEL)
// PF.02
// PF.03 FORCEON (SERIAL)
// PF.04 FORCEOFF (SERIAL)
// PF.05
// PF.06
// PF.07
// PF.08
// PF.09
// PF.10
// PF.11
// PF.12
// PF.13
// PF.14
// PF.15

// PG.00 PWRMON (CF) [R1 BOARD]
// PG.01 GSM_LED (CF) [R1 BOARD]
// PG.02 GPIO5 (CF)
// PG.03 GPIO6 (CF)
// PG.04 GPIO7 (CF)
// PG.05 GPIO2 (CF)
// PG.06 GPIO1 (CF)
// PG.07 GPIO4 (CF)
// PG.08 GPIO3 (CF)
// PG.09
// PG.10
// PG.11
// PG.12 USART6_RTS RTS (CF)
// PG.13
// PG.14
// PG.15 USART6_CTS CTS (CF)

// TERMINUS 2 EXT HEADER
//
//      ______________===______________
//   30| o o o o o o o o o o o o o o o |2  /-\
//     |                               |   |O| 12V CENTRE
//   29| o o o o o o o o o o o o o o o |1  \-/
//      -------------------------------
//
//  1  SUPPLY ENABLE
//  2  POWER SUPPLY INPUT (12V)
//  3  OPTICAL ISOLATED INPUT (IGN)
//  4  GROUND
//  5  RS485_A (UART2)
//  6  RS485_B
//  7  TRACE RECEIVE (IN)
//  8  TRACE TRANSMIT (OUT)
//  9  CAN_LO
// 10  CAN_HI
// 11  GROUND
// 12  GROUND
// 13  CURRENT LOOP SUPPLY CHANNEL 1
// 14  GPIO1  (PA15 SPI/JTAG)
// 15  CURRENT LOOP SUPPLY CHANNEL 2
// 16  GPIO2  (PB3 SPI/JTAG)
// 17  GROUND
// 18  GPIO3  (PB4 SPI)
// 19  GPIO14 (PA7 ADC)
// 20  GPIO4  (PB5 SPI)
// 21  GPIO13 (PA6 ADC)
// 22  GPIO5  (PB8 I2C)
// 23  GPIO12 (PA5 ADC/DAC)
// 24  GPIO6  (PB9 I2C)
// 25  GPIO11 (PA4 ADC/DAC)
// 26  GPIO7  (PA0 ADC/WKUP/UART4/TIM5)
// 27  GROUND
// 28  GPIO8  (PA1 ADC/UART4/TIM5)
// 29  GPIO10 (PA3 ADC/UART2/TIM5)
// 30  GPIO9  (PA2 ADC/UART2/TIM5)
