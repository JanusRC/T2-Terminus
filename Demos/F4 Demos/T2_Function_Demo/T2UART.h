//****************************************************************************
// Project Uart Library
//****************************************************************************
#include "stm32f4xx.h"
#include "RingBuffer.h"

#define MODEM_BAUD 115200
#define TERMINAL_BAUD 115200

#define FLOW_NONE   0
#define FLOW_CTS_HW 1
#define FLOW_RTS_HW 2
#define FLOW_CTS_SW 4
#define FLOW_RTS_SW 8

#define FLOW3 FLOW_NONE // RS232
#define FLOW6 (FLOW_CTS_HW | FLOW_RTS_SW) // MODEM

typedef struct _GPIO {
  GPIO_TypeDef* Bank;
  uint32_t Pin;
  uint32_t PinSource;
} GPIO;

//****************************************************************************
//Main UART structure, holds FIFO and I/O information for UART6 (modem)
//****************************************************************************
typedef struct _UART {

  USART_TypeDef *Port;  // STM32 Hardware for interface

  // Management and diagnostics

  int Flow;
  int Transmit;
  int Throttle;
  int Errors;
  int Overflow;
  int RxCount;
  int Busy;
  int TxCount;

  // FIFO Ring buffers

  FIFO Tx[1];
  FIFO Rx[1];

  // Modem state per interface

  int ModemOn;									//0 = Not Active, 1 = Active
  int ModemOnTick;							//Systick mark for turn on
	int ModemOffTick;							//Systick mark for turning off
	int ModemSIMReady;						//0 = Not ready, 1 = Ready - Only useful for SIM based modems
  int NetworkCellRegState; 			//Cellular: None, Home, Searching, Denied, N/A, Roaming
	int NetworkDataRegState; 			//Data: None, Home, Searching, Denied, N/A, Roaming
  int NetworkActivationState;		//Context Activation: 0 = Not Active, 1 = Active
	char NetworkIP[16];						//Ex: 255.255.255.255
	char NetworkName[16];					//Ex: Family Mobile
  int RSSI;											//Ex: 12
	int BER;											//Ex: 99
	int SocketOpen;								//Socket Status: 0 = Not Open, 1 = Open
	int SocketMode;								//Socket Type: 0 = Data/Online, 1 = Command
	int ConnID;										//Socket Listen Connection ID

  // Interface pins, watch that Uart_Open() clears this region

  GPIO TX_GPIO;
  GPIO RX_GPIO;
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

//****************************************************************************

//USART General Prototypes
void USART3_Configuration(UART *Uart);
void USART6_Configuration(UART *Uart);

UART *Uart_Open(UART *Uart, USART_TypeDef *Port);
void Uart_Close(UART *Uart);
void Uart_EmptyRx(UART *Uart);
void Uart_FillTx(UART *Uart, int Periodic);
void Uart_Enable(UART *Uart);
void Uart_Disable(UART *Uart);
void Uart_Throttle(UART *Uart);
void Uart_ForwardHalfDuplex(UART *UartA, UART *UartB);
void Uart_ForwardFullDuplex(UART *UartA, UART *UartB);
void Uart_SendBuffer(UART *Uart, DWORD Size, BYTE *Buffer);
void Uart_SendMessage(UART *Uart, const char *Buffer);
void Uart_Flush(UART *Uart);
char *Uart_GetLine(UART *Uart);

extern void Modem_WriteRTS(UART *Uart, int i);	//From main.h

////USART Modem Control Prototypes - TODO, change the flow to be out of the UART struct.
//int Modem_TurnOn(UART *Uart, int PulseWidth);
//int Modem_TurnOff(UART *Uart, int PulseWidth);
//void Modem_WriteOnOff(UART *Uart, int i);
//u8 Modem_ReadPwrMon(UART *Uart);
//u8 Modem_ReadOnOff(UART *Uart);
//u8 Modem_ReadGSMLED(UART *Uart);
//u8 Modem_ReadDSR(UART *Uart);
//u8 Modem_ReadRING(UART *Uart);
//u8 Modem_ReadDCD(UART *Uart);
//u8 Modem_ReadCTS(UART *Uart);
//u8 Modem_ReadRTS(UART *Uart);
//void Modem_WriteRTS(UART *Uart, int i);
//u8 Modem_ReadDTR(UART *Uart);
//void Modem_WriteDTR(UART *Uart, int i);
//u8 Modem_ReadReset(UART *Uart);
//void Modem_WriteReset(UART *Uart, int i);
//void Modem_DumpUARTPins(UART *Uart);
//void Modem_DumpPWRPins(UART *Uart);
//void Modem_Reset(UART *Uart);

void USART6_Suppress(UART *Uart, int Enable);


