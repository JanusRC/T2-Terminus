//****************************************************************************
// Terminus Modem Library
//****************************************************************************
//#include "T2UART.h"	//Eventually this will be moved away from the UART struct to here
#include "Modem.h"
#include "main.h"
#include "ATC.h"

//****************************************************************************

extern volatile int SystemTick;	//From Main
extern volatile int CurrentLED;
extern int Sleep(int Delay);	//From Main
extern void WriteLED(int i);
extern unsigned Debug;					//From Main

//****************************************************************************

//#define DEBUG
//#define DBG_MODEM
//#define DBG_BRIEF

//****************************************************************************
// Modem Hardware Stuff
//****************************************************************************

//***************************
// Discrete I/O Calls
//***************************
u8 Modem_ReadPwrMon(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->PWRMON_GPIO.Bank, Uart->PWRMON_GPIO.Pin)); // Modem PwrMon
}

//****************************************************************************

u8 Modem_ReadOnOff(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin)); // Modem OnOff
}

//****************************************************************************

void Modem_WriteOnOff(UART *Uart, int i)
{
  if (i)
    GPIO_SetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin);    // Modem OnOff High
  else
    GPIO_ResetBits(Uart->ONOFF_GPIO.Bank, Uart->ONOFF_GPIO.Pin);  // Modem OnOff Low
}

//****************************************************************************

u8 Modem_ReadGSMLED(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->GSMLED_GPIO.Bank, Uart->GSMLED_GPIO.Pin)); // Modem LED
}

//****************************************************************************

u8 Modem_ReadDSR(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->DSR_GPIO.Bank, Uart->DSR_GPIO.Pin)); // Modem DSR
}

//****************************************************************************

u8 Modem_ReadRING(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->RING_GPIO.Bank, Uart->RING_GPIO.Pin)); // Modem RING
}

//****************************************************************************

u8 Modem_ReadDCD(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->DCD_GPIO.Bank, Uart->DCD_GPIO.Pin)); // Modem DCD
}

//****************************************************************************

u8 Modem_ReadCTS(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->CTS_GPIO.Bank, Uart->CTS_GPIO.Pin)); // Modem CTS
}

//****************************************************************************

u8 Modem_ReadRTS(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.Pin)); // Modem RTS
}

//****************************************************************************

void Modem_WriteRTS(UART *Uart, int i)
{
  if (i)
    GPIO_SetBits(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.Pin);   // RTS High
  else
    GPIO_ResetBits(Uart->RTS_GPIO.Bank, Uart->RTS_GPIO.Pin); // RTS Low
}

//****************************************************************************

u8 Modem_ReadDTR(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->DTR_GPIO.Bank, Uart->DTR_GPIO.Pin)); // Modem DTR
}

//****************************************************************************

void Modem_WriteDTR(UART *Uart, int i)
{
  if (i)
    GPIO_SetBits(Uart->DTR_GPIO.Bank, Uart->DTR_GPIO.Pin);   // DTR High
  else
    GPIO_ResetBits(Uart->DTR_GPIO.Bank, Uart->DTR_GPIO.Pin); // DTR Low
}

//****************************************************************************

u8 Modem_ReadReset(UART *Uart)
{
  return(GPIO_ReadInputDataBit(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin)); // Modem Reset
}

//****************************************************************************

void Modem_WriteReset(UART *Uart, int i)
{
  if (i)
    GPIO_SetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);    // Modem Reset High
  else
    GPIO_ResetBits(Uart->RESET_GPIO.Bank, Uart->RESET_GPIO.Pin);  // Modem Set Low
}

//****************************************************************************

// Display the modem status lines for diagnostic purposes

void Modem_DumpUARTPins(UART *Uart)
{
  if (Modem_ReadDSR(Uart) == Bit_SET)
    printf("DSR  Hi (I),");
  else
    printf("DSR  Lo (I),");

  if (Modem_ReadRING(Uart) == Bit_SET)
    printf(" RING Hi (I),");
  else
    printf(" RING Lo (I),");

  if (Modem_ReadDCD(Uart) == Bit_SET)
    printf(" DCD  Hi (I),");
  else
    printf(" DCD  Lo (I),");

  if (Modem_ReadDTR(Uart) == Bit_SET)
    printf(" DTR  Hi (O),");
  else
    printf(" DTR  Lo (O),");

  if (Modem_ReadCTS(Uart) == Bit_SET)
    printf(" CTS  Hi (I),");
  else
    printf(" CTS  Lo (I),");

  if (Modem_ReadRTS(Uart) == Bit_SET)
    printf(" RTS  Hi (O)\r\n");
  else
    printf(" RTS  Lo (O)\r\n");
}

//****************************************************************************

void Modem_DumpPWRPins(UART *Uart)
{
  if (Modem_ReadPwrMon(Uart) == Bit_SET)
    printf("PWRMON Hi (I),");
  else
    printf("PWRMON Lo (I),");

  if (Modem_ReadReset(Uart) == Bit_SET)
    printf(" RESET Hi (O),");
  else
    printf(" RESET Lo (O),");

  if (Modem_ReadOnOff(Uart) == Bit_SET)
    printf(" ON_OFF Hi (O)\r\n");
  else
    printf(" ON_OFF Lo (O)\r\n");
}

//****************************************************************************

//***************************
// Higher level functions
//***************************

void Modem_Reset(UART *Uart)
{
  Modem_WriteReset(Uart, 0);	// Modem Reset Low
  Sleep(250);         						// At least 200 ms
  Modem_WriteReset(Uart, 1);	// Modem Reset High
  Sleep(250);											// At least 200 ms
}

//****************************************************************************

int Modem_TurnOn(UART *Uart, int PulseWidth)
{
  int i; //Initialize temp variable

	//Initialize structure variables
  Uart->ModemOn = 0;

	// Disable USART/IF - Remove suprious loads and voltage that might prevent modem starting
  USART6_Suppress(Uart, 0);

	//Short pause before beginning
  Sleep(100);

	//Quick check if either reset or ON/OFF are currently being held LOW
	//If they are, release them to hi-z state.
  if ((Modem_ReadReset(Uart) == Bit_RESET) || (Modem_ReadOnOff(Uart) == Bit_RESET))
  {
    Modem_WriteReset(Uart, 1); // Modem Reset High
    Modem_WriteOnOff(Uart, 1); // Modem OnOff High
    Sleep(250);	//Short delay to let the modem stabilize.
  }
	
	//Quick double check to see if perhaps there's something else wrong
	//I.E. no modem installed or the hardware lines are broken.
  if ((Modem_ReadReset(Uart) == Bit_RESET) || (Modem_ReadOnOff(Uart) == Bit_RESET))
  {
    printf("Reset or ON/OFF Unresponsive.\r\n");
		return(0);
  }

  if (Modem_ReadPwrMon(Uart)) // Modem PwrMon
  {
    printf("Modem Already On.\r\n");
		
		//Release the UART lines
		USART6_Suppress(Uart, 1);
		return(1);
  }
  else
  {
    // Pull OnOff Low
    Modem_WriteOnOff(Uart, 0); // Modem OnOff Low

		//Loop for passed pulse width
    for(i=0; i<10; i++)
    {
      Sleep(PulseWidth / 10);
    }

    Modem_WriteOnOff(Uart, 1); // Modem OnOff High

		//Loop for a few seconds to ensure the modem is in fact ON
    for(i=0; i<12; i++) // 3 Seconds is more than enough. Follows our power up flow chart
    {
      Sleep(250);
    }
  }

	//Test PWRMON
  if (!Modem_ReadPwrMon(Uart))
  {
    printf("Didn't power on right\n"); // It's dead Jim!
    return(0);
  }

	//Release the UART lines
	USART6_Suppress(Uart, 1);

	//Short pause to let lines stablize
	Sleep(100);

	printf("Modem is On.\r\n"); // Modem state ready

	//Reset structure variables for program use
	Uart->ModemOnTick = SystemTick;
	Uart->ModemOn = 1;
  Uart->NetworkCellRegState = 0;
	Uart->NetworkDataRegState = 0;
  Uart->NetworkActivationState = 0;
	strcpy(Uart->NetworkName, "");
	strcpy(Uart->NetworkIP, "000.000.000.000");
	Uart->RSSI = 99;
	Uart->BER = 99;
	
	return(1); //Success
}

//****************************************************************************

int Modem_TurnOff(UART *Uart, int PulseWidth)
{
  int i;	//Initialize temp variable

	// Disable USART/IF - Remove suprious loads and voltage that might prevent modem shut down
  USART6_Suppress(Uart, 0);

	//Short pause before beginning
  Sleep(100);

	//Quick check if either reset or ON/OFF are currently being held LOW
	//If they are, exit early
  if ((Modem_ReadReset(Uart) == Bit_RESET) || (Modem_ReadOnOff(Uart) == Bit_RESET))
  {
    printf("Reset or ON/OFF Already Asserted.\r\n");
		return(0);
  }

  if (!Modem_ReadPwrMon(Uart))
  {
    fprintf(STDERR, "Modem already Off\r\n");
		return(1);
  }
  else
  {
    // Pull OnOff Low
    Modem_WriteOnOff(Uart, 0); // Modem OnOff Low

		//Loop for passed pulse width
    for(i=0; i<10; i++)
    {
      Sleep(PulseWidth / 10);
    }

    Modem_WriteOnOff(Uart, 1); // Modem OnOff High

		//Loop for a few seconds to ensure the modem is in fact OFF
    for(i=0; i<40; i++) // 10 Seconds is more than enough.
    {
			if (!Modem_ReadPwrMon(Uart))	//Break out early if PWRMON drops
				break;

      Sleep(250);
    }
  }

	//Test PWRMON
  if (Modem_ReadPwrMon(Uart))
  {
    printf("Didn't power off right\n");
		
		//Release the UART lines
		USART6_Suppress(Uart, 1);
    return(0);
  }
	
	//Reset structure variables for program use
	Uart->ModemOffTick = SystemTick;
  Uart->ModemOn = 0;
  Uart->NetworkCellRegState = 0;
	Uart->NetworkDataRegState = 0;
  Uart->NetworkActivationState = 0;
	//strcpy(Uart->NetworkName, "");
	memset(Uart->NetworkName, 0, sizeof(Uart->NetworkName));
	strcpy(Uart->NetworkIP, "000.000.000.000");
	Uart->RSSI = 99;
	Uart->BER = 99;
	
	printf("Modem is Off.\r\n"); // Modem state ready
	
	return(1); //Success
}

//****************************************************************************

//SIM Check routine
int Modem_WaitForSIMReady(UART *Uart)
{
		int Fault = FAULT_OK;		//Initialize Fault State
		char buffer[40]; 				//Temp buffer
		int Start = SystemTick;	//System timer initialization
	
		Uart->ModemSIMReady = 0;	//Initialize structure variable
	
    while((SystemTick - Start) < MODEM_POWER_DELAY)
    {
			Fault = SendATStr(Uart, "AT+CPIN?\r\n", buffer, ATC_STD_TIMEOUT);
			if (!Fault)
			{
				// +CPIN: READY
				if (strncmp(buffer,"+CPIN: READY",12) == 0) 
				{
					Uart->ModemSIMReady = 1;	//Update structure variable
					return Fault;
				}
				
				//TODO - Add extra faults to handle things like needing a PIN
			}
      Sleep(500);
    }

//SIM not ready, return a timeout
return FAULT_TIMEOUT;
}

//****************************************************************************

int Modem_GetPhoneNum(UART *Uart, char *outPN)
{
  int Fault = FAULT_OK;	//Initialize Fault
  char string[256];			//Figure 193 plus some margin
	char PhoneNumber[16];	//Initialize phone number buffer
	char *r;							//Create pointer to hold search location

	//Send the Query
	Fault = SendATStr(Uart, "AT+CNUM\r\n", string, ATC_STD_TIMEOUT);

	//**********************
	// 129 National Numbering, 145 International "+1"
	//Example Responses:
	//AT+CNUM (DE910)
	//+CNUM: "",2223334444,129
	//OK

	//AT+CNUM (HE910)
	//+CNUM: "","12223334444",129
	//OK
	//**********************
	
	//If no fault found
  if (!Fault)
  {
		//printf("Received String: [%s]\n", string);

		//Check the "1xxxxxxxxxx" type of response
		//+CNUM: "","12223334444",129
		
		// Search for end of valid number: ",129
		r = strstr(string, "\",129"); 
	
		//Found
		if (r)
		{
			*r = 0;	//Reset
			
			//Check for the beginning of valid number: ","1
			r = strstr(string, "\",\"1"); 

			//Found
			if (r)
			{
				int i = 0;	//Initialize main increment
				int j = 4;	//Initialize position

				//Fill in phone number, excluding initial '1': 2223334444
				while((i < 16) && (r[j] != 0))
					PhoneNumber[i++] = r[j++];

				PhoneNumber[i] = 0;

				//fprintf(STDERR, "Phone [%s] from CNUM-129\n", PhoneNumber);
				
				//Copy to the outbound buffer
				strcpy(outPN, PhoneNumber);

				//Success
				return(Fault);
			}
		}

		//Check the "xxxxxxxxxx" type of response
		//+CNUM: "",2223334444,129
		
		// Search for end of valid number: ",129
		r = strstr(string, ",129");

		//Found
		if (r)
		{
			*r = 0;	//Mark the end of valid number

			//Check for the beginning of valid number: ",
			r = strstr(string, "\",");

			//Found
			if (r)
			{
				int i = 0;	//Initialize main increment
				int j = 2;	//Initialize position

				//Fill in phone number, 2223334444
				while((i < 16) && (r[j] != 0))
					PhoneNumber[i++] = r[j++];

				PhoneNumber[i] = 0;

				//fprintf(STDERR, "Phone [%s] from CNUM-129\n", PhoneNumber);

				//Copy to the outbound buffer
				strcpy(outPN, PhoneNumber);

				//Success
				return(Fault);
			}
		}

		//Check the "+1xxxxxxxxxx" type of response
		//+CNUM: "","+12223334444",129
		
		// Search for end of valid number: ",145
		r = strstr(string, ",145");

		//Found
		if (r)
		{
			*r = 0;	//Reset

			//Check for the beginning of valid number:  ","+1
			r = strstr(string, "\",+1");

			//Found
			if (r)
			{
				int i = 0;	//Initialize main increment
				int j = 4;	//Initialize position

				//Fill in phone number, excluding initial '+1': 2223334444
				while((i < 16) && (r[j] != 0))
					PhoneNumber[i++] = r[j++];

				PhoneNumber[i] = 0;

				//fprintf(STDERR, "Phone [%s] from CNUM-145\n", PhoneNumber);

				//Copy to the outbound buffer
				strcpy(outPN, PhoneNumber);
				
				//Success
				return(Fault);
			}
		}
	}

  return(Fault);	//Fault return
}

//****************************************************************************

//Delay routine based on the chosen power up delay time, utilizes the modem's start time
void Modem_WaitForPowerUp(UART *Uart)
{
  // Wait for modem to start
  if ((SystemTick - Uart->ModemOnTick) < MODEM_POWER_DELAY)
  {
    while((SystemTick - Uart->ModemOnTick) < MODEM_POWER_DELAY)
    {
      Sleep(125);
    }
  }
}
