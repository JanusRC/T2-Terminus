//****************************************************************************
//
//  TERMINUS2 Functionality Demonstration Application - Keil 4.x
//
//  Clive Turvey - Connor Winfield Corp (Janus Remote Communications Group)
//     cturvey@conwin.com  cturvey@janus-rc.com  cturvey@gmail.com
//
//	Clayton Knight - cknight@janus-rc.com
//
//****************************************************************************
//
// Modified system_stm32f4xx.c
//
//  *        HSE Frequency(Hz)                      | 16000000
//  *        PLL_M                                  | 16
//  #define PLL_M      16
//
//****************************************************************************

#include "main.h"
#include "Modem.h"
#include "ATC.h"

//****************************************************************************

unsigned Debug =
// DEBUG_TERMINUS |
//    DEBUG_SOCKET |
//    DEBUG_SOCKET_TX |
//    DEBUG_SOCKET_RX |
//    DEBUG_SOCKET_WAIT |
//    DEBUG_FLASH |
0 ;

//****************************************************************************
//Modem Characteristics
char ModemMake[40];			//CGMI	-	Manufacturer Information
char ModemModel[40];		//CGMM	-	Modem Type
char ModemFirmware[40];	//CGMR	-	Modem Firmware
char ModemSerial[40]; 	//IMEI	- International Mobile Equipment Identity, modem serial number

//SIM Information
char SIMSerial[40]; 		//IMSI	- International Mobile Subscriber Identity
char SIMIdentity[40]; 	//ICCID	- Integrated Circuit Card Identification
char SIMPNumber[40]; 		//CNUM	- SIM Given phone number

//******************************************************************************
//Application variables

//General use
volatile int CurrentLED;	//Storage for the LED mask

//****************************************************************************
// Timing
//****************************************************************************
volatile int SystemTick = 0; // 1 ms ticker

extern void * __Vectors;

u8 SysTickPriority;

void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  u8 Priority;

  NVIC_SetVectorTable((u32)(&__Vectors), 0x0); // Smart Base Location

  /* Configure the NVIC Preemption Priority Bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;

  Priority = 0;

  /* Enable the USART3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = Priority++;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the USART6 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = Priority++;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = Priority++;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

//  NVIC_InitStructure.NVIC_IRQChannel = SD_SDIO_DMA_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = Priority++;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);

  SysTickPriority = Priority++;
}

//****************************************************************************
// SMS Initialization
//****************************************************************************

//Create an SMS type structure for our SMS container
static SMSSTRUCT SMSInfo[1];

//Create a pointer to the structure for easy use/reference
static SMSSTRUCT *SMSPointer = SMSInfo;

//****************************************************************************
// BEGIN UART Initialization & Handling
//****************************************************************************

//Create structures for the two main UARTs being used
static UART Uart3[1]; // RS232 DB9
static UART Uart6[1]; // Modem

//Create pointers to structures for easy use/reference
static UART *UartDB9 = Uart3;
static UART *UartModem = Uart6;

//****************************************************************************

void UART_Initialization(void)
{
  // Called prior to hardware initialization
  Uart_Open(Uart3,USART3);

#ifdef FLOW3
  Uart3->Flow = FLOW3;
#endif

  Uart_Open(Uart6,USART6);

#ifdef FLOW6
  Uart6->Flow = FLOW6;
#endif
}

//****************************************************************************
//Main area Functions
//1ms Ticker Function
void PeriodicUART(int i)
{
	//DB9-RS232
  if (i == (1 << (3 - 1))) 
  {
		//Handle the DB9 TX/RX
    Uart_EmptyRx(Uart3);		//Empty the RX register and stuff into FIFO
    Uart_FillTx(Uart3, 0);	//Empty the FIFO TX and stuff into the TX register

    Uart_Throttle(Uart3); 	// Flow Control
  }
	//Modem
  else if (i == (1 << (6 - 1))) 
  {
		//Handle the Modem TX/RX
    Uart_EmptyRx(Uart6);		//Empty the RX register and stuff into FIFO
    Uart_FillTx(Uart6, 0);	//Empty the FIFO TX and stuff into the TX register

    Uart_Throttle(Uart6); 	// Flow Control
  }
	//General Handling
  else if (i == 0) 
  {
		/***********
		UART
		***********/
		//Send all FIFO inserted data to the respecitve ports
    Uart_FillTx(Uart3, 1);	//Empty the FIFO TX and stuff into the TX register
    Uart_FillTx(Uart6, 1);	//Empty the FIFO TX and stuff into the TX register

    Uart_Throttle(Uart3); 	// Flow Control
    Uart_Throttle(Uart6); 	// Flow Control

		/***********
		LEDs
		***********/
		//Pass the desired continual read data to a respective external LED
		//GSM Status LED Update
		if (Modem_ReadGSMLED(UartModem) == Bit_SET)
      CurrentLED |= 2; // YELLOW
    else
      CurrentLED &= ~2;

		/***********
		PWRMON Check
		***********/
		//Read the PWRMON signal and update the structure for application checking
		if (Modem_ReadPwrMon(UartModem) == Bit_SET)
		{
      CurrentLED |= 4; // GREEN
			UartModem->ModemOn = 1;
		}
    else
		{
      CurrentLED &= ~4;
			UartModem->ModemOn = 0;
		}

    WriteLED(CurrentLED);

		/***********
		Socket Status
		***********/
		//Check DCD state for main application socket checks
		//Read DCD, pass back the opposite of the UART state
		//1 = Unasserted	=	Not Open 
		//0 = Asserted 		=	Open
		UartModem->SocketOpen = !Modem_ReadDCD(UartModem);
		
  }
}

//****************************************************************************

void SimpleForwardingLoop(void)
{
  printf("\r\nForwarding USART3 (DB9 - 115200) to USART6 (GSM MODEM %d)\r\n",MODEM_BAUD);

	//Flush Debug Port
	while(Fifo_Used(UartDB9->Tx)) // Wait for debug FIFO to flush
    Sleep(100);

  while(1) /* Infinite loop */
  {
    __WFI();

    Uart_ForwardFullDuplex(UartModem, UartDB9); // Modem to/from Debug
  }
}

//****************************************************************************
// END UART Initialization & Handling
//****************************************************************************

//****************************************************************************
// BEGIN Modem Initialization
//****************************************************************************
int Terminus_Init(UART *Uart)
{
  int Fault = FAULT_OK;	//Initialize Fault State = 0
	
	//Initialize the modem information
	memset(ModemMake, 0, sizeof(ModemMake));
	memset(ModemModel, 0, sizeof(ModemModel));
	memset(ModemFirmware, 0, sizeof(ModemFirmware));
	memset(ModemSerial, 0, sizeof(ModemSerial));
	
	//Initialize the SIM information
	memset(SIMSerial, 0, sizeof(SIMSerial));
	memset(SIMIdentity, 0, sizeof(SIMIdentity));
	memset(SIMPNumber, 0, sizeof(SIMPNumber));

	//Empty any clutter from the Fifo
  Fifo_Flush(Uart->Rx); 

	//Disable echo to keep ATC handler happy
  if (!Fault) Fault = SendAT(Uart, "ATE0\r\n", ATC_STD_TIMEOUT); 

	//Quick check for UART responsiveness
  if (Fault == FAULT_TIMEOUT)
  {
    fprintf(STDERR, "Error: Modem not responsive\n");
    return(FAULT_MODEM_NOT_CONNECTED);
  }
	
	//*********************************
	// Fill in Modem Information
	//*********************************	
  if (!Fault) Fault = SendATStr(Uart, "AT+CGMI\r\n", ModemMake, ATC_STD_TIMEOUT);
  if (!Fault) Fault = SendATStr(Uart, "AT+CGMM\r\n", ModemModel, ATC_STD_TIMEOUT);
  if (!Fault) Fault = SendATStr(Uart, "AT+CGMR\r\n", ModemFirmware, ATC_STD_TIMEOUT);
	if (!Fault) Fault = SendATStr(Uart, "AT+CGSN\r\n", ModemSerial, ATC_STD_TIMEOUT);
	
	//Stop and respond with fault information if necessary
	if (Fault)
	{
		printf("Error: Gathering Modem Information: %08X\n",Fault);
		return (Fault);
	}
	
	//*********************************
	// General Set up commands to help 
	// facilitate everything else
	//*********************************
	//TODO - Adjust this to check instead of just shotgun set. ENS doesn't apply to CDMA/EVDO
  if (!Fault) Fault = SendAT(Uart, "AT+CMEE=2\r\n", ATC_STD_TIMEOUT);
	if (!Fault) Fault = SendAT(Uart, "AT#SELINT=2\r\n", ATC_STD_TIMEOUT);
	
	if (strncmp(ModemModel,"HE910",5) == 0) 
		if (!Fault) Fault = SendAT(Uart, "AT#ENS=1\r\n", ATC_STD_TIMEOUT); // North American

	//Stop and respond with fault information if necessary
	if (Fault)
	{
		printf("Error: General Settings: %08X\n",Fault);
		return (Fault);
	}

	//*********************************
	// Get SIM Information if applicable
	// Usable with GSM/HSPA/LTE based modems
	// HSPA910CF, GSM865CF, LTE910CF
	//*********************************		
	if (strncmp(ModemModel,"HE910",5) == 0) 
	{
		//Wait for SIM to be ready
		if (!Fault) Fault = Modem_WaitForSIMReady(Uart);
		
		if (Fault)
		{
			printf("SIM Error: %08X\n",Fault);
			return (Fault);
		}

		//SIM Ready, gather usable/notable information
		Fault = Modem_GetPhoneNum(Uart, SIMPNumber);

		Fault = SendATStr(Uart, "AT#CIMI\r\n", SIMSerial, 8000);
		if (!Fault)
		{
			// #CIMI: 310260584850247
			if (strncmp(SIMSerial,"#CIMI: ",7) == 0) 
				strcpy(SIMSerial,&SIMSerial[7]);
		}

		Fault = SendATStr(Uart, "AT#CCID\r\n", SIMIdentity, 8000);
		if (!Fault)
		{
			// #CCID: 8901260580048502477
			if (strncmp(SIMIdentity,"#CCID: ",7) == 0) 
				strcpy(SIMIdentity,&SIMIdentity[7]);
		}
		
		if (Fault)
		{
			printf("Error: Gathering SIM Information: %08X\n",Fault);
			return (Fault);
		}
		
	}//End HE910 IF
	
	//*********************************
	// Misc. Setup commands
	//*********************************	
	//Let's spit out the modem and SIM information
	printf("Modem Information\r\n");
	printf("------------------------------\r\n");
	fprintf(STDERR, "Make:      %s\r\n", ModemMake);
	fprintf(STDERR, "Model:     %s\r\n", ModemModel);
	fprintf(STDERR, "Serial:    %s\r\n", ModemSerial);
	fprintf(STDERR, "Firmware:  %s\r\n", ModemFirmware);
	
	if (strncmp(ModemModel,"HE910",5) == 0) 
	{
		printf("---------------\r\n");
		printf("SIM Information\r\n");
		fprintf(STDERR, "Serial:    %s\r\n", SIMSerial);
		fprintf(STDERR, "ICCID:     %s\r\n", SIMIdentity);
		fprintf(STDERR, "Phone #:   %s\r\n", SIMPNumber);
	}
	printf("------------------------------\r\n\r\n");
	
  return(Fault);
}

//****************************************************************************
// END Modem Initialization
//****************************************************************************

//****************************************************************************
// BEGIN Main Program Entry
//****************************************************************************
int main(void)
{
	int Fault = FAULT_OK;		//Initial fault condition
	//char *String;						//Local string buffer
	
  RCC_ClocksTypeDef RCC_Clocks;

  /*!< At this stage the microcontroller clock setting is already configured,
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f4xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f4xx.c file
     */

	//Need to initialize these structures really early
  UART_Initialization(); 

	//Set up timing
  NVIC_Configuration();
  EnableTiming();

#ifdef USE_ITM
  Debug_ITMDebugEnable();
  Debug_ITMDebugOutputString("SWV Enabled\n");
#endif

  //Set up SysTick count event, every 1ms */
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
  NVIC_SetPriority(SysTick_IRQn, SysTickPriority);

	//Set up GPIO
  GPIO_Configuration(); 

	//Set up UARTs
  USART3_Configuration(Uart3);	//Set up DB9 USART
  USART6_Configuration(Uart6);	//Set up modem USART

	//USART6 supressed, I/O HI-Z (ready)

	//Enable the interrupts
  Uart_Enable(Uart3); 
  Uart_Enable(Uart6);
	
	//Welcome message
	printf("\n\n\nJanus Remote Communications - TERMINUS 2 - Basic Demonstration\nFW: %s %s\n\n", __DATE__, __TIME__);

	//*********************************
	//Run the terminal first - Allots 5 seconds to enter command
	//*********************************
	TerminalMode(5, UartDB9, UartModem, SMSPointer);

	//*********************************
	// Start the modem up
	//*********************************
  if (!UartModem->ModemOn)
  {
    printf("Starting Modem.\r\n");
    Modem_TurnOn(UartModem, TELIT_TIME_ON);
		Modem_WaitForPowerUp(UartModem); // Wait for modem to warm up
  }
	
  if (UartModem->ModemOn)
  {
		printf("Modem Initialization\r\n\r\n");
		
		//*********************************
		// Initialize the modem
		// This will run basic setup/init commands
		//*********************************
    Fault = Terminus_Init(UartModem); //Init routine
		
    if (Fault) 
			printf("Modem not initialized\r\n");
		else
		{
			//*********************************
			// Initialize the network
			// Check registration, signal quality, prints information
			//*********************************
			printf("Network Initializing\r\n\r\n");
			if (!Fault) Fault = Network_WaitForCellReg(UartModem, NET_REG_TIMEOUT);	//Cellular Registration
			if (!Fault) Fault = Network_WaitForDataReg(UartModem, NET_REG_TIMEOUT);	//Data Registration/readiness
			if (!Fault) Fault = Network_GetSignalQuality(UartModem);								//Signal Quality Check

			if (!Fault && (strncmp(ModemModel,"HE910",5) == 0)) 
				Fault = Network_GetNetworkName(UartModem);														//Network Name

			if (Fault) 
				printf("Network not Ready\r\n");
			else
			{
				printf("Network Information\r\n");
				printf("------------------------------\r\n");
				fprintf(STDERR, "Cell Reg:   %i\r\n", UartModem->NetworkCellRegState);
				fprintf(STDERR, "Data Reg:   %i\r\n", UartModem->NetworkDataRegState);
				fprintf(STDERR, "RSSI:       %i (%d dBm)\r\n", UartModem->RSSI, (-113 + (UartModem->RSSI * 2)));
				fprintf(STDERR, "BER:        %i\r\n", UartModem->BER);
				fprintf(STDERR, "Network:    %s\r\n", UartModem->NetworkName);
				printf("------------------------------\r\n\r\n");
			}
		}
	}//ModemOn If End
	
	//*********************************
	// User Code Space BEGIN
	//*********************************
	
	//Uncomment the demo you wish to run
	
	//Run the serial bridge client demo
	//Requires a listening host via netcat or similar
	//printf("Running Serial Bridge - Client - Demo\r\n");
	//Demo_SerialBridge_Client(UartDB9, UartModem);
	
	//Run the serial bridge host demo
	//Requires a dialing client via netcat or similar
	//printf("Running Serial Bridge - Host - Demo\r\n");
	//Demo_SerialBridge_Host(UartDB9, UartModem);
	
	//Run the SMS Echo demo
	//Requires an SMS ready account/SIM
	//printf("Running SMS Echo Demo\r\n");
	//Demo_SMSEcho(UartDB9, UartModem, SMSPointer);

	
	//Run forwarding loop
  SimpleForwardingLoop();
	
  while(1); /* Infinite loop */
}

//****************************************************************************
// END Main Program Entry
//****************************************************************************


//****************************************************************************
// BEGIN Hosting of stdio functionality through USART3
//****************************************************************************

#include <rt_misc.h>

#pragma import(__use_no_semihosting_swi)

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;

static void __outchar(char ch)
{
  while(!Fifo_Insert(UartDB9->Tx, 1, (void *)&ch)) Sleep(10);
}

static char __inchar(void)
{
  char ch;

  while(!Fifo_Extract(UartDB9->Rx, 1, (void *)&ch)) Sleep(10);

  return(ch);
}

int fputc(int ch, FILE *f)
{
  static int last;

#ifdef USE_ITM
  if (f == STDERR)
  {
    Debug_ITMDebugOutputChar(ch);
    return(ch);
  }
#endif // USE_ITM

  if ((ch == (int)'\n') && (last != (int)'\r'))
    __outchar('\r');

    last = ch;

  __outchar((char)ch);

  return(ch);
}

int fgetc(FILE *f)
{
  return((int)__inchar());
}

int ferror(FILE *f)
{
  /* Your implementation of ferror */
  return EOF;
}

void _ttywrch(int ch)
{
  static int last;

  if ((ch == (int)'\n') && (last != (int)'\r'))
    __outchar('\r');

    last = ch;

  __outchar((char)ch);
}

void _sys_exit(int return_code)
{
label:  goto label;  /* endless loop */
}

//****************************************************************************

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  while(1); /* Infinite loop */
}
#endif

//****************************************************************************
// END Hosting of stdio functionality through USART3
//****************************************************************************

