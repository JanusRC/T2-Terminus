//****************************************************************************
// Modem SMS Handling
//****************************************************************************
#include "main.h"
#include "ATC.h"

extern volatile int SystemTick;	//From Main
extern int Sleep(int Delay);		//From Main
extern unsigned Debug;					//From Main

//****************************************************************************

int SMS_SetupSMS(UART *Uart)
{
	int Fault = FAULT_OK;		//Initialize Fault State
	
	//Enable TEXT format for SMS Message
	Fault = SendAT(Uart, "AT+CMGF=1\r\n", ATC_STD_TIMEOUT);
	
	//No indications, we will poll manually
	if (!Fault) Fault = SendAT(Uart, "AT+CNMI=0,0,0,0,0\r\n", ATC_STD_TIMEOUT);
	
	//Storage location	
	if (!Fault) Fault = SendAT(Uart, "AT+CPMS=\"SM\"\r\n", ATC_STD_TIMEOUT);	
	
	//Disable SMS extra information display
	if (!Fault) Fault = SendAT(Uart, "AT+CSDH=0\r\n", ATC_STD_TIMEOUT);
	
return Fault;
}

//****************************************************************************

int SMS_SendSMS(UART *Uart, SMSSTRUCT *SMS, char *Text, char *Phone)
{
	int Fault = FAULT_OK;			//Initialize Fault State
  char sndstring[40];				//Temporary buffer
	char rcvstring[40];				//Temporary buffer
	char *r;									//Pointer for searching
	BYTE Buffer[1];						//Send indication character buffer
  int SMSIndex = 0;					//Parsed SMS Index
	
	//Clear structure variable
	SMS->OutIndex = 0; 
	
	//Clear the receive buffer, the Telit returns "nothing"/OK
	memset(rcvstring, 0, sizeof(rcvstring)); 
	
	//Format and store the full string to the temp buffer
	sprintf(sndstring,"AT+CMGS=\"%s\"\r\n", Phone);
	
	//Send the formatted string to the modem to start the SMS send
  Fault = SendAT(Uart, sndstring, ATC_STD_TIMEOUT);

	//fprintf(STDERR, "Sent Command: %s", sndstring);
	//fprintf(STDERR, "SMS Send Fault: %08X\n",Fault);
	
	//Easy check for prompt built into the ATC
	if (Fault == FAULT_PROMPT_FOUND)
	{
		//fprintf(STDERR, "Sending SMS: [%s]\n", Text);
		//Bare send of the message to the uart
		Uart_SendMessage(Uart, Text);			//Send the command out via the FIFO
		Uart_Flush(Uart);									//Flush the UART Tx

		Buffer[0] = 0x1A;									//Create send indication
		
		Uart_SendBuffer(Uart, 1, Buffer);	//Finish the send with a CTRL+Z (0x1a)
		Uart_Flush(Uart);									//Flush the UART Tx	
		
		//Read the UART for the Fault condition, receive the SMS index
		Fault = ModemToFaultCode(ReceiveATStr(Uart, rcvstring, NET_SMS_TIMEOUT));
		
		//fprintf(STDERR, "\r\nReceived Fault: %08X", Fault);
		//fprintf(STDERR, "\r\nReceived String: %s\r\n", rcvstring);
		
		//If no Fault occurs during a send
		if (!Fault) 
		{
			//Search for the returned string for the index response
			//"+CMGS: x
			r = strstr(rcvstring, "+CMGS: ");
			
			//Found
			if (r)
			{
				//Increment the r pointer to be at the value based on known length
				r += 7;
				//printf("r value: [%s]\n", r);
				
				//Scan the string and convert to decimal value
				sscanf(r, "%d", &SMSIndex);
			}
			
			//Update the structure with send index
			SMS->OutIndex = SMSIndex;
			
		}//Send Fault endif
	}//Prompt Fault endif

//Return standard fault
return Fault;
}

//****************************************************************************

int SMS_CheckSMS(UART *Uart, SMSSTRUCT *SMS)
{
	int Fault = FAULT_OK;		//Initialize Fault State
	char SMSList[300]; 			//Large space for storage of possibly multiple lines of text
	char SearchString[16]; 	//Small Search buffer to hold the CMGL: string
	char *r;								//Pointer for searching
	int i = 1;							//Counter
	
	//Clear the list buffer, the Telit returns "nothing"/OK
	memset(SMSList, 0, sizeof(SMSList)); 
	
	//Clear structure variable
	SMS->NumOfStored = 0; 
	
	//Send the query to check for new messages
	//Make use of the Multi-Line AT sending to retreive ALL messages.
	//If there are no stored messages it'll just be an OK with blank response
	Fault = SendATStrML(Uart, "AT+CMGL=\"ALL\"\r\n", SMSList, NET_SMS_TIMEOUT);
	
//	printf("Received Response: \r\n");
//	printf("-----------------\r\n");
//	printf("%s", SMSList);
//	printf("-----------------\r\n");
	
	//No Fault
	if (!Fault)
	{
		//Do an initial probe of the list, starting with 1
		r = strstr(SMSList, "+CMGL: 1");

		while (r)
		{
			//Increment counter, will start at 1 for the loop
			i++;
			
			//Format the search string and store it
			sprintf(SearchString,"+CMGL: %d", i);
			
			//Keep searching the incremental message list
			r = strstr(r, SearchString);
		}
		
		//Subtract 1 for the NULL find if loop was utilized
		if (i > 0)
			i--;
		
		//Update container with amount
		SMS->NumOfStored = i;
	}

//Return standard fault
return Fault;
}

//****************************************************************************

int SMS_ProcessSMS(UART *Uart, SMSSTRUCT *SMS, int selection)
{
	int Fault = FAULT_OK;			//Initialize Fault State
	char SMSRead[300]; 				//Large space for storage of possibly multiple lines of text
	char sndstring[16]; 			//Small buffer to hold the CMGR: string
	int i = 0;								//Initialize value position increment
	int j = 0;								//Initialize r offset
	//char *Match = "+CMGR: \"";//String to match for this query
	char *r;									//Pointer for searching
	
	//Clear the read buffer, the Telit returns "nothing"/OK
	memset(SMSRead, 0, sizeof(SMSRead)); 
	
	//Format and store the full string to the temp buffer
	sprintf(sndstring,"AT+CMGR=%d\r\n", selection);
	
	//Send the query to read the specified message
	//Make use of the Multi-Line AT sending to retreive the SMS as \r\n delimited
	//If there are no stored messages it'll just be an OK with blank response
	Fault = SendATStrML(Uart, sndstring, SMSRead, NET_SMS_TIMEOUT);
	
//	printf("Read Response: \r\n");
//	printf("-----------------\r\n");
//	printf("%s", SMSRead);
//	printf("-----------------\r\n");
	
	//****************************************************************************
	//-Technical Notation-
	//+CMGR: <index>,<stat>,<oa/da>,<alpha>,<scts><CR><LF><data><CR><LF>
	//
	//-Plaintext Example-
	//+CMGR: "REC READ","+12223334444","","15/02/11,12:37:17-24"
	//Test12345
	//OK
	//****************************************************************************

	//If no Fault occurs during a send
	if (!Fault) 
	{
		//Search for the returned string for the index response
		//Looking for CMGR: "
		r = strstr(SMSRead, "+CMGR: \"");
		
		//Found
		if (r)
		{
			//Update the structure with the selected message index
			SMS->StoreIndex = selection;
			
			//Clear the structure variables
			memset(SMS->Stat, 0, sizeof(SMS->Stat)); 
			memset(SMS->OriginatingPN, 0, sizeof(SMS->OriginatingPN)); 
			memset(SMS->Alpha, 0, sizeof(SMS->Alpha)); 
			memset(SMS->Date, 0, sizeof(SMS->Date)); 
			memset(SMS->Time, 0, sizeof(SMS->Time)); 
			memset(SMS->Data, 0, sizeof(SMS->Data)); 
			SMS->Length = 0;
			
			//Increment the r pointer to be at the start of the string based on known length
			//+CMGR: "
			r += 8;
			//printf("r value: [%s]\n", r);

			//*********************
			//	STAT Value
			//*********************
			i = 0;	//Initialize value position increment
			j = 0;	//Initialize r offset
			
			//Scan for the trailing "
			while(r[j] != '"')
				SMS->Stat[i++] = r[j++];
			
			//Increment r to the next value: j value + ","
			r += (j+3);
			
			//printf("r value: [%s]\n", r);
			
			//*********************
			//	OriginatingPN Value
			//*********************	
			i = 0;	//Initialize value position increment
			j = 0;	//Initialize r offset
			
			//Scan for the trailing "
			while(r[j] != '"')
				SMS->OriginatingPN[i++] = r[j++];
			
			//Increment r to the next value: j value + ","
			r += (j+3);
			
			//printf("r value: [%s]\n", r);
			
			//*********************
			//	Alpha Value
			//*********************	
			i = 0;	//Initialize value position increment
			j = 0;	//Initialize r offset
			
			//Scan for the trailing "
			while(r[j] != '"')
				SMS->Alpha[i++] = r[j++];
			
			//Increment r to the next value: j value + ","
			r += (j+3);
			
			//printf("r value: [%s]\n", r);
			
			//*********************
			//	Date Value
			//*********************	
			i = 0;	//Initialize value position increment
			j = 0;	//Initialize r offset
			
			//Scan for the trailing ,
			while(r[j] != ',')
				SMS->Date[i++] = r[j++];
			
			//Increment r to the next value: j value + ,
			r += (j+1);
			
			//printf("r value: [%s]\n", r);
			
			//*********************
			//	Time Value
			//*********************	
			i = 0;	//Initialize value position increment
			j = 0;	//Initialize r offset
			
			//Scan for the trailing "
			while(r[j] != '"')
				SMS->Time[i++] = r[j++];
			
			//Increment r to the next value: j value + "\r\n
			r += (j+3);
			
			//printf("r value: [%s]\n", r);
			
			//*********************
			//	Data Value
			//*********************	
			i = 0;	//Initialize value position increment
			j = 0;	//Initialize r offset
			
			//Scan for the trailing \r\n EOL
			while(r[j] != '\r')
				SMS->Data[i++] = r[j++];
			
			//*********************
			//	Length Value
			//*********************	
			SMS->Length = strlen(SMS->Data);
			
		}//r search endif
	}//send fault endif

return Fault;
}

//****************************************************************************

int SMS_DeleteOneSMS(UART *Uart, int Selection)
{
	int Fault = FAULT_OK;	//Initialize Fault State
	char sndstring[20];		//Temporary buffer
	
	//Format and store the full string to the temp buffer
	//Delete specific message, type "REC READ" and "REC UNREAD"
	sprintf(sndstring,"AT+CMGD=%d,0\r\n",Selection); 
	
	//Send the formatted string to the modem to start the SMS send
  Fault = SendAT(Uart, sndstring, ATC_STD_TIMEOUT);

return Fault;
}

//****************************************************************************

int SMS_DeleteAllSMS(UART *Uart)
{
	int Fault = FAULT_OK;	//Initialize Fault State
	
	//Delete all stored messages
  Fault = SendAT(Uart, "AT+CMGD=1,4\r\n", ATC_STD_TIMEOUT);

return Fault;
}








