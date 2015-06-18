//****************************************************************************
// Modem Network Commands
//****************************************************************************
#include "main.h"
#include "ATC.h"

extern volatile int SystemTick;	//From Main
extern int Sleep(int Delay);		//From Main
extern unsigned Debug;					//From Main
//****************************************************************************

int Network_WaitForCellReg(UART *Uart, int Timeout)
{
	int Fault = FAULT_OK;		//Initialize Fault State
	int Start = SystemTick;	//System timer initialization

	//Begin query loop
	while((SystemTick - Start) < Timeout)
	{
		//Send registration check
		Fault = Network_CheckCellReg(Uart);
		
		//If no fault (OK w/ value response)
		if (!Fault)
		{
			// +CREG: x,x
			//If registered home or roaming, return OK
			//printf("Registration Value: [%d]\n", Uart->NetworkCellRegState);
			if (Uart->NetworkCellRegState == 1 || Uart->NetworkCellRegState == 5) 
				return Fault;
		}
		Sleep(500);	//Check every half second
	}

//Registration failed, return a timeout
//Main program can query the structure for the actual value
return FAULT_TIMEOUT;
}

//****************************************************************************

int Network_CheckCellReg(UART *Uart)
{
  int Fault = FAULT_OK;	//Initialize Fault = 0
  char string[40];			//Temporary buffer

	//Send the Query
	Fault = SendATStr(Uart, "AT+CREG?\r\n", string, ATC_STD_TIMEOUT);
	
	//**********************
	// State Responses
	// +CREG: Mode,State
	// +CREG: 0,x
	// 0 - not registered, ME is not currently searching for a new operator
	// 1 - registered, home network
	// 2 - not registered, but ME is currently searching a new operator to register
	// 3 - registration denied
	// 4 - unknown
	// 5 - registered, roaming
	//**********************

	Uart->NetworkCellRegState = 0; //Initialize the stucture variable
	
	//If no fault found
  if (!Fault)
  {
		//printf("Received String: [%s]\n", string);
		
		//Search for recognized response value and update structure accordingly
		if (strstr(string, ",1"))
			Uart->NetworkCellRegState = 1;

		if (strstr(string, ",2"))
			Uart->NetworkCellRegState = 2;

		if (strstr(string, ",3"))
			Uart->NetworkCellRegState = 3;
		
		if (strstr(string, ",4"))
			Uart->NetworkCellRegState = 4;

		if (strstr(string, ",5"))
			Uart->NetworkCellRegState = 5;
		
		//printf("Registration Value: [%d]\n", Uart->NetworkCellRegState);
	}

return Fault;
}

//****************************************************************************

int Network_WaitForDataReg(UART *Uart, int Timeout)
{
	int Fault = FAULT_OK;		//Initialize Fault State
	int Start = SystemTick;	//System timer initialization

	//Begin query loop
	while((SystemTick - Start) < Timeout)
	{
		//Send registration check
		Fault = Network_CheckDataReg(Uart);
		
		//If no fault (OK w/ value response)
		if (!Fault)
		{
			// +CREG: x,x
			//If registered home or roaming, return OK
			//printf("Registration Value: [%d]\n", Uart->NetworkCellRegState);
			if (Uart->NetworkDataRegState == 1 || Uart->NetworkDataRegState == 5) 
				return Fault;
		}
		Sleep(500);	//Check every half second
	}

//Registration failed, return a timeout
//Main program can query the structure for the actual value
return FAULT_TIMEOUT;
}

//****************************************************************************

int Network_CheckDataReg(UART *Uart)
{
  int Fault = FAULT_OK;	//Initialize Fault = 0
  char string[40];			//Temporary buffer

	//Send the Query
	Fault = SendATStr(Uart, "AT+CGREG?\r\n", string, ATC_STD_TIMEOUT);
	
	//**********************
	// State Responses
	// +CGREG: Mode,State
	// +CGREG: 0,x
	// 0 - not registered, ME is not currently searching for a new operator
	// 1 - registered, home network
	// 2 - not registered, but ME is currently searching a new operator to register
	// 3 - registration denied
	// 4 - unknown
	// 5 - registered, roaming
	//**********************

	Uart->NetworkDataRegState = 0; //Initialize the stucture variable
	
	//If no fault found
  if (!Fault)
  {
		//printf("Received String: [%s]\n", string);
		
		//Search for recognized response value and update structure accordingly
		if (strstr(string, ",1"))
			Uart->NetworkDataRegState = 1;

		if (strstr(string, ",2"))
			Uart->NetworkDataRegState = 2;

		if (strstr(string, ",3"))
			Uart->NetworkDataRegState = 3;
		
		if (strstr(string, ",4"))
			Uart->NetworkDataRegState = 4;

		if (strstr(string, ",5"))
			Uart->NetworkDataRegState = 5;
		
		//printf("Data Registration Value: [%d]\n", Uart->NetworkDataRegState);
	}

return Fault;
}

//****************************************************************************

int Network_GetSignalQuality(UART *Uart)
{
  int Fault = FAULT_OK;		//Initialize Fault = 0
  char string[40];				//Temporary buffer
	char *r;								//Pointer for searching
	char *Match = "+CSQ: ";	//String to match for this query
	
  int RSSIState;  // Signal Strength
  int BERState;   // Bit Error Rate

	//Send the Query
	Fault = SendATStr(Uart, "AT+CSQ\r\n", string, ATC_STD_TIMEOUT);
	
	//**********************
	// State Responses
	// +CSQ: <rssi>,<ber>
	// +CSQ: x,0
	// 0 - (-113) dBm or less
	// 1 - (-111) dBm
	// 2..30 - (-109)dBm..(-53)dBm / 2 dBm per step
	// 31 - (-51)dBm or greater
	// 99 - not known or not detectable
	//**********************
	
	Uart->RSSI = 99;	//Initialize the structure variable
	Uart->BER = 0;		//Initialize the structure variable
	
	//If no fault found
  if (!Fault)
  {
		//printf("Received String: [%s]\n", string);
		
		//Search for valid match
		r = strstr(string, Match);

		//Found
		if (r)
		{
			//Grab the length and increment the r pointer to be at the values
			r += strlen(Match);
			//printf("r value: [%s]\n", r);
			
			//Scan the string and convert to decimal values
			sscanf(r, "%d, %d", &RSSIState, &BERState);
			
      //fprintf(STDERR, "RSSI %d -> %d dBm, ",RSSIState,(-113 + (RSSIState * 2)) );
      //fprintf(STDERR, "BER  %d -> ",BERState);
			
			//Update Structure
			Uart->RSSI = RSSIState;
			Uart->BER = BERState;
		}
  } //Fault endif

return Fault;
}

//****************************************************************************

int Network_GetNetworkName(UART *Uart)
{
  int Fault = FAULT_OK;		//Initialize Fault = 0
  char string[40];				//Temporary buffer
	char *r;								//Pointer for searching

	//Send the Query
	Fault = SendATStr(Uart, "AT+COPS?\r\n", string, ATC_STD_TIMEOUT);
	
	//**********************
	// State Responses
	// +COPS: <mode>,<format>,<oper>
	// +COPS: 0,0,"AT&T",2
	//**********************
	
	//Uart->NetworkName = NULL;	//Initialize the structure variable
	
	//If no fault found
  if (!Fault)
  {
		//printf("Received String: [%s]\n", string);
			
		//Check for the beginning of valid name: ,"
		r = strstr(string, ",\""); 
		//printf("rbeg value: [%s]\n", r);
		
		strcpy(Uart->NetworkName, "");	//Reset the Name before filling anew
		
		//Found
		if (r)
		{
			int i = 0;	//Initialize main increment
			int j = 2;	//Initialize position

			//Fill in the name
			while((i < 16) && (r[j] != '"'))
				Uart->NetworkName[i++] = r[j++];

			Uart->NetworkName[i] = 0;	//Mark the end

			//fprintf(STDERR, "Network Name [%s] from COPS?\n", Uart->NetworkName);
		}
	} //Fault endif
	
return Fault;
}

//****************************************************************************

int Network_SetupContext(UART *Uart, char *inAPN)
{
  int Fault = FAULT_OK;	//Initialize Fault = 0
  char string[128];			//Temporary buffer
	
	//Format and store the full string to the temp buffer
	//Currently supports context 1 only
  sprintf(string, "AT+CGDCONT=1,\"IP\",\"%s\",\"0.0.0.0\",0,0\r\n", inAPN);
	//fprintf(STDERR, "Sent Command: %s", string);
	
	//Send the formatted string to the modem to set the context info
  Fault = SendAT(Uart, string, ATC_STD_TIMEOUT);

return Fault;
}

//****************************************************************************

int Network_GetContext(UART *Uart)
{
  int Fault = FAULT_OK;	//Initialize Fault = 0
	char string[40];			//Temporary buffer
	char *r;							//Pointer for searching

  //Check for an active context, only supports context 1 currently.
  //We can use CGPADDR to retreive the current CID's IP if available as well.
	
	//Send the query, replace the string with the response
	Fault = SendATStr(Uart, "AT#CGPADDR=1\r\n", string, ATC_STD_TIMEOUT);
	
	//**********************
	// State Responses
	// #CGPADDR: cid,”xxx.yyy.zzz.www”
	// #CGPADDR: 1,”255.255.255.255”
	//**********************	
	
	Uart->NetworkActivationState = 0;						//Reset the activation state before filling anew
	strcpy(Uart->NetworkIP, "000.000.000.000");	//Reset the IP before filling anew
	
	//If we have a fault the above clearing is just as valid since the context cannot be open
	//If we get an ERROR on the command
	
	//No Fault
	if (!Fault)
	{
		//Search for a decimal, if we don't find one a context is not open (returned "")
		r = strstr(string, "."); 
		//printf("r value: [%s]\n", r);
		
		//Found IP
		if (r)
		{
			//printf("Received String: [%s]\n", string);
			
			// Search for beginning of valid string: just a ": "166.130.102.97"
			r = strstr(string, "\""); 
			//printf("rbeg value: [%s]\n", r);

			//Found BOS
			if (r)
			{
				int i = 0;	//Initialize main increment
				int j = 1;	//Initialize position

				//Fill in the IP
				while((i < 16) && (r[j] != '"'))
					Uart->NetworkIP[i++] = r[j++];

				Uart->NetworkIP[i] = 0;						//Mark the end
				Uart->NetworkActivationState = 1;	//Set the activation state
				
				//fprintf(STDERR, "Network IP [%s] from CGPADDR?\n", Uart->NetworkIP);
			}//BOS endif
		} //IPcheck If endif
	}	//Fault endif
	
return Fault;
}

//****************************************************************************

int Network_ActivateContext(UART *Uart, char *UserName, char *Password)
{
  int Fault = FAULT_OK;	//Initialize Fault = 0
  char sndstring[128];	//Temporary buffer
	char rcvstring[40];		//Temporary buffer

  //Activate context, only supports context 1 currently

	if (Uart->NetworkActivationState == 0)
	{
		//Context not already active
		//Format and store the full string to the temp buffer
		sprintf(sndstring, "AT#SGACT=1,1,\"%s\",\"%s\"\r\n", UserName,Password);
		
		//Send the query
		Fault = SendATStr(Uart, sndstring, rcvstring, NET_QRY_TIMEOUT);
		//fprintf(STDERR, "Sent Command: %s", sndstring);
		//fprintf(STDERR, "Received Response: [%s]\n", rcvstring);
		
		//**********************
		// State Responses
		// #SGACT: xxx.yyy.zzz.www
		// #SGACT: 255.255.255.255
		//**********************	
		
		//We don't actually care what the response is, we're running GetContext to update information
	}

	//Active or not, refresh the structure information
	if (!Fault)
		Fault = Network_GetContext(Uart);

return Fault;
}

//****************************************************************************

int Network_DeactivateContext(UART *Uart)
{
  int Fault = FAULT_OK;	//Initialize Fault = 0

  //Deactivate context, only supports context 1 currently

	//Check the structure state
	if (Uart->NetworkActivationState == 1)
	{
		//Context active, send the command
		Fault = SendAT(Uart, "AT#SGACT=1,0\r\n", NET_QRY_TIMEOUT);
	}
	
	Sleep(2000);	//Sleep for 2s to let the modem catch up
	
	//Active or not, refresh the structure information
	if (!Fault)
		Fault = Network_GetContext(Uart);

return Fault;
}

//****************************************************************************




