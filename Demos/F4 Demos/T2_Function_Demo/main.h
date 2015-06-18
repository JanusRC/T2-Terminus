//****************************************************************************
//
// Main header file to contain all the typedef, #define, and main.c prototoypes
//
//****************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#include "stm32f4xx.h"
#include "T2UART.h"
#include "SMS.h"
#include "T2Common.h"

//****************************************************************************

#define APPLICATION_BASE 0x08020000
//#define USE_ITM

//****************************************************************************
//Debug
#define DUMP_MAX 0x300

#define DEBUG_UART_DUMP         0x00000010
#define DEBUG_UART_MSG          0x00000020
#define DEBUG_SOCKET            0x00000100
#define DEBUG_SOCKET_WAIT       0x00000200
#define DEBUG_SOCKET_RX         0x00000400
#define DEBUG_SOCKET_TX         0x00000800
#define DEBUG_SOCKET_MSG        0x00001000
#define DEBUG_VERBOSE           0x00002000
#define DEBUG_OPSTATE           0x00010000
#define DEBUG_FLASH             0x00020000
#define DEBUG_HTTP              0x00040000
#define DEBUG_MODEM             0x00080000
#define DEBUG_MODEM_POWER       0x00100000
#define DEBUG_LED               0x00200000
#define DEBUG_ADC               0x00400000
#define DEBUG_TERMINUS          0x00800000

//****************************************************************************
//Function Prototypes
void UART_Initialization(void);
void NVIC_Configuration(void);

int Terminus_Init(UART *Uart);

//****************************************************************************
//Prototypes
void GPIO_Configuration(void);
void WriteLED(int i);

//****************************************************************************
//Prototypes
void EnableTiming(void);
void TimingDelay(unsigned int tick);
int Sleep(int Delay);
int strnicmp(char *str1, char *str2, int n);
int stricmp(char *str1, char *str2);
char *DumpLine(DWORD Addr, DWORD Size, BYTE *Buffer);
void DumpData(UART *Uart, DWORD Size, BYTE *Buffer);
void DumpMemory(UART *Uart, DWORD Addr, DWORD Size, BYTE *Buffer);
int StackDepth(void);
int AddressValid(DWORD Addr);
void TerminalMode(int i, UART *UartDB9, UART *UartModem, SMSSTRUCT *SMSPointer);

void Debug_ITMDebugEnable(void);
void Debug_ITMDebugOutputChar(char ch);
void Debug_ITMDebugOutputString(char *Buffer);
void ITM_SendString(char *s);

extern void SystemReset(void);

//****************************************************************************
//Prototypes
//USART Modem Control Prototypes - TODO, change the flow to be out of the UART struct.
int Modem_TurnOn(UART *Uart, int PulseWidth);
int Modem_TurnOff(UART *Uart, int PulseWidth);
void Modem_WriteOnOff(UART *Uart, int i);
u8 Modem_ReadPwrMon(UART *Uart);
u8 Modem_ReadOnOff(UART *Uart);
u8 Modem_ReadGSMLED(UART *Uart);
u8 Modem_ReadDSR(UART *Uart);
u8 Modem_ReadRING(UART *Uart);
u8 Modem_ReadDCD(UART *Uart);
u8 Modem_ReadCTS(UART *Uart);
u8 Modem_ReadRTS(UART *Uart);
void Modem_WriteRTS(UART *Uart, int i);
u8 Modem_ReadDTR(UART *Uart);
void Modem_WriteDTR(UART *Uart, int i);
u8 Modem_ReadReset(UART *Uart);
void Modem_WriteReset(UART *Uart, int i);
void Modem_DumpUARTPins(UART *Uart);
void Modem_DumpPWRPins(UART *Uart);
void Modem_Reset(UART *Uart);

void Modem_WaitForPowerUp(UART *Uart);
int Modem_WaitForSIMReady(UART *Uart);
int Modem_GetPhoneNum(UART *Uart, char *outPN);

//****************************************************************************
//Prototypes
int ProbeModemResponse(char *s);
char *StripWhiteSpace(char *s);
int ModemToFaultCode(const char *s);

char *ReceiveAT(UART *Uart, int Timeout);
char *ReceiveATStr(UART *Uart, char *outString, int Timeout);
char *ReceiveATStrML(UART *Uart, char *outString, int Timeout);
int SendAT(UART *Uart, const char *s, int Timeout);
int SendATStr(UART *Uart, const char *s, char *outString, int Timeout);
int SendATStrML(UART *Uart, const char *s, char *outString, int Timeout);

//****************************************************************************
//Prototypes
int Network_WaitForCellReg(UART *Uart, int Timeout);
int Network_WaitForDataReg(UART *Uart, int Timeout);
int Network_CheckCellReg(UART *Uart);
int Network_CheckDataReg(UART *Uart);
int Network_GetSignalQuality(UART *Uart);
int Network_GetNetworkName(UART *Uart);
int Network_SetupContext(UART *Uart, char *inAPN);
int Network_GetContext(UART *Uart);
int Network_ActivateContext(UART *Uart, char *UserName, char *Password);
int Network_DeactivateContext(UART *Uart);

//****************************************************************************
//Prototypes
int Socket_Setup(UART *Uart, int SID);
int Socket_Dial(UART *Uart, int SID, char *IP, char *Port, int Mode);
int Socket_Listen(UART *Uart, int SID, char *Port);
int Socket_SetupFirewall(UART *Uart, char *IP, char *Mask);
int Socket_EnterCMDMode(UART *Uart);
int Socket_EnterDataMode(UART *Uart, int SID);
int Socket_Close(UART *Uart, int SID);
int Socket_Accept(UART *Uart, int CID);

//****************************************************************************
//Prototypes
int SMS_SetupSMS(UART *Uart);
int SMS_SendSMS(UART *Uart, SMSSTRUCT *SMS, char *Text, char *Phone);
int SMS_CheckSMS(UART *Uart, SMSSTRUCT *SMS);
int SMS_DeleteOneSMS(UART *Uart, int Selection);
int SMS_DeleteAllSMS(UART *Uart);
int SMS_ProcessSMS(UART *Uart, SMSSTRUCT *SMS, int selection);

//****************************************************************************
//Prototypes
void Demo_SerialBridge_Client(UART *UartDB9, UART *UartModem);
void Demo_SerialBridge_Host(UART *UartDB9, UART *UartModem);
void Demo_SMSEcho(UART *UartDB9, UART *UartModem, SMSSTRUCT *SMSPointer);

//****************************************************************************
//Prototypes
void FatTest(void);

