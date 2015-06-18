//****************************************************************************
// Project Uart Library
//****************************************************************************
#include "T2UART.h"

//****************************************************************************

extern int Sleep(int Delay);	//From Main

//****************************************************************************

//******************************************************************************
//UART Initializations
//******************************************************************************
//USART 3 - DB9
void USART3_Configuration(UART *Uart)
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
#ifdef REV2
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; // Pulled down
#else // REV1
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
#endif
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOF, &GPIO_InitStructure);

  GPIO_SetBits(GPIOF, GPIO_Pin_3 | GPIO_Pin_4);  // Both High

  /* USART RS232 */

  Uart->TX_GPIO.Bank = GPIOB; // PB10
  Uart->TX_GPIO.Pin = GPIO_Pin_10;
  Uart->TX_GPIO.PinSource = GPIO_PinSource10;

  Uart->RX_GPIO.Bank = GPIOB; // PB11
  Uart->RX_GPIO.Pin = GPIO_Pin_11;
  Uart->RX_GPIO.PinSource = GPIO_PinSource11;

  Uart->CTS_GPIO.Bank = GPIOD; // PD11
  Uart->CTS_GPIO.Pin = GPIO_Pin_11;
  Uart->CTS_GPIO.PinSource = GPIO_PinSource11;

  Uart->RTS_GPIO.Bank = GPIOD; // PD12
  Uart->RTS_GPIO.Pin = GPIO_Pin_12;
  Uart->RTS_GPIO.PinSource = GPIO_PinSource12;

  /* Define the AF function, we can still choose between AF/GPIO later */
  GPIO_PinAFConfig(Uart->TX_GPIO.Bank, Uart->TX_GPIO.PinSource, GPIO_AF_USART3); /* Connect PB10 to USART3_Tx */
  GPIO_PinAFConfig(Uart->RX_GPIO.Bank, Uart->RX_GPIO.PinSource, GPIO_AF_USART3); /* Connect PB11 to USART3_Rx */
  GPIO_PinAFConfig(Uart->CTS_GPIO.Bank, Uart->CTS_GPIO.PinSource, GPIO_AF_USART3); /* Connect PD11 to USART3_CTS */
  GPIO_PinAFConfig(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.PinSource, GPIO_AF_USART3); /* Connect PD12 to USART3_RTS */

  /* Configure USART3 Tx & Rx as alternate function  */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  GPIO_InitStructure.GPIO_Pin = Uart->TX_GPIO.Pin;
  GPIO_Init(Uart->TX_GPIO.Bank, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = Uart->RX_GPIO.Pin;
  GPIO_Init(Uart->RX_GPIO.Bank, &GPIO_InitStructure);

  /* Configure USART3 CTS as alternate function or input */

  if (Uart->Flow & FLOW_CTS_HW)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; // Defaults as input

  GPIO_InitStructure.GPIO_Pin = Uart->CTS_GPIO.Pin;
  GPIO_Init(Uart->CTS_GPIO.Bank, &GPIO_InitStructure);

  /* Configure USART3 RTS as alternate function or output */

  if (Uart->Flow & FLOW_RTS_HW)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  else
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; // Defaults as output

  GPIO_InitStructure.GPIO_Pin = Uart->RTS_GPIO.Pin;
  GPIO_Init(Uart->RTS_GPIO.Bank, &GPIO_InitStructure);

  GPIO_ResetBits(Uart->RTS_GPIO.Bank, GPIO_InitStructure.GPIO_Pin); // Defaults to low (Request To Send = Yes)

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;

  if (Uart->Flow & (FLOW_RTS_HW | FLOW_CTS_HW))
  {
    if ((Uart->Flow & (FLOW_RTS_HW | FLOW_CTS_HW)) == (FLOW_RTS_HW | FLOW_CTS_HW))
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
    else if (Uart->Flow & FLOW_RTS_HW)
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS;
    else
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
  }
  else
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART3, &USART_InitStructure); /* Configure USART */

  USART_Cmd(USART3, ENABLE);  /* Enable the USART */
}

//****************************************************************************
//USART 6 - Terminus Modem
//This contains not only the UART configuration, but also some of the hardware configuration too.
//TODO - Split this up, I want the discrete I/O for hardware in a different include
void USART6_Configuration(UART *Uart)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  RCC_AHB1PeriphClockCmd(
    RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOG,
    ENABLE);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE); /* Enable USART6 Clock */

	//****************
  // Modem UART
	//****************
  Uart->TX_GPIO.Bank = GPIOC; // PC6
  Uart->TX_GPIO.Pin = GPIO_Pin_6;
  Uart->TX_GPIO.PinSource = GPIO_PinSource6;

  Uart->RX_GPIO.Bank = GPIOC; // PC7
  Uart->RX_GPIO.Pin = GPIO_Pin_7;
  Uart->RX_GPIO.PinSource = GPIO_PinSource7;

  Uart->CTS_GPIO.Bank = GPIOG; // PG15
  Uart->CTS_GPIO.Pin = GPIO_Pin_15;
  Uart->CTS_GPIO.PinSource = GPIO_PinSource15;

  Uart->RTS_GPIO.Bank = GPIOG; // PG12
  Uart->RTS_GPIO.Pin = GPIO_Pin_12;
  Uart->RTS_GPIO.PinSource = GPIO_PinSource12;

  Uart->DSR_GPIO.Bank = GPIOD; // PD3
  Uart->DSR_GPIO.Pin = GPIO_Pin_3;
  Uart->DSR_GPIO.PinSource = GPIO_PinSource3;

  Uart->RING_GPIO.Bank = GPIOD; // PD7
  Uart->RING_GPIO.Pin = GPIO_Pin_7;
  Uart->RING_GPIO.PinSource = GPIO_PinSource7;

  Uart->DCD_GPIO.Bank = GPIOD; // PD8
  Uart->DCD_GPIO.Pin = GPIO_Pin_8;
  Uart->DCD_GPIO.PinSource = GPIO_PinSource8;

  Uart->DTR_GPIO.Bank = GPIOD; // PD9
  Uart->DTR_GPIO.Pin = GPIO_Pin_9;
  Uart->DTR_GPIO.PinSource = GPIO_PinSource9;

  //Define the AF function, we can still choose between AF/GPIO later
  GPIO_PinAFConfig(Uart->TX_GPIO.Bank, Uart->TX_GPIO.PinSource, GPIO_AF_USART6); /* Connect PC6 to USART6_Tx */
  GPIO_PinAFConfig(Uart->RX_GPIO.Bank, Uart->RX_GPIO.PinSource, GPIO_AF_USART6); /* Connect PC7 to USART6_Rx */
  GPIO_PinAFConfig(Uart->CTS_GPIO.Bank, Uart->CTS_GPIO.PinSource, GPIO_AF_USART6); /* Connect PG15 to USART6_CTS */
  GPIO_PinAFConfig(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.PinSource, GPIO_AF_USART6); /* Connect PG12 to USART6_RTS */

	// Hi-Z the modem pins
  USART6_Suppress(Uart, 0); 

  USART_InitStructure.USART_BaudRate = MODEM_BAUD;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;

 if (Uart->Flow & (FLOW_RTS_HW | FLOW_CTS_HW))
  {
    if ((Uart->Flow & (FLOW_RTS_HW | FLOW_CTS_HW)) == (FLOW_RTS_HW | FLOW_CTS_HW))
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
    else if (Uart->Flow & FLOW_RTS_HW)
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS;
    else
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
  }
  else
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  USART_Init(USART6, &USART_InitStructure); /* Configure USART */

  Uart_Enable(Uart); /* Enable USART */
	
	//****************

	//GPIO_InitTypeDef GPIO_InitStructure;
	
	//****************
  // Modem Control
	//****************
  // PC.03 (RESET*)
  Uart->RESET_GPIO.Bank = GPIOC;
  Uart->RESET_GPIO.Pin = GPIO_Pin_3;
  Uart->RESET_GPIO.PinSource = GPIO_PinSource3;

  // PC.02 (ON_OFF)
  Uart->ONOFF_GPIO.Bank = GPIOC;
  Uart->ONOFF_GPIO.Pin = GPIO_Pin_2;
  Uart->ONOFF_GPIO.PinSource = GPIO_PinSource2;

#ifdef REV2 // PG.00 (PWRMON*)
  Uart->PWRMON_GPIO.Bank = GPIOG;
  Uart->PWRMON_GPIO.Pin = GPIO_Pin_0;
  Uart->PWRMON_GPIO.PinSource = GPIO_PinSource0;
#else // REV1  PC.13 (PWRMON*)
  Uart->PWRMON_GPIO.Bank = GPIOC;
  Uart->PWRMON_GPIO.Pin = GPIO_Pin_13;
  Uart->PWRMON_GPIO.PinSource = GPIO_PinSource13;
#endif

#ifdef REV2 // PG.01 (GSM_LED)
  Uart->GSMLED_GPIO.Bank = GPIOG;
  Uart->GSMLED_GPIO.Pin = GPIO_Pin_1;
  Uart->GSMLED_GPIO.PinSource = GPIO_PinSource1;
#else // REV1  PC.14 (GSM_LED)
  Uart->GSMLED_GPIO.Bank = GPIOC;
  Uart->GSMLED_GPIO.Pin = GPIO_Pin_14;
  Uart->GSMLED_GPIO.PinSource = GPIO_PinSource14;
#endif
	
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

  /* Configure (ON_OFF) as output open-drain */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin =  Uart->ONOFF_GPIO.Pin;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(Uart->ONOFF_GPIO.Bank, &GPIO_InitStructure);

  GPIO_SetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin); // Default High

  /* Configure PC.05 (ENABLE_VBUS) ??TODO */
	//PC4 = Service (DNC)
	//PC5 = Enable VBUS
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

  /* Configure (RESET*) as output open-drain */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin = Uart->RESET_GPIO.Pin;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(Uart->RESET_GPIO.Bank, &GPIO_InitStructure);

  /* Kill GPIO and USB pins to CF */
	//Input with no pull up - HI-Z
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	
	//GPIO
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_7 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
  GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	//USB
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  //Set initial pin states
  //GPIO_ResetBits(GPIOC, GPIO_Pin_4);                            // Set SERVICE low
  //GPIO_ResetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin);  // Set ON_OFF low
  //GPIO_ResetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);  // Set RESET low
  //GPIO_ResetBits(GPIOC, GPIO_Pin_1);                            // Set ENABLE_SUPPLY low
  //Sleep(250);

  GPIO_SetBits(GPIOC, GPIO_Pin_5);                            	// Set ENABLE_VBUS High
  GPIO_SetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin);    // Set ON_OFF high
  GPIO_SetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);    // Set RESET high
  GPIO_SetBits(GPIOC, GPIO_Pin_1);                              // Set ENABLE_SUPPLY High - Plug_in terminal power supply is enabled
  Sleep(250);
	
	//Pulse RESET
  //GPIO_ResetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);  // Set RESET low
  //Sleep(250);                                                   // Hold low for at least 200 ms
  //GPIO_SetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);    // Set RESET high
  //Sleep(250);
}

void USART6_Suppress(UART *Uart, int Enable)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* 910 Modem wants all USART pins in undriven state to start */

  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;

  /* Configure (DSR), (RING), (DCD) as input */
  GPIO_InitStructure.GPIO_Pin = Uart->DSR_GPIO.Pin;
  GPIO_Init(Uart->DSR_GPIO.Bank, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = Uart->RING_GPIO.Pin;
  GPIO_Init(Uart->RING_GPIO.Bank, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = Uart->DCD_GPIO.Pin;
  GPIO_Init(Uart->DCD_GPIO.Bank, &GPIO_InitStructure);

  if (Enable) /* Configure USART6 Tx & Rx as alternate function */
  {
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  }

  GPIO_InitStructure.GPIO_Pin = Uart->TX_GPIO.Pin;
  GPIO_Init(Uart->TX_GPIO.Bank, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = Uart->RX_GPIO.Pin;
  GPIO_Init(Uart->RX_GPIO.Bank, &GPIO_InitStructure);

  if (Enable) /* Configure (DTR) as output */
  {
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  }

  GPIO_InitStructure.GPIO_Pin = Uart->DTR_GPIO.Pin;
  GPIO_Init(Uart->DTR_GPIO.Bank, &GPIO_InitStructure);

  if (Enable) /* Configure (RTS) as output */
  {
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    if (Uart->Flow & FLOW_RTS_HW)
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    else
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  }

  GPIO_InitStructure.GPIO_Pin = Uart->RTS_GPIO.Pin;
  GPIO_Init(Uart->RTS_GPIO.Bank, &GPIO_InitStructure);

  if (Enable) /* Configure (CTS) as input */
  {
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    if (Uart->Flow  & FLOW_CTS_HW)
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    else
      GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  }

  GPIO_InitStructure.GPIO_Pin = Uart->CTS_GPIO.Pin;
  GPIO_Init(Uart->CTS_GPIO.Bank, &GPIO_InitStructure);
}

//******************************************************************************
//Functions
//******************************************************************************

UART *Uart_Open(UART *Uart, USART_TypeDef *Port)
{
  memset(Uart, 0, sizeof(UART));

  Uart->Port = Port;

  Fifo_Open(Uart->Tx);
  Fifo_Open(Uart->Rx);

  return(Uart);
}

//****************************************************************************

void Uart_Close(UART *Uart)
{
  if (Uart)
  {
    Fifo_Close(Uart->Tx);
    Fifo_Close(Uart->Rx);

  // free(Uart); // dynamic implementation
  }
}

//****************************************************************************

#define USART_FLAG_ERRORS (USART_FLAG_ORE | USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE)

void Uart_EmptyRx(UART *Uart)
{
  USART_TypeDef *Port = Uart->Port;
  WORD status;

  status = Port->SR;

  while(status & (USART_FLAG_RXNE | USART_FLAG_ERRORS))
  {
    BYTE ch;

    ch = Port->DR;  // Read Data, or Clear Error(s)

//    if (USART_GetITStatus(Uart->Port, USART_IT_RXNE) != RESET)
//      USART_ClearITPendingBit(Uart->Port, USART_IT_RXNE); /* Clear the USART Receive interrupt */

    if (status & USART_FLAG_ERRORS)
      Uart->Errors++;
    else
    {
      Uart->RxCount++;

      if (Fifo_Insert(Uart->Rx, sizeof(ch), &ch) != sizeof(ch))
        Uart->Overflow++;
    }

    status = Port->SR;
  }

  if (USART_GetITStatus(Uart->Port, USART_IT_CTS) != RESET)
    USART_ClearITPendingBit(Uart->Port, USART_IT_CTS);  /* Clear the USART CTS interrupt */
}

//****************************************************************************

// Called by interrupt, and on periodic

void Uart_FillTx(UART *Uart, int Periodic)
{
  USART_TypeDef *Port = Uart->Port;
  WORD status;
  BYTE ch;

  status = Port->SR;

  if (status & USART_FLAG_ERRORS)
  {
    ch = Port->DR; // Clear the error

    Uart->Errors++;
  }

  if (Periodic && Uart->Transmit) // Don't try to prempt the interrupt
    return;

  if (status & USART_FLAG_TXE)
  {
    DWORD Used;

    Used = Fifo_Used(Uart->Tx);

    if (Used != 0)
    {
      Fifo_Extract(Uart->Tx, sizeof(ch), &ch);

//      Used--;

      Port->DR = ch;

      Uart->TxCount++;
    }

    if (Used == 0)
    {
      if ((Uart->Transmit == 1) || Periodic)
      {
        USART_ITConfig(Uart->Port, USART_IT_TXE, DISABLE); // Nothing to send, throttle

        Uart->Transmit = 0;
      }
    }
    else
    {
      if ((Uart->Transmit == 0) || Periodic)
      {
        USART_ITConfig(Uart->Port, USART_IT_TXE, ENABLE);

        Uart->Transmit = 1;
      }
    }
  }
}

//****************************************************************************

void Uart_Enable(UART *Uart)
{
  if (Uart->Port == USART1)
  {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); /* Enable USART1 Clock */
  }
  else if (Uart->Port == USART2)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); /* Enable USART2 Clock */
  }
  else if (Uart->Port == USART3)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); /* Enable USART3 Clock */
  }
  else if (Uart->Port == UART4)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE); /* Enable UART4 Clock */
  }
  else if (Uart->Port == UART5)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE); /* Enable UART5 Clock */
  }
  else if (Uart->Port == USART6)
  {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE); /* Enable USART6 Clock */
  }

  USART_Cmd(Uart->Port, ENABLE);  /* Enable the USART */

  Uart->Transmit = 1;

  USART_ITConfig(Uart->Port, USART_IT_TXE, ENABLE);

  USART_ITConfig(Uart->Port, USART_IT_RXNE, ENABLE);

  if (Uart->Flow & FLOW_CTS_HW)
    USART_ITConfig(Uart->Port, USART_IT_CTS, ENABLE); /* Interupt on CTS changing state */
}

//****************************************************************************

void Uart_Disable(UART *Uart)
{
  Uart->Transmit = 0;

  USART_ITConfig(Uart->Port, USART_IT_TXE, DISABLE);

  USART_ITConfig(Uart->Port, USART_IT_RXNE, DISABLE);

  if (Uart->Flow & FLOW_CTS_HW)
    USART_ITConfig(Uart->Port, USART_IT_CTS, DISABLE);

  USART_Cmd(Uart->Port, DISABLE);  /* Disable the USART */

  if (Uart->Port == USART1)
  {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE); /* Disable USART1 Clock */
  }
  else if (Uart->Port == USART2)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, DISABLE); /* Disable USART2 Clock */
  }
  else if (Uart->Port == USART3)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, DISABLE); /* Disable USART3 Clock */
  }
  else if (Uart->Port == UART4)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, DISABLE); /* Disable UART4 Clock */
  }
  else if (Uart->Port == UART5)
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, DISABLE); /* Disable UART5 Clock */
  }
  else if (Uart->Port == USART6)
  {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, DISABLE); /* Disable USART6 Clock */
  }
}

//****************************************************************************

void Uart_Throttle(UART *Uart)
{
  if (Uart->Flow & FLOW_CTS_HW)
    if (USART_GetITStatus(Uart->Port, USART_IT_CTS) != RESET) // Acknowledge any CTS
      USART_ClearITPendingBit(Uart->Port, USART_IT_CTS);  /* Clear the USART CTS interrupt */

#define RTS_THRESH 32

  if (Uart->Flow & FLOW_RTS_SW) // Pin in Software
  {
    if (Uart->Busy || (Fifo_Free(Uart->Rx) < RTS_THRESH)) // Manage flow control in software
  {
      if (Uart->Throttle != 1)
    {
      Uart->Throttle = 1;

        Modem_WriteRTS(Uart, Uart->Throttle);
    }
  }
  else
  {
      if ((!Uart->Busy) && (Uart->Throttle) && (Fifo_Used(Uart->Rx) < RTS_THRESH))
  //    if ((!Uart->Busy) && (Uart->Throttle) && (Fifo_Free(Uart->Rx) > RTS_THRESH))
    {
      Uart->Throttle = 0;

        Modem_WriteRTS(Uart, Uart->Throttle);
      }
    }
  }
}

//****************************************************************************

void Uart_ForwardHalfDuplex(UART *UartA, UART *UartB)
{
  Fifo_Forward(UartA->Rx, UartB->Tx);
}

//****************************************************************************

void Uart_ForwardFullDuplex(UART *UartA, UART *UartB)
{
  Uart_ForwardHalfDuplex(UartA, UartB);
  Uart_ForwardHalfDuplex(UartB, UartA);
}

//****************************************************************************

void Uart_SendBuffer(UART *Uart, DWORD Size, BYTE *Buffer)
{
  DWORD Length;
  DWORD Free;

  while(Size)
  {
    Free = Fifo_Free(Uart->Tx);

    Length = min(Free, Size);

    if (Length && Fifo_Insert(Uart->Tx, Length, Buffer))
    {
      Size   -= Length;
      Buffer += Length;
    }
    else
      Sleep(1);
  }
}

//****************************************************************************

void Uart_SendMessage(UART *Uart, const char *Buffer)
{
  Uart_SendBuffer(Uart, (DWORD)strlen(Buffer), (BYTE *)Buffer);
}

//****************************************************************************

void Uart_Flush(UART *Uart)
{
  while(Fifo_Used(Uart->Tx)) // Wait for FIFO to flush
    Sleep(100);
}

//****************************************************************************

char *Uart_GetLine(UART *Uart)
{
  static char InputLine[256];
  int i; //, j;
  int Echo;

  Echo = 1;

  //j = 0;

  i = 0;

  while(1)
  {
    char x;

    x = Fifo_GetChar(Uart->Rx); // Key strokes on UART/Debug Console

    if ((x == '\n') || (x == '\r'))
    {
//      printf("[%02X]",x);

      if (x == '\r') // 0x0D (RETURN/ENTER), we will eat 0x0A
      {
        if (Echo)
          Fifo_Insert(Uart->Tx, 2, "\r\n"); // CR/LF

        break;
      }
    }
    else if ((x == 0x08) || (x == 0x7F))
    {
      if (i)
      {
        i--;

        if (Echo)
          Fifo_Insert(Uart->Tx, 3, "\010 \010"); // Backspace/Erase
      }
    }
    else if (x)
    {
      if ((i == 0) && ((x == 'S') || (x == ':'))) // Hex Lines
        Echo = 0;

      InputLine[i++] = x;

      if (Echo)
        Fifo_Insert(Uart->Tx, 1, (BYTE *)&x); // Echo to console
    }

		//Sleep(25); // Lower power than grinding

		//ITM_SendChar('C');

    //if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_15)) /* PE15 INVALID - RS232 Connected */
    //  WriteLED(1);
    //else
    //  WriteLED(0);
  }

  InputLine[i] = 0;

  return(InputLine);
}
