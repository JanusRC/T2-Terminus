//****************************************************************************
// Modem AT Handling
//****************************************************************************
#include "main.h"
#include "ATC.h"

extern volatile int SystemTick;	//From Main
extern int Sleep(int Delay);		//From Main
extern unsigned Debug;					//From Main

//****************************************************************************

// Expects response string with leading white space stripped
//  Identifies classic modem responses

int ProbeModemResponse(char *s)
{
  if (strncmp(s, "OK", 2) == 0)
    return(1);

  if (strncmp(s, "RING", 4) == 0)
    return(1);
	
  if (strncmp(s, "SRING", 5) == 0)
    return(1);

  if (strncmp(s, "CONNECT", 7) == 0)
    return(1);

  if (strncmp(s, "ERROR", 5) == 0)
    return(1);

  if (strncmp(s, "+CMS ERROR", 10) == 0)
    return(1);

  if (strncmp(s, "+CME ERROR", 10) == 0)
    return(1);

  if (strncmp(s, "NO ANSWER", 9) == 0)
    return(1);

  if (strncmp(s, "NO CARRIER", 10) == 0)
    return(1);

  if (strstr(s, "ERROR")) // Anywhere as async
    return(1);

  if (strstr(s, "NO CARRIER")) // Anywhere as async
    return(1);

  return(0);
}

//****************************************************************************

/*********************************************************
Basic result codes
Result Codes, Numeric form, Verbose form Description
0 OK Command executed.
1 CONNECT Entering online state.
2 RING Alerting signal received from network.
3 NO CARRIER Unable to activate the service.
4 ERROR Command not recognized or could not be executed.
6 NO DIALTONE No dial tone detected within time-out period.
7 BUSY Reorder (Busy signal) received.
8 NO ANSWER Five seconds of silence not detected after ring back when @ dial modifier is used.
9 > Prompt response that accompanies SMS and similar command structures
*********************************************************/

int ModemToFaultCode(const char *s)
{
  if (strncmp(s,"OK",2) == 0)
    return(FAULT_OK);

  if (strncmp(s,"RING",4) == 0)
    return(FAULT_RING);
	
  if (strncmp(s,"SRING",5) == 0)
    return(FAULT_SRING);

  if (strncmp(s,"CONNECT",7) == 0)
    return(FAULT_CONNECT);

  if (strncmp(s,"ERROR",5) == 0)
    return(FAULT_ERROR);
	
  if (strncmp(s,">",1) == 0)
    return(FAULT_PROMPT_FOUND);

  if (strncmp(s,"+CMS ERROR:", 11) == 0)
		return(FAULT_CMS);

  if (strncmp(s,"+CME ERROR:", 11) == 0)
		return(FAULT_CME);

  if (strncmp(s,"NO CARRIER",10) == 0)
    return(FAULT_NO_CARRIER);

  if (strncmp(s,"NO DIALTONE",12) == 0)
    return(FAULT_NO_DIALTONE);

  if (strncmp(s,"NO ANSWER",9) == 0)
    return(FAULT_NO_ANSWER);

  if (strncmp(s,"BUSY",4) == 0)
    return(FAULT_BUSY);

  if (strncmp(s,"TIMEOUT",7) == 0)
    return(FAULT_TIMEOUT);

  return(FAULT_FAIL);
}

//****************************************************************************
//NEW FUNCTIONS
//****************************************************************************

int SendAT(UART *Uart, const char *s, int Timeout)
{
	char *res;
	
	//fprintf(STDERR, "\r\nSent Command: %s", s);
	
	//Send the command out via the FIFO
  Uart_SendMessage(Uart, s);
	
	//Flush the UART
  Uart_Flush(Uart);
	
	//Read current UART buffer and stuff into our FIFO
  res = ReceiveAT(Uart, Timeout); 

	//fprintf(STDERR, "\r\nReceived Response: %s", res);
	
	//Return the fault code for fast evaluation
	return(ModemToFaultCode(res));
}

//****************************************************************************

int SendATStr(UART *Uart, const char *s, char *outString, int Timeout)
{
	char *res;
	
	//fprintf(STDERR, "\r\nSent Command: %s", s);
	
	//Send the command out via the FIFO
  Uart_SendMessage(Uart, s);
	
	//Flush the UART
  Uart_Flush(Uart);
	
	//Read current UART buffer and stuff into our FIFO
	//Place the returned string into the buffer for main program evaluation
  res = ReceiveATStr(Uart, outString, Timeout); 

	//fprintf(STDERR, "\r\nReceived Response: %s", res);
	//fprintf(STDERR, "\r\nReceived String: %s", outString);
	
	//Return the fault code for fast evaluation
	return(ModemToFaultCode(res));
}

//****************************************************************************

int SendATStrML(UART *Uart, const char *s, char *outString, int Timeout)
{
	char *res;
	
	//fprintf(STDERR, "\r\nSent Command: %s", s);
	
	//Send the command out via the FIFO
  Uart_SendMessage(Uart, s);
	
	//Flush the UART
  Uart_Flush(Uart);
	
	//Read current UART buffer and stuff into our FIFO
	//Place the returned string into the buffer for main program evaluation
  res = ReceiveATStrML(Uart, outString, Timeout); 

	//fprintf(STDERR, "\r\nReceived Response: %s", res);
	//fprintf(STDERR, "\r\nReceived String: %s", outString);
	
	//Return the fault code for fast evaluation
	return(ModemToFaultCode(res));
}

//****************************************************************************

char *StripWhiteSpace(char *s)
{
	// Strip leading CR LF SPACE TAB if there's valid data
  if (s) 
    while((*s == '\r') || (*s == '\n') || (*s == ' ') || (*s == '\t'))
      s++;

  return(s);
}

//****************************************************************************

char *ReceiveAT(UART *Uart, int Timeout)
{
  char *String;						//Local string buffer
  int i = 0;							//Counter
  int Start = SystemTick;	//System counter

  do
  {
		//Parse the AT response, find and strip the ending /r/n
		//Returns NULL or received line
		//Example: "\r\nOK\r\n"
		//Will return 2 times in this loop with: 
		//-->""
		//-->OK
    String = Fifo_ParseGenericLine(Uart->Rx);

		//Strip leading CR LF TAB SPACE if the data is valid
    String = StripWhiteSpace(String); 

		//Buffer is not null
    if (String)
    {
			//Check against standard end of reply responses
			// OK, ERROR, NO CARRIER, etc
      if (ProbeModemResponse(String)) 
        break;
			
			//Check against the entry prompt reply
			// ">"
      if (strstr(String, ">"))
        break;

			//Not one of these expected responses, put the string back to NULL and continue loop
      String = NULL;
    }
		
		//Accidental NULL
    else
    {
			// The modem prompt is not followed by CR/LF pair, peek directly at FIFO
      if (Fifo_PeekChar(Uart->Rx) == '>') 
      {
				//Prompt found, return prompt for processing
        Fifo_GetChar(Uart->Rx);
        return(">");
      }
    }
		
		//Nothing, loop again
    if (!String)
    {
			//Pause for 10ms
      Sleep(10);
    }

    i = SystemTick - Start;
  }
  while((String == NULL) && (i < Timeout));

  if (i >= Timeout)
    return("TIMEOUT");

  return(String);
}

//****************************************************************************

char *ReceiveATStr(UART *Uart, char *outString, int Timeout)
{
  char *String;						//Local string buffer
  int i = 0;							//Timeout Counter
  int Start = SystemTick;	//System counter

  do
  {
		//Parse the standard AT response, find and strip the ending /r/n
		//Returns NULL or received line
		//Example: "\r\n+CREG: 0,1\r\n\r\nOK\r\n"
		//Will return 3 times in this loop with: 
		//-->""
		//-->+CREG: 0,1
		//-->OK
    String = Fifo_ParseGenericLine(Uart->Rx);

		//Strip leading CR LF TAB SPACE if the data is valid
    String = StripWhiteSpace(String);

		//Buffer is not null
    if (String)
    {
			//Create length buffer
      int len = 0;

			//Check against standard end of reply responses
			// OK, ERROR, NO CARRIER, etc
      if (ProbeModemResponse(String))
        break;

			//No EOR found, update buffer with length of parsed line
      len = strlen(String);

			//Valid length/data
      if (len)
      {
				//printf("\r\nFound Length %d", len);
				//printf("\r\nLocal String: %s", String);
				
				//Copy the local buffered line to the out buffer
        memcpy(outString, String, len);

				//Remove any trailing /r/n from copied buffer
        while(len &&
              ((outString[len - 1] == '\r') ||   //CR
               (outString[len - 1] == '\n')) )   //LF
          len--;

      }

			//Put the local buffer back to NULL to continue loop
      String = NULL;
    }
    else
    {
			//The modem prompt is not followed by CR/LF pair, peek directly at FIFO
      if (Fifo_PeekChar(Uart->Rx) == '>')
      {
				//Prompt found, return prompt for processing
        Fifo_GetChar(Uart->Rx);
        return(">");
      }
    }

		//Nothing, loop again
    if (!String)
    {
			//Pause for 10ms
      Sleep(10);
    }

		//Update counter
    i = SystemTick - Start;
  }
  while((String == NULL) && (i < Timeout));

	//Timeout
  if (i >= Timeout)
    return("TIMEOUT");

	//Return EOL, Prompt, or Timeout
  return(String);
}

//****************************************************************************
// Multiline accumulation
// For example SMS messages, terminating with \r\nOK\r\n
char *ReceiveATStrML(UART *Uart, char *outString, int Timeout)
{
  char *String;						//Local string buffer
  int i = 0; 							//Timeout Counter
  int Start = SystemTick;	//System counter

  do
  {
		//Parse the AT response, find and strip the ending /r/n
		//Returns NULL or received line
		//Example: "\r\n+CMGL: 1,"REC UNREAD","+12223334444","","15/02/11,12:37:17-24",145,9\r\nTest12345\r\n\r\nOK\r\n"
		//Will return 4 times in this loop with: 
		//-->""
		//-->+CMGL: 1,"REC UNREAD","+12223334444","","15/02/11,12:37:17-24",145,9
		//-->Test12345
		//-->OK
		
		//Example: "\r\n+CMGL: 1,"REC UNREAD","+12223334444","","15/02/11,12:37:17-24",145,9\r\nTest\r\n12345\r\n\r\nOK\r\n"
		//-->""
		//-->+CMGL: 1,"REC UNREAD","+12223334444","","15/02/11,12:37:17-24",145,9
		//-->Test
		//-->12345
		//-->OK
    String = Fifo_ParseGenericLine(Uart->Rx);

		//Strip leading CR LF TAB SPACE if the data is valid
    String = StripWhiteSpace(String);

		//Buffer is not null
    if (String)
    {
			//Create length buffer
      int len = 0;

			//Check against standard end of reply responses
			// OK, ERROR, NO CARRIER, etc
      if (ProbeModemResponse(String))
        break;

      //No EOR found, let's accumulate
			//Check the length of this line
      len = strlen(String);

			//Valid length
      if (len)
      {
				//printf("\r\nFound Length %d", len);
				//printf("\r\nLocal String: %s", String);
				
				//Add \r\n to separate for later parsing/use
				//This doubles to put back the \r\n in areas that meant to have this in the data (Ex: SMS data)
        String[len] = '\r';
				String[len+1] = '\n';

				//Copy the local buffered line to the out buffer (+2 for the additive)
        memcpy(outString, String, len+2);

				//Advance (+2 for the additive)
        outString += len+2;  
      }

			//Put the local buffer back to NULL to continue loop
      String = NULL;
    }
		
		//Nothing, loop again
    if (!String)
    {
			//Pause for 10ms
      Sleep(10);
    }

		//Update counter
    i = SystemTick - Start;
		
  }
  while((String == NULL) && (i < Timeout));

	//Timeout
  if (i >= Timeout)
    return("TIMEOUT");

	//Return EOL, or Timeout
  return(String);
}


