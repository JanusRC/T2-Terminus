/**
  ******************************************************************************
  * @file    app.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file provides all the Application firmware functions.
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

#define REV2 // T2 REV2

/* Includes ------------------------------------------------------------------*/ 

#include <stdio.h>

#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usb_conf.h"
#include "usbd_desc.h"

#define STDERR ((void *)0x12345678)
//#define STDERR (stdout)

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup APP_VCP 
  * @brief Mass storage application module
  * @{
  */ 

/** @defgroup APP_VCP_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Defines
  * @{
  */ 

/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Variables
  * @{
  */ 
  
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
   
__ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_dev __ALIGN_END ;

/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_FunctionPrototypes
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup APP_VCP_Private_Functions
  * @{
  */ 

//******************************************************************************

volatile int SystemTick       = 0; // 1 ms ticker
int CurrentLED = 0;

//******************************************************************************

// Win32 style implementation of time delay, or yield type functionality
//  Timer increments on timer tick interrupt, so processor halted with
//  Thumb2 WFI instruction so this won't grind in a loop unnecessarily

int Sleep(int Delay)
{
  int Start, Current;

  Start = SystemTick;

  do
  {
    __WFI();                  // Wait for interrupt

    Current = SystemTick;
  }
  while((Current - Start) < Delay);

  return(Current - Start); // Estimate of time past
}

//******************************************************************************

// From http://forums.arm.com/index.php?showtopic=13949

volatile unsigned int *DWT_CYCCNT   = (volatile unsigned int *)0xE0001004; //address of the register
volatile unsigned int *DWT_CONTROL  = (volatile unsigned int *)0xE0001000; //address of the register
volatile unsigned int *SCB_DEMCR    = (volatile unsigned int *)0xE000EDFC; //address of the register

int FiveMicro;

//******************************************************************************

void EnableTiming(void)
{
  static int enabled = 0;

  if (!enabled)
  {
    RCC_ClocksTypeDef RCC_Clocks;

    RCC_GetClocksFreq(&RCC_Clocks);

    FiveMicro = RCC_Clocks.HCLK_Frequency / (1000000 / 5); // Clock ticks for ~5us

    *SCB_DEMCR = *SCB_DEMCR | 0x01000000;
    *DWT_CYCCNT = 0; // reset the counter
    *DWT_CONTROL = *DWT_CONTROL | 1 ; // enable the counter

    enabled = 1;
  }
}

//******************************************************************************

void TimingDelay(unsigned int tick)
{
  unsigned int start, current;

  start = *DWT_CYCCNT;

  do
  {
    current = *DWT_CYCCNT;
  } while((current - start) < tick);
}

//******************************************************************************
//
// STM32 Uart Library
//
//******************************************************************************

typedef struct _GPIO {
  GPIO_TypeDef* Bank;
  uint32_t Pin;
  uint32_t PinSource;
} GPIO;

//******************************************************************************

typedef struct _UART {

  USART_TypeDef *Port;

  int Transmit;
  int Throttle;
  int Errors;
  int Overflow;
  int RxCount;
  int Busy;

  int ModemOn;
  int ModemOnTick;
  int ModemConnected;
  int NetworkRegistrationState; // Not, Failed, Local, Roam
  int NetworkActivationState;

  GPIO CTS_GPIO;
  GPIO RTS_GPIO;
  GPIO DSR_GPIO;
  GPIO DTR_GPIO;
  GPIO DCD_GPIO;
  GPIO RING_GPIO;

  GPIO PWRMON_GPIO;
  GPIO ONOFF_GPIO;
  GPIO RESET_GPIO;
  GPIO GSMLED_GPIO;

} UART;

//******************************************************************************

static UART Uart6[1]; // Modem

static UART *UartModem = Uart6;

void USART6_Suppress(UART *Uart, int Enable);

//******************************************************************************
//
// Modem Hardware Stuff
//
//******************************************************************************

//#define DBG_MODEM

u8 Uart_ModemReadPwrMon(UART *Uart)
{
  if (Uart->PWRMON_GPIO.Bank)
    return(GPIO_ReadInputDataBit(Uart->PWRMON_GPIO.Bank, Uart->PWRMON_GPIO.Pin)); // Modem PwrMon
  else
    return(0);
}

//******************************************************************************

u8 Uart_ModemReadOnOff(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin)); // Modem OnOff
}

//******************************************************************************

void Uart_ModemWriteOnOff(UART *Uart, int i)
{
  if (i)
  {
#ifdef DBG_MODEM
    fprintf(STDERR, "Assert   OnOff (HIGH)\r\n");
#endif
    GPIO_SetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin);    // Modem OnOff High
  }
  else
  {
#ifdef DBG_MODEM
    fprintf(STDERR, "Deassert OnOff (LOW)\r\n");
#endif
    GPIO_ResetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin);  // Modem OnOff Low
  }
}

//******************************************************************************

u8 Uart_ModemReadGSMLED(UART *Uart)
{
  if (Uart->GSMLED_GPIO.Bank)
    return(GPIO_ReadInputDataBit(Uart->GSMLED_GPIO.Bank, Uart->GSMLED_GPIO.Pin)); // Modem LED
  else
    return(0);
}

//******************************************************************************

u8 Uart_ModemReadDSR(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->DSR_GPIO.Bank, Uart->DSR_GPIO.Pin)); // Modem DSR
}

//******************************************************************************

u8 Uart_ModemReadRING(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->RING_GPIO.Bank, Uart->RING_GPIO.Pin)); // Modem RING
}

//******************************************************************************

u8 Uart_ModemReadDCD(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->DCD_GPIO.Bank, Uart->DCD_GPIO.Pin)); // Modem DCD
}

//******************************************************************************

u8 Uart_ModemReadCTS(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->CTS_GPIO.Bank, Uart->CTS_GPIO.Pin)); // Modem CTS
}

//******************************************************************************

u8 Uart_ModemReadRTS(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.Pin)); // Modem RTS
}

//******************************************************************************

void Uart_ModemWriteRTS(UART *Uart, int i)
{
  if (i)
    GPIO_SetBits(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.Pin);   // RTS High
  else
    GPIO_ResetBits(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.Pin); // RTS Low
}

//******************************************************************************

u8 Uart_ModemReadDTR(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->DTR_GPIO.Bank, Uart->DTR_GPIO.Pin)); // Modem DTR
}

//******************************************************************************

void Uart_ModemWriteDTR(UART *Uart, int i)
{
  if (i)
    GPIO_SetBits(Uart->DTR_GPIO.Bank, Uart->DTR_GPIO.Pin);   // DTR High
  else
    GPIO_ResetBits(Uart->DTR_GPIO.Bank, Uart->DTR_GPIO.Pin); // DTR Low
}

//******************************************************************************

u8 Uart_ModemReadReset(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin)); // Modem Reset
}

//******************************************************************************

void Uart_ModemWriteReset(UART *Uart, int i)
{
  if (i)
  {
#ifdef DBG_MODEM
    fprintf(STDERR, "Assert   Reset (HIGH)\r\n");
#endif
    GPIO_SetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);    // Modem Reset High

for(i=0; i<10; i++)
{
  if (GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin) == Bit_SET)
  {
#ifdef DBG_MODEM
    fprintf(STDERR, "H");
#endif
    break;
  }
#ifdef DBG_MODEM
  else
    fprintf(STDERR, "L");
#endif

  Sleep(100);
}
#ifdef DBG_MODEM
  fprintf(STDERR, "\n");
#endif

#ifdef DBG_MODEM
    if (GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin) != Bit_SET)
      fprintf(STDERR, "Pin not asserting\r\n");
#endif

for(i=0; i<10; i++)
{
  if (GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin) == Bit_SET)
  {
#ifdef DBG_MODEM
    fprintf(STDERR, "H");
#endif
    break;
  }
#ifdef DBG_MODEM
  else
    fprintf(STDERR, "L");
#endif

  Sleep(100);
}
#ifdef DBG_MODEM
  fprintf(STDERR, "\n");
#endif
  }
  else
  {
#ifdef DBG_MODEM
    fprintf(STDERR, "Deassert Reset (LOW)\r\n");
#endif

    GPIO_ResetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);  // Modem Set Low

    Sleep(125);

#ifdef DBG_MODEM
    if (GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin) != Bit_RESET)
      fprintf(STDERR, "Pin not deasserting\r\n");
#endif
  }
}

//****************************************************************************

int Uart_ModemReadPower(UART *Uart)
{
#if 1
  if (Uart_ModemReadPwrMon(Uart) == Bit_RESET)
    return(0);
#endif

  if (Uart_ModemReadReset(Uart) == Bit_RESET)
    return(0);

  return(1);
}

//****************************************************************************

// Display the modem status lines for diagnostic purposes

void Uart_DumpModemPins(UART *Uart)
{
  if (Uart_ModemReadDSR(Uart) == Bit_SET)
    fprintf(STDERR, "DSR  Hi (I),");
  else
    fprintf(STDERR, "DSR  Lo (I),");

  if (Uart_ModemReadRING(Uart) == Bit_SET)
    fprintf(STDERR, "RING Hi (I),");
  else
    fprintf(STDERR, "RING Lo (I),");

  if (Uart_ModemReadDCD(Uart) == Bit_SET)
    fprintf(STDERR, "DCD  Hi (I),");
  else
    fprintf(STDERR, "DCD  Lo (I),");

  if (Uart_ModemReadDTR(Uart) == Bit_SET)
    fprintf(STDERR, "DTR  Hi (O),");
  else
    fprintf(STDERR, "DTR  Lo (O),");

  if (Uart_ModemReadCTS(Uart) == Bit_SET)
    fprintf(STDERR, "CTS  Hi (I),");
  else
    fprintf(STDERR, "CTS  Lo (I),");

  if (Uart_ModemReadRTS(Uart) == Bit_SET)
    fprintf(STDERR, "RTS  Hi (O)\r\n");
  else
    fprintf(STDERR, "RTS  Lo (O)\r\n");


  if (Uart_ModemReadPwrMon(Uart) == Bit_SET)
    fprintf(STDERR, "PWRMON Hi (I),");
  else
    fprintf(STDERR, "PWRMON Lo (I),");

  if (Uart_ModemReadReset(Uart) == Bit_SET)
    fprintf(STDERR, "RESET Hi (O),");
  else
    fprintf(STDERR, "RESET Lo (O),");

  if (Uart_ModemReadOnOff(Uart) == Bit_SET)
    fprintf(STDERR, "ONOFF Hi (O)\r\n");
  else
    fprintf(STDERR, "ONOFF Lo (O)\r\n");
}

//****************************************************************************

void Uart_DumpModemPinsBrief(UART *Uart, char *s)
{
  if (Uart_ModemReadDSR(Uart) == Bit_SET)
    fprintf(STDERR, "H");
  else
    fprintf(STDERR, "L");

  if (Uart_ModemReadRING(Uart) == Bit_SET)
    fprintf(STDERR, "H");
  else
    fprintf(STDERR, "L");

  if (Uart_ModemReadDCD(Uart) == Bit_SET)
    fprintf(STDERR, "H");
  else
    fprintf(STDERR, "L");

  if (Uart_ModemReadDTR(Uart) == Bit_SET)
    fprintf(STDERR, "H");
  else
    fprintf(STDERR, "L");

  if (Uart_ModemReadCTS(Uart) == Bit_SET)
    fprintf(STDERR, "H");
  else
    fprintf(STDERR, "L");

  if (Uart_ModemReadRTS(Uart) == Bit_SET)
    fprintf(STDERR, "H");
  else
    fprintf(STDERR, "L");

  fprintf(STDERR, " ");

  if (Uart_ModemReadPwrMon(Uart) == Bit_SET)
    fprintf(STDERR, "H");
  else
    fprintf(STDERR, "L");

  if (Uart_ModemReadReset(Uart) == Bit_SET)
    fprintf(STDERR, "H");
  else
    fprintf(STDERR, "L");

  if (Uart_ModemReadOnOff(Uart) == Bit_SET)
    fprintf(STDERR, "H");
  else
    fprintf(STDERR, "L");

  if (Uart_ModemReadGSMLED(Uart) == Bit_SET)
    fprintf(STDERR, "H");
  else
    fprintf(STDERR, "L");

  fprintf(STDERR, "%-8s %10d\n", s, SystemTick);
}

//******************************************************************************

void Uart_DebugModemDCD(UART *Uart)
{
  if (Uart_ModemReadDCD(Uart) == Bit_SET) // Check DCD
    fprintf(STDERR, "DCD Hi\n");
  else
    fprintf(STDERR, "DCD Lo\n");
}

//******************************************************************************

void Uart_DebugModemPwrMon(UART *Uart, int mode)
{
  if (mode)
  {
    if (Uart_ModemReadReset(Uart) == Bit_SET)
      fprintf(STDERR, "Modem Reset High, ");
    else
      fprintf(STDERR, "Modem Reset Low,  ");

    if (Uart_ModemReadOnOff(Uart) == Bit_SET)
      fprintf(STDERR, "Modem OnOff High, ");
    else
      fprintf(STDERR, "Modem OnOff Low,  ");

    if (Uart_ModemReadPwrMon(Uart) == Bit_SET)
      fprintf(STDERR, "Modem PwrMon High\r\n");
    else
      fprintf(STDERR, "Modem PwrMon Low\r\n");
  }
}

//******************************************************************************

void Uart_DebugModemOnOff(UART *Uart)
{
  if (Uart_ModemReadOnOff(Uart) == Bit_SET)
    fprintf(STDERR, "Modem OnOff High\r\n");
  else
    fprintf(STDERR, "Modem OnOff Low\r\n");
}

//******************************************************************************

void Uart_ModemReset(UART *Uart)
{
  Uart_ModemWriteReset(Uart, 0); // Modem Reset Low

  Sleep(250);         // At least 200 ms

  Uart_ModemWriteReset(Uart, 1); // Modem Reset High

  Sleep(250);
}

//******************************************************************************

//#define DEBUG

#ifdef HE910
#define TELIT_TIME_ON  5000 // HE910
#define TELIT_TIME_OFF 3300
#else
#define TELIT_TIME_ON  1250
#define TELIT_TIME_OFF 2500
#endif

//#define DBG_MODEM
//#define DBG_BRIEF

//******************************************************************************

int Uart_ModemOn(UART *Uart, int PulseWidth)
{
  int i;
  int j = 0;
  int LedToggle = 0;
  int Tick, OnTick;

  Uart->ModemOn = 0;
  Uart->ModemConnected = 0;

#ifdef DBG_MODEM
  fprintf(STDERR, "Uart_ModemOn (%d)\r\n", PulseWidth);
#endif

  USART6_Suppress(Uart, 0);// Disable USART/IF - Remove suprious loads and voltage that might prevent modem starting

Sleep(250);

#ifdef DBG_BRIEF
  Uart_DumpModemPinsBrief(Uart, " Initial");
#endif

  if ((Uart_ModemReadReset(Uart) == Bit_RESET) || // Modem Reset (200 ms)
      (Uart_ModemReadOnOff(Uart) == Bit_RESET))
  {
#ifdef DBG_MODEM
    fprintf(STDERR, "Modem reset released\r\n");
#endif

    Uart_ModemWriteReset(Uart, 1); // Modem Reset High
    Uart_ModemWriteOnOff(Uart, 1); // Modem OnOff High

    for(i=0; i<4; i++) // Sleep(1000)
    {
      Sleep(250);

      CurrentLED = (CurrentLED & ~1) | (j++ & 1);

#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " Z");
#endif
    }
  }

#if 0 // MODEM ALREADY ON
  if (Uart_ModemReadPower(Uart)) // Modem PwrMon
  {
#ifdef DBG_MODEM
    fprintf(STDERR, "Modem already On\r\n");
#endif
  }
  else
#endif // MODEM ALREADY ON
  {
    LedToggle = 0;

    // Pull OnOff Low for at least one second

    Uart_ModemWriteOnOff(Uart, 0); // Modem OnOff Low

#if 1 // BLINKY
    for(i=0; i<10; i++)
      {
#ifdef DBG_MODEM_PWR
        fprintf(STDERR, "#%c  - ",'A'+i);
#endif

      Sleep(PulseWidth / 10);

      CurrentLED = (CurrentLED & ~1) | (j++ & 1);

#ifdef DEBUG
      Uart_DebugModemPwrMon(Uart, 1);
#endif

#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " A");
#endif
    }
#else // BLINKY
    Sleep(PulseWidth); // At least on second
#endif

    Uart_ModemWriteOnOff(Uart, 1); // Modem OnOff High

    if (Uart_ModemReadReset(Uart) == Bit_RESET)
    {
#ifdef DBG_MODEM
      fprintf(STDERR, "Modem not present\r\n");
#endif

      return(0);
    }

    Tick = SystemTick;

    for(i=0; i<40; i++) // 10 Seconds
    {
#ifdef DBG_MODEM_PWR
      fprintf(STDERR, "#%2d - ",i);
      Uart_DebugModemPwrMon(Uart, 1);
#endif

#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " B");
#endif

      if (Uart_ModemReadPower(Uart)) // Leave early if ON
        break;

#if 0
        // Detect if the GSMLED has toggled

        if (GPIO_ReadInputDataBit(Uart->GSMLED_GPIO.Bank, Uart->GSMLED_GPIO.Pin))
          LedToggle |= 2;
        else
          LedToggle |= 1;

        if (LedToggle == 3)
          break;
#endif

      Sleep(250);

      CurrentLED = (CurrentLED & ~1) | (j++ & 1);
    }

fprintf(STDERR, "\r\nHere %d\r\n",(SystemTick - Tick)); // This should be 5-6 Seconds, beyond that we're done.
Uart_DebugModemPwrMon(Uart, 1);

  }

#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " F1");
#endif

  if (!Uart_ModemReadPower(Uart))
  {
#ifdef DBG_MODEM
    fprintf(STDERR, "Didn't power on right\n"); // It's dead Jim!
#endif

    return(0);
  }
  else
  {
    Sleep(100);

    USART6_Suppress(Uart, 1); // Enable Modem Pins

    Sleep(100);

    for(i=0; i<12; i++) // 3 Seconds
    {
#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " F2");
#endif

      if (Uart_ModemReadPwrMon(Uart) == Bit_RESET) // PWRMON drops
        break;

      Sleep(250);

      CurrentLED = (CurrentLED & ~1) | (j++ & 1);
    }

    if (Uart_ModemReadPwrMon(Uart) == Bit_RESET) // PWRMON dropped
    {
#ifdef DBG_MODEM
      printf("Didn't power on right\n");
#endif

      return(0);
    }

    for(i=0; i<12; i++) // 3 Seconds
    {
#ifdef DBG_BRIEF
      Uart_DumpModemPinsBrief(Uart, " F3");
#endif

      if (Uart_ModemReadDCD(Uart) == Bit_SET) // DCD Goes High when modem comes ready
        break;

      Sleep(250);

      CurrentLED = (CurrentLED & ~1) | (j++ & 1);
    }

    // Expect RING and DCD to go High, and CTS Low here if modem on and functioning

    if (Uart_ModemReadDCD(Uart) == Bit_RESET) // DCD Didn't go High
    {
#ifdef DBG_MODEM
      printf("Didn't power on right\n");
#endif

      return(0);
    }

//#ifdef DBG_MODEM
    fprintf(STDERR, "Modem is GO..\r\n"); // Thunderbirds are go...
//#endif

    Uart->ModemOnTick = SystemTick;
    Uart->ModemOn = 1;

    return(1);
  }
}

//******************************************************************************

void USART6_Suppress(UART *Uart, int Enable)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* 910 Modem wants all USART pins in undriven state to start */

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;

  /* Configure (DSR), (RING), and (DCD) as input */
  GPIO_InitStructure.GPIO_Pin = Uart->DSR_GPIO.Pin;
  GPIO_Init(Uart->DSR_GPIO.Bank, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = Uart->RING_GPIO.Pin;
  GPIO_Init(Uart->RING_GPIO.Bank, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = Uart->DCD_GPIO.Pin;
  GPIO_Init(Uart->DCD_GPIO.Bank, &GPIO_InitStructure);

  /* Configure USART6 Tx & Rx as alternate function  */
  if (Enable)
  {
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  }
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 |  GPIO_Pin_7;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* Configure (DTR) as output */
  if (Enable)
  {
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  }
  GPIO_InitStructure.GPIO_Pin = Uart->DTR_GPIO.Pin;
  GPIO_Init(Uart->DTR_GPIO.Bank, &GPIO_InitStructure);

  /* Configure (RTS) as output */
  if (Enable)
  {
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
#ifdef USE_RTS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
#else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
#endif
  }
  GPIO_InitStructure.GPIO_Pin = Uart->RTS_GPIO.Pin;
  GPIO_Init(Uart->RTS_GPIO.Bank, &GPIO_InitStructure);

  /* Configure PG.15 (CTS) as input */
  if (Enable)
  {
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
#ifdef USE_CTS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
#else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
#endif
  }
  GPIO_InitStructure.GPIO_Pin = Uart->CTS_GPIO.Pin;
  GPIO_Init(Uart->CTS_GPIO.Bank, &GPIO_InitStructure);
}

//******************************************************************************

void USART6_Configuration(UART *Uart)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  RCC_AHB1PeriphClockCmd(
    RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
    RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOG, ENABLE);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE); /* Enable USART6 Clock */

  Uart->Port = USART6;

  /* USART MODEM */

  // PC.03 (RESET*)
  Uart->RESET_GPIO.Bank = GPIOC;
  Uart->RESET_GPIO.Pin = GPIO_Pin_3;
  Uart->RESET_GPIO.PinSource = GPIO_PinSource3;

  // PC.02 (ON_OFF)
  Uart->ONOFF_GPIO.Bank = GPIOC;
  Uart->ONOFF_GPIO.Pin = GPIO_Pin_2;
  Uart->ONOFF_GPIO.PinSource = GPIO_PinSource2;

#ifdef REV2 // PG.00 (PWRMON)
  Uart->PWRMON_GPIO.Bank = GPIOG;
  Uart->PWRMON_GPIO.Pin = GPIO_Pin_0;
  Uart->PWRMON_GPIO.PinSource = GPIO_PinSource0;
#else // REV1 PC.13 (PWRMON)
  Uart->PWRMON_GPIO.Bank = GPIOC;
  Uart->PWRMON_GPIO.Pin = GPIO_Pin_13;
  Uart->PWRMON_GPIO.PinSource = GPIO_PinSource13;
#endif

#ifdef REV2 // PG.01 (GSMLED)
  Uart->GSMLED_GPIO.Bank = GPIOG;
  Uart->GSMLED_GPIO.Pin = GPIO_Pin_1;
  Uart->GSMLED_GPIO.PinSource = GPIO_PinSource1;
#else // REV1 PC.14 (GSMLED)
  Uart->GSMLED_GPIO.Bank = GPIOC;
  Uart->GSMLED_GPIO.Pin = GPIO_Pin_14;
  Uart->GSMLED_GPIO.PinSource = GPIO_PinSource14;
#endif

  // PG.15 (CTS)
  Uart->CTS_GPIO.Bank = GPIOG;
  Uart->CTS_GPIO.Pin = GPIO_Pin_15;
  Uart->CTS_GPIO.PinSource = GPIO_PinSource15;

  // PG.12 (RTS)
  Uart->RTS_GPIO.Bank = GPIOG;
  Uart->RTS_GPIO.Pin = GPIO_Pin_12;
  Uart->RTS_GPIO.PinSource = GPIO_PinSource12;

  // PD.03 (DSR)
  Uart->DSR_GPIO.Bank = GPIOD;
  Uart->DSR_GPIO.Pin = GPIO_Pin_3;
  Uart->DSR_GPIO.PinSource = GPIO_PinSource3;

  // PD.07 (RING)
  Uart->RING_GPIO.Bank = GPIOD;
  Uart->RING_GPIO.Pin = GPIO_Pin_7;
  Uart->RING_GPIO.PinSource = GPIO_PinSource7;

  // PD.08 (DCD)
  Uart->DCD_GPIO.Bank = GPIOD;
  Uart->DCD_GPIO.Pin = GPIO_Pin_8;
  Uart->DCD_GPIO.PinSource = GPIO_PinSource8;

  // PD.09 (DTR)
  Uart->DTR_GPIO.Bank = GPIOD;
  Uart->DTR_GPIO.Pin = GPIO_Pin_9;
  Uart->DTR_GPIO.PinSource = GPIO_PinSource9;

  GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6); /* Connect PC6 to USART6_Tx */
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6); /* Connect PC7 to USART6_Rx */
  GPIO_PinAFConfig(Uart->CTS_GPIO.Bank, Uart->CTS_GPIO.PinSource, GPIO_AF_USART6); /* Connect PG15 to USART6_CTS */
#ifdef USE_RTS
  GPIO_PinAFConfig(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.PinSource, GPIO_AF_USART6); /* Connect PG12 to USART6_RTS */
#endif

  USART6_Suppress(Uart, 0); // Disable outputs to modem

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
#ifdef USE_RTS
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
#else
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
#endif
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(Uart->Port, &USART_InitStructure); /* Configure USART */

  USART_Cmd(Uart->Port, ENABLE); /* Enable USART */

  // Modem Control

  /* Configure (PWRMON*) as input pulled-down */
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin = Uart->PWRMON_GPIO.Pin;
  GPIO_Init(Uart->PWRMON_GPIO.Bank, &GPIO_InitStructure);

  /* Configure (GSM_LED) as input pulled-down */
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin = Uart->GSMLED_GPIO.Pin;
  GPIO_Init(Uart->GSMLED_GPIO.Bank, &GPIO_InitStructure);

  /* Configure PC.04 (SERVICE) ??TODO */
  /* Configure PC.05 (ENABLE_VBUS) ??TODO */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* Configure PC.01 (ENABLE_SUPPLY) as output open-drain */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* Configure (ON_OFF) as output open-drain */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = Uart->ONOFF_GPIO.Pin;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(Uart->ONOFF_GPIO.Bank, &GPIO_InitStructure);

  /* Configure PC.03 (RESET*) as output open-drain */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD; //??
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; // Modem itself has pull-up, won't go high if no CF present
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(Uart->RESET_GPIO.Bank, &GPIO_InitStructure);

  /* Kill GPIO and USB pins to CF */
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_7 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
  GPIO_Init(GPIOG, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Set initial pin states */

  GPIO_ResetBits(GPIOC, GPIO_Pin_4);                            // Set SERVICE low
  GPIO_ResetBits(GPIOC, GPIO_Pin_5);                            // Set ENABLE_VBUS low
  GPIO_ResetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin);  // Set ON_OFF low
  GPIO_ResetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);  // Set RESET low
  GPIO_ResetBits(GPIOC, GPIO_Pin_1);                            // Set ENABLE_SUPPLY low
  Sleep(250);

  GPIO_SetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin);    // Set ON_OFF high
  GPIO_SetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);    // Set RESET high

  GPIO_SetBits(GPIOC, GPIO_Pin_1);                              // Set ENABLE_SUPPLY High - Plug_in terminal power supply is enabled
  Sleep(250);

#if 1
  GPIO_ResetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);  // Set RESET low
  Sleep(250);                                                   // Hold low for at least 200 ms
  GPIO_SetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);    // Set RESET high
  Sleep(250);
#endif
}

//****************************************************************************

void USART3_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  // Initialization of Terminus 2 (T2) RS232 port for debugging

  RCC_AHB1PeriphClockCmd(
    RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOF,
    ENABLE);

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); /* Enable USART3 Clock */

  /* Configure PF3 FORCEON, PF4 FORCEOFF - RS232 */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOF, &GPIO_InitStructure);

  GPIO_SetBits(GPIOF, GPIO_Pin_3 | GPIO_Pin_4);  // Both High

  /* USART RS232 */

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3); /* Connect PB10 to USART3_Tx */
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3); /* Connect PB11 to USART3_Rx */
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource11, GPIO_AF_USART3); /* Connect PD11 to USART3_CTS */
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_USART3); /* Connect PD12 to USART3_RTS */

  /* Configure USART3 Tx & Rx as alternate function  */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 |  GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Configure USART3 CTS & RTS as alternate function  */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 |  GPIO_Pin_12;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART3, &USART_InitStructure); /* Configure USART */

  USART_Cmd(USART3, ENABLE);  /* Enable the USART */
}

//******************************************************************************

void USART3_OutChar(char ch)
{
  while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);

  USART_SendData(USART3, ch);
}

//******************************************************************************

void USART3_OutString(char *s)
{
  while(*s)
    USART3_OutChar(*s++);
}

//******************************************************************************

/**
  * @brief  Program entry point
  * @param  None
  * @retval None
  */
int main(void)
{
  RCC_ClocksTypeDef RCC_Clocks;
  __IO uint32_t i = 0;  

  /*!< At this stage the microcontroller clock setting is already configured, 
  this is done through SystemInit() function which is called from startup
  file (startup_stm32fxxx_xx.s) before to branch to application main.
  To reconfigure the default setting of SystemInit() function, refer to
  system_stm32fxxx.c file
  */  
 
  /* SysTick end of count event each 1ms */
  RCC_GetClocksFreq(&RCC_Clocks);

  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

  USART3_Configuration(); // Debug via RS232

puts("T2 VCP Demo");

  USART6_Configuration(Uart6); // Modem for USB

  Uart_ModemOn(Uart6, TELIT_TIME_ON);

  USBD_Init(&USB_OTG_dev,
#ifdef USE_USB_OTG_HS 
            USB_OTG_HS_CORE_ID,
#else            
            USB_OTG_FS_CORE_ID,
#endif  
            &USR_desc, 
            &USBD_CDC_cb, 
            &USR_cb);
  
  /* Main loop */
  while (1)
  {
    if (i++ == 0x100000)
    {
      STM_EVAL_LEDToggle(LED1);
      STM_EVAL_LEDToggle(LED2);
      STM_EVAL_LEDToggle(LED3);
      i = 0;
    }
  }
} 

//******************************************************************************

// After Joseph Yiu's implementation

void hard_fault_handler_c(unsigned int * hardfault_args)
{
  printf ("[Hard Fault]\n");

  printf ("r0 = %08X, r1 = %08X, r2 = %08X, r3 = %08X\n",
    hardfault_args[0], hardfault_args[1], hardfault_args[2], hardfault_args[3]);
  printf ("r12= %08X, lr = %08X, pc = %08X, psr= %08X\n",
    hardfault_args[4], hardfault_args[5], hardfault_args[6], hardfault_args[7]);

  printf ("bfar=%08X, cfsr=%08X, hfsr=%08X, dfsr=%08X, afsr=%08X\n",
    *((volatile unsigned long *)(0xE000ED38)),
    *((volatile unsigned long *)(0xE000ED28)),
    *((volatile unsigned long *)(0xE000ED2C)),
    *((volatile unsigned long *)(0xE000ED30)),
    *((volatile unsigned long *)(0xE000ED3C)) );

  while(1);

  return;
}

//******************************************************************************
// Hosting of stdio functionality in Keil through USART3
//******************************************************************************

#include <rt_misc.h>

#pragma import(__use_no_semihosting_swi)

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f)
{
	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);

  USART_SendData(USART3, ch);

  return(ch);
}

int fgetc(FILE *f)
{
	char ch;

	while(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == RESET);

	ch = USART_ReceiveData(USART3);

  return((int)ch);
}

int ferror(FILE *f)
{
  /* Your implementation of ferror */
  return EOF;
}

void _ttywrch(int ch)
{
	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);

  USART_SendData(USART3, ch);
}

void _sys_exit(int return_code)
{
label:  goto label;  /* endless loop */
}

//******************************************************************************

#ifdef USE_FULL_ASSERT
/**
* @brief  assert_failed
*         Reports the name of the source file and the source line number
*         where the assert_param error has occurred.
* @param  File: pointer to the source file name
* @param  Line: assert_param error line source number
* @retval None
*/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  /* Infinite loop */
  while (1)
  {}
}
#endif

/**
  * @}
  */ 


/**
  * @}
  */ 


/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

