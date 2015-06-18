//****************************************************************************
// Modem Socket Handling
//****************************************************************************
#include "main.h"
#include "ATC.h"

extern volatile int SystemTick;	//From Main
extern int Sleep(int Delay);		//From Main
extern unsigned Debug;					//From Main
//****************************************************************************

int Socket_Setup(UART *Uart, int SID)
{
	int Fault = FAULT_OK;		//Initialize Fault State
  char sndstring[40];			//Temporary buffer
	
	//------------------------
	
  //Sets the socket information via the SCFG command
	//
  //SCFG=x,1,1500,0,300,1		<- Our Selection
  //     x,1,300,90,600,50	<- Default
	
	//Format and store the full string to the temp buffer
	//Currently supports context 1 only
	sprintf(sndstring,"AT#SCFG=%d,1,1500,0,300,1\r\n", SID);
	//fprintf(STDERR, "Sent Command: %s", sndstring);
	
	//Send the formatted string to the modem
  Fault = SendAT(Uart, sndstring, ATC_STD_TIMEOUT);
	
	//------------------------
	
  //Sets the socket information via the SCFGEXT command
  //AT#SCFGEXT=1,0,0,30,0,0	<- Our Selection
  //           1,2,0,30,1,0	<- Default
	
	//Clear the send buffer
	memset(sndstring, 0, sizeof(sndstring)); 
	
	//Format and store the full string to the temp buffer
	sprintf(sndstring,"AT#SCFGEXT=%d,0,0,30,0,0\r\n", SID);
	//fprintf(STDERR, "Sent Command: %s", sndstring);
	
	//Send the formatted string to the modem
	if (!Fault) Fault = Fault = SendAT(Uart, sndstring, ATC_STD_TIMEOUT);
	
	//------------------------
	
	//Skip the escape sequence during transfer
	if (!Fault) Fault = SendAT(Uart, "AT#SKIPESC=1\r\n", ATC_STD_TIMEOUT);
	
return Fault;
}

//****************************************************************************

int Socket_Dial(UART *Uart, int SID, char *IP, char *Port, int Mode)
{
	int Fault = FAULT_OK;		//Initialize Fault State
  char sndstring[40];			//Temporary buffer
	
	//Clear structure variable
	Uart->SocketOpen = 0;
	
	//Format and store the full string to the temp buffer
	//Currently supports TCP only
	sprintf(sndstring,"AT#SD=%d,0,%s,\"%s\",0,0,%d\r\n", SID, Port, IP, Mode);
	//fprintf(STDERR, "Sent Command: %s", sndstring);
	
	//Send the command to open the socket in the specified manner
	Fault = SendAT(Uart, sndstring, NET_SKT_TIMEOUT);
	
	//If we're opening a data mode socket
	if (!Mode)
	{
		//Check for the CONNECT response
		if (Fault == FAULT_CONNECT)
			Fault = FAULT_OK;		//Connect found, simply reassign fault back to OK for main processing
	}
	
	//If we're opening a command mode socket, we get an OK, so just return it
	
	//Update the structure variable with the passed in mode reflected
	if (!Fault)
		Uart->SocketMode = Mode;

//Return OK, ERROR, or socket error types
return Fault;
}

//****************************************************************************

int Socket_Listen(UART *Uart, int SID, char *Port)
{
	int Fault = FAULT_OK;		//Initialize Fault State
  char sndstring[40];			//Temporary buffer
	char rcvstring[40];			//Temporary buffer
	char Match[16];					//Match buffer
	char *r;								//Pointer for searching
	
	//Clear structure variable
	Uart->SocketOpen = 0;
	
	//Clear the receive buffer
	memset(rcvstring, 0, sizeof(rcvstring)); 
	
	//Format and store the full string to the temp buffer
	sprintf(sndstring,"AT#SL=%d,1,%s\r\n", SID, Port);
	//fprintf(STDERR, "Sent Command: %s", sndstring);
	
	//Send the command to open the socket in the specified manner
	Fault = SendAT(Uart, sndstring, ATC_STD_TIMEOUT);
	
	//Check for a possible "CME: already listening" error response
	if (Fault == FAULT_CME)
	{
		//Send an AT#SL? query
		Fault = SendATStr(Uart, "AT#SL?\r\n", rcvstring, ATC_STD_TIMEOUT);

		//No Fault
		if (!Fault)
		{
			//Format and store a compare string to the temp buffer
			//#SL: SID,Port,x
			sprintf(Match,"AT#SL: %d,%s", SID, Port);
		
			//Serach for the match
			r = strstr(rcvstring, Match);
			printf("r value: [%s]\n", r);
			
			//Found
			if (r)
				Fault = FAULT_OK; //Already open socket found, simply reassign fault back to OK for main processing
			
			//Not found, just an OK response meaning nothing already open
			
		}//Fault endif
	}//CME check endif
	
	//If we're opening a listening socket, we get an OK, so just return it

//Return OK, ERROR, or socket error types
return Fault;
}

//****************************************************************************

int Socket_SetupFirewall(UART *Uart, char *IP, char *Mask)
{
	int Fault = FAULT_OK;		//Initialize Fault State
  char sndstring[40];			//Temporary buffer
	
	/*****************
	Socket Listening Whitelist
	Connection is accepted if the criteria is met:
		incoming_IP & <net_mask> = <ip_addr> & <net_mask>
		
	Example:
		Possible IP Range = 197.158.1.1 to 197.158.255.255
		We set:
		IP = "197.158.1.1",
		Mask = "255.255.0.0"
	******************/	
	
	//Clear the previous whitelist if there is one
	Fault = SendAT(Uart, "AT#FRWL=2\r\n", ATC_STD_TIMEOUT);
	
	//Format and store the full string to the temp buffer
	sprintf(sndstring,"AT#FRWL=1,\"%s\",\"%s\"\r\n", IP, Mask);
	//fprintf(STDERR, "Sent Command: %s", sndstring);
	
	//Send the formatted string to the modem
  if (!Fault) Fault = SendAT(Uart, sndstring, ATC_STD_TIMEOUT);
	
return Fault;
}

//****************************************************************************

int Socket_EnterCMDMode(UART *Uart)
{
	int Fault = FAULT_OK;		//Initialize Fault State
	BYTE Buffer[1];					//Send indication character buffer
	
	//Bare send of the message to the uart
	Buffer[0] = '+';									//Create escape character
	
	Uart_SendBuffer(Uart, 1, Buffer);	//Send first escape
	Sleep(50);
	Uart_SendBuffer(Uart, 1, Buffer);	//Send second escape
	Sleep(50);
	Uart_SendBuffer(Uart, 1, Buffer);	//Send third escape
	Sleep(50);
	
	Uart_Flush(Uart);									//Flush the UART Tx	
	
	Sleep(250);												//Slight delay
	
	//Read the UART for the Fault condition, receive an OK
	Fault = ModemToFaultCode(ReceiveAT(Uart, NET_SKT_TIMEOUT));
	
	//Update the structure variable
	if (!Fault)
		Uart->SocketMode = 1;
	
return Fault;
}

//****************************************************************************

int Socket_EnterDataMode(UART *Uart, int SID)
{
	int Fault = FAULT_OK;		//Initialize Fault State
  char sndstring[16];			//Temporary buffer
	
	//Format and store the full string to the temp buffer
	sprintf(sndstring,"AT#SO=%d\r\n", SID);
	fprintf(STDERR, "Sent Command: %s", sndstring);
	
	//Send the command to open the socket in the specified manner
	Fault = SendAT(Uart, sndstring, ATC_STD_TIMEOUT);
	
	//Update the structure variable
	if (!Fault)
		Uart->SocketMode = 0;
	
return Fault;
}

//****************************************************************************

int Socket_Close(UART *Uart, int SID)
{
	int Fault = FAULT_OK;		//Initialize Fault State
  char sndstring[16];			//Temporary buffer
	
	//Format and store the full string to the temp buffer
	sprintf(sndstring,"AT#SH=%d\r\n", SID);
	//fprintf(STDERR, "Sent Command: %s", sndstring);
	
	//Send the command to open the socket in the specified manner
	Fault = SendAT(Uart, sndstring, NET_SKT_TIMEOUT);
	
return Fault;
}

//****************************************************************************

int Socket_Accept(UART *Uart, int CID)
{
	int Fault = FAULT_OK;		//Initialize Fault State
  char sndstring[16];			//Temporary buffer
	
	//Format and store the full string to the temp buffer
	sprintf(sndstring,"AT#SA=%d\r\n", CID);
	//fprintf(STDERR, "Sent Command: %s", sndstring);
	
	//Send the command to open the socket in the specified manner
	Fault = SendAT(Uart, sndstring, NET_SKT_TIMEOUT);
	
return Fault;
}



