//****************************************************************************
// T2 Terminal Handling
//****************************************************************************
#include "main.h"
#include "Modem.h"
#include "ATC.h"

extern volatile int SystemTick;						//From Main
extern int Sleep(int Delay);							//From Main
extern unsigned Debug;										//From Main
extern void SimpleForwardingLoop(void);		//From Main

//Modem Characteristics
extern char ModemMake[40];			//CGMI	-	Manufacturer Information
extern char ModemModel[40];			//CGMM	-	Modem Type
extern char ModemFirmware[40];	//CGMR	-	Modem Firmware
extern char ModemSerial[40]; 		//IMEI	- International Mobile Equipment Identity, modem serial number

//SIM Information
extern char SIMSerial[40]; 			//IMSI	- International Mobile Subscriber Identity
extern char SIMIdentity[40]; 		//ICCID	- Integrated Circuit Card Identification
extern char SIMPNumber[40]; 		//CNUM	- SIM Given phone number

void TerminalHelp(void)
{
  printf("Terminal Mode Help - USART3 Debug Port (Terminus 2) %s %s\r\n\r\n",__TIME__,__DATE__);

	printf("--UTILITIES--\r\n");
  printf("?, H           This help page\r\n");
  printf("Q, QUIT        Exit Terminal Mode\r\n");
  printf("NUKE           Erase Application\r\n");
  printf("REBOOT         Reboot\r\n");
  printf("DFUSE          Reboot w/ DfuSe\r\n");
  printf("FORWARD        Forward SERIAL: DB9<->MODEM\r\n");
	printf("--BASIC MODEM--\r\n");
  printf("MODEM x        None=Status, 0=Off, 1=On\r\n");
  printf("INIT           Initialize Modem With Basic AT commands\r\n");
  printf("IDENTIFY       Identification Information of the Modem\r\n");
	printf("--NETWORK--\r\n");
  printf("REGISTER       Register on Network\r\n");
  printf("ACTIVATE x     Activate Context w/ x APN\r\n");
  printf("DEACTIVATE     Deactivate Context\r\n");
	printf("--SMS--\r\n");
  printf("SENDSMS 1 2    Send SMS Text Message: 1=Phone Number, 2=Text String\r\n");
  printf("CHECKSMS       Check SMS Messages\r\n");
  printf("GETSMS x       Process SMS x\r\n");
  printf("DELSMS x       Delete SMS x\r\n");
	printf("--SOCKET--\r\n");
  printf("SOCKETD 1:2    Open a socket to a listening server: 1=IP Address, 2=Port\r\n");
  printf("SOCKETL 1      Open a socket listen: 1=Port\r\n");

}

//******************************************************************************

void TerminalMode(int TimeOut, UART *UartDB9, UART *UartModem, SMSSTRUCT *SMSPointer)
{
  char *s;
	int i;
  int Start;
	int Fault = FAULT_OK;	//Initialize Fault State = 0

  printf("Entering Terminal Mode, [%d] Seconds to type first command\r\nType ? for help, Q to exit\r\n", TimeOut);

  Start = SystemTick;

  printf("READY\r\n");

  i = 0;

  while(i || ((SystemTick - Start) < (TimeOut * 1000)))
  {
    if (i)
      s = Uart_GetLine(UartDB9);
    else
    s = Fifo_ParseGenericLine(UartDB9->Rx);

    s = StripWhiteSpace(s); // Strip wayward leading CR LF TAB SPACE

    if(s && (*s != 0))
    {
//      printf("CMD:%s\r\n",s);

      if ((stricmp(s, "q") == 0) ||
          (stricmp(s, "quit") == 0) ||
          (stricmp(s, "exit") == 0) )
      {
        break;
      }
      else if ((stricmp(s, "?") == 0) ||
          (stricmp(s, "h") == 0) ||
          (stricmp(s, "help") == 0) )
      {
        TerminalHelp();
      }
      //==================================================================== reboot
      else if (strnicmp(s, "reboot", 6) == 0)
      {
        printf("Rebooting..\r\n");

        while(Fifo_Used(UartDB9->Tx)) // Wait for debug FIFO to flush
          Sleep(100);

        Sleep(100);

        SystemReset();
      }
      //==================================================================== nuke
      else if (stricmp(s, "nuke") == 0)
      {
				printf("Nuking..\r\n");

				while(Fifo_Used(UartDB9->Tx)) // Wait for debug FIFO to flush
					Sleep(100);

				Sleep(100);

				*((unsigned long *)0x2001FFFC) = 0xCAFE0001; // Erase Application

				SystemReset();
      }
      //==================================================================== dfuse
      else if (stricmp(s, "dfuse") == 0)
      {
				printf("DfuSe..\r\n");

				while(Fifo_Used(UartDB9->Tx)) // Wait for debug FIFO to flush
				  Sleep(100);

				Sleep(100);

				*((unsigned long *)0x2001FFFC) = 0xBEEFBEEF; // Boot to ROM

				SystemReset();
      }
      //==================================================================== modem
      else if (strnicmp(s, "modem", 5) == 0)
      {
        unsigned i;

        if (sscanf(s+5," %x",&i) == 1)
        {
          if (i == 1)
          {
            printf("Bringing up modem..\n");
            GPIO_SetBits(GPIOC, GPIO_Pin_1); // PC.01 Plug_in terminal power supply is enabled
            Modem_TurnOn(UartModem, TELIT_TIME_ON);
          }
          else if (i == 2)
            Modem_WriteReset(UartModem, 0);	// Modem Reset Low
          else if (i == 3)
            Modem_WriteReset(UartModem, 1);	// Modem Reset High
          else if (i == 5)
            Modem_Reset(UartModem);     		// Modem Reset
          else if (i == 0)
          {
            printf("Shutting down modem..\n");
            Modem_TurnOff(UartModem, TELIT_TIME_OFF);
            GPIO_ResetBits(GPIOC, GPIO_Pin_1); // PC.01 Plug_in terminal power supply is disabled
         }
        }
        else
          printf("Modem State %s\n", (UartModem->ModemOn ? "On" : "Off") );
      }
      //==================================================================== init
      else if (strnicmp(s, "init", 4) == 0)
      {
        if (UartModem->ModemOn)
        {
					printf("Running initialization routine.\r\n");
					Fault = Terminus_Init(UartModem); //Init routine
					
					if (Fault) 
						printf("Error: Modem Initialization: %08X\n",Fault);
				}
				else
					printf("Modem Not on.\r\n");
      }
      //==================================================================== identify
      else if (strnicmp(s, "identify", 9) == 0)
      {
        if (UartModem->ModemOn)
        {
					printf("Gathering Modem Information.\r\n");
					//Initialize the modem information
					memset(ModemMake, 0, sizeof(ModemMake));
					memset(ModemModel, 0, sizeof(ModemModel));
					memset(ModemFirmware, 0, sizeof(ModemFirmware));
					memset(ModemSerial, 0, sizeof(ModemSerial));
					
					Fault = SendATStr(UartModem, "AT+CGMI\r\n", ModemMake, ATC_STD_TIMEOUT);
					if (!Fault) Fault = SendATStr(UartModem, "AT+CGMM\r\n", ModemModel, ATC_STD_TIMEOUT);
					if (!Fault) Fault = SendATStr(UartModem, "AT+CGMR\r\n", ModemFirmware, ATC_STD_TIMEOUT);
					if (!Fault) Fault = SendATStr(UartModem, "AT+CGSN\r\n", ModemSerial, ATC_STD_TIMEOUT);
					
					//Stop and respond with fault information if necessary
					if (Fault)
						printf("Error: Gathering Modem Information: %08X\n",Fault);
					else
					{
						printf("Modem Information\r\n");
						printf("------------------------------\r\n");
						fprintf(STDERR, "Make:      %s\r\n", ModemMake);
						fprintf(STDERR, "Model:     %s\r\n", ModemModel);
						fprintf(STDERR, "Serial:    %s\r\n", ModemSerial);
						fprintf(STDERR, "Firmware:  %s\r\n", ModemFirmware);
						printf("------------------------------\r\n\r\n");
					}
				}
				else
					printf("Modem Not on.\r\n");
      }
      //==================================================================== register
      else if (stricmp(s, "register") == 0)
      {
        if (UartModem->ModemOn)
        {
					printf("Running Registration Routine.\r\n");
					Fault = Network_WaitForCellReg(UartModem, NET_REG_TIMEOUT);
					if (!Fault) Fault = Network_WaitForDataReg(UartModem, NET_REG_TIMEOUT);
					if (!Fault) Fault = Network_GetSignalQuality(UartModem);

					if (!Fault && (strncmp(ModemModel,"HE910",5) == 0)) 
						Fault = Network_GetNetworkName(UartModem);

					if (Fault) 
						printf("Error: Unable to register: %08X\n",Fault);
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
				else
					printf("Modem Not on.\r\n");
      }
      //==================================================================== activate
      else if (strnicmp(s, "activate", 8) == 0)
      {
				char inAPN[20];	//20 chars, ex: 'epc.tmobile.com'
				
				memset(inAPN, 0, sizeof(inAPN)); 	
				
        if (UartModem->NetworkCellRegState)
        {
					//Scan standard string for the APN to use
					//Increment again + length of APN
					if (sscanf(s+(9+strlen(inAPN)),"%s",inAPN) == 1)
					{							
						printf("Activating Context.\r\n");
						
						//Run the setup routine
						Fault = Network_SetupContext(UartModem, inAPN);

						if (!Fault) Fault = Network_GetContext(UartModem);
						
						if ((!UartModem->NetworkActivationState) && (!Fault))
							Fault = Network_ActivateContext(UartModem, "", "");
						
						if (!Fault)
						{
							printf("Context set up.\r\n");
							printf("State: [%d] || IP: [%s]\r\n", UartModem->NetworkActivationState, UartModem->NetworkIP);
						}
						else
							printf("Error: Creating Context: %08X\n",Fault);
					}
					else
						printf("No APN Entered.\r\n");
        }
				else
					printf("Modem Not Registered.\r\n");
				
				//Reset the S memory location, without this the APN may be retained
				//9 = "activate "
				memset(s, 0, (9+sizeof(inAPN))); 
      }
      //==================================================================== deactivate
      else if (stricmp(s, "deactivate") == 0)
      {
        if (UartModem->NetworkCellRegState)
        {
					printf("Deactivating Context.\r\n");
					Fault = Network_GetContext(UartModem);
					
					if ((UartModem->NetworkActivationState) && (!Fault))
						Fault = Network_DeactivateContext(UartModem);
					
					if (!Fault)
					{
						printf("Context down.\r\n");
						printf("State: [%d] || IP: [%s]\r\n", UartModem->NetworkActivationState, UartModem->NetworkIP);
					}
					else
						printf("Error: Tearing Down Context: %08X\n",Fault);
        }
				else
					printf("Modem Not Registered.\r\n");
      }
      //==================================================================== sendsms
      else if (strnicmp(s, "sendsms", 7) == 0)
      {
				char inPN[16];				//x-xxx-xxx-xxxx
				char inText[200];			//200 chars
				
				memset(inText, 0, sizeof(inText)); 	
				memset(inPN, 0, sizeof(inPN));				
				
        if (UartModem->NetworkCellRegState)
        {
					//Increment by standard string for the number
					if (sscanf(s+8,"%s",inPN) == 1)
					{
						//Scan standard string for the text to send
						//Increment again + length of IP + 1 character for the space
						if (sscanf(s+(8+strlen(inPN)+1),"%s",inText) == 1)
						{			
							printf("Sending Specified SMS.\r\n");
							
							//Quickly run the setup routine
							Fault = SMS_SetupSMS(UartModem);	

							//Send the SMS
							if (!Fault) Fault = SMS_SendSMS(UartModem, SMSPointer, inText, inPN);
							
							//Fault response check with the outbound SMS index
							if (!Fault)
								printf("SMS Sent, Reference Index: [%d].\r\n", SMSPointer->OutIndex);
							else
								printf("Error: Sending SMS: %08X\n",Fault);
						}
						else
						{
							printf("No Message Found, Sending Basic SMS.\r\n");
							
							sprintf(inText,"Powered Up:\n %s %s \nIMEI: %s\nIMSI: %s\nICCID: %s\n",
								__DATE__, __TIME__, ModemSerial, SIMSerial, SIMIdentity);
							
							//Quickly run the setup routine
							Fault = SMS_SetupSMS(UartModem);	

							//Send the SMS
							if (!Fault) Fault = SMS_SendSMS(UartModem, SMSPointer, inText, inPN);
							
							//Fault response check with the outbound SMS index
							if (!Fault)
								printf("SMS Sent, Reference Index: [%d].\r\n", SMSPointer->OutIndex);
							else
								printf("Error: Sending SMS: %08X\n",Fault);
						}
					}
					else
						printf("No Phone Number Found.\r\n");
        }
				else
					printf("Modem Not Registered.\r\n");
				
				//Reset the S memory location, without this the phone number/text may be retained
				//8 = "sendsms "
				memset(s, 0, (8+sizeof(inPN)+1+sizeof(inText))); 
      }
      //==================================================================== checksms
      else if (stricmp(s, "checksms") == 0)
      {
        if (UartModem->NetworkCellRegState)
        {
					printf("Checking for Stored SMS.\r\n");
					
					//Quickly run the setup routine
					Fault = SMS_SetupSMS(UartModem);	
					
					//Check for stored SMS
					if (!Fault) Fault = SMS_CheckSMS(UartModem, SMSPointer);
					
					//Fault response check with available messages
					if (!Fault) 
						printf("[%d] Messages Available.\r\n", SMSPointer->NumOfStored);
					else
						printf("Error: Checking SMS: %08X\n",Fault);
        }
				else
					printf("Modem Not Registered.\r\n");
      }
      //==================================================================== getsms
      else if (strnicmp(s, "getsms", 6) == 0)	
      {
        unsigned i;

        if (UartModem->NetworkCellRegState)
        {
					if (sscanf(s+6," %x",&i) == 1)
					{
						if (i >= 1)
						{				
							printf("Processing Message: [%d]\r\n", i);
							if (!Fault) Fault = SMS_ProcessSMS(UartModem, SMSPointer, i);
							
							//Fault response check with parsed information
							if (Fault)
								printf("Error: Processing SMS: %08X\n",Fault);
						}
						else
						{
							printf("Processing Last Message Stored: [%d].\r\n", SMSPointer->NumOfStored);
							if (!Fault) Fault = SMS_ProcessSMS(UartModem, SMSPointer, SMSPointer->NumOfStored);
							
							//Fault response check with parsed information
							if (Fault)
								printf("Error: Processing SMS: %08X\n",Fault);
						}
					}
					else
					{
						printf("Processing Last Message Stored: [%d].\r\n", SMSPointer->NumOfStored);
						if (!Fault) Fault = SMS_ProcessSMS(UartModem, SMSPointer, SMSPointer->NumOfStored);
						
						//Fault response check with parsed information
						if (Fault)
							printf("Error: Processing SMS: %08X\n",Fault);
					}

					if (!Fault)
					{
						printf("\r\nMessage Information\r\n");
						printf("------------------------------\r\n");
						printf("Index value:         [%d]\r\n", SMSPointer->StoreIndex);
						printf("Stat value:          [%s]\n", SMSPointer->Stat);
						printf("OriginatingPN value: [%s]\n", SMSPointer->OriginatingPN);
						printf("Alpha value:         [%s]\n", SMSPointer->Alpha);
						printf("Date value:          [%s]\n", SMSPointer->Date);
						printf("Time value:          [%s]\n", SMSPointer->Time);
						printf("Data value:          [%s]\n", SMSPointer->Data);
						printf("Length value:        [%d]\n", SMSPointer->Length);
						printf("------------------------------\r\n\r\n");
					}
					
				}//Regcheck endif
				else
					printf("Modem Not Registered.\r\n");
				
				//Reset the S memory location, without the number may be retained
				//7 = "getsms "
				memset(s, 0, (7+i)); 
      }
      //==================================================================== delsms
      else if (strnicmp(s, "delsms", 6) == 0)	
      {
        unsigned i;

        if (UartModem->NetworkCellRegState)
        {
					if (sscanf(s+6," %x",&i) == 1)
					{
						if (i >= 1)
						{				
							printf("Deleting Message: [%d]\r\n", i);
							if (!Fault) Fault = SMS_DeleteOneSMS(UartModem,i);
							
							//Fault response check with parsed information
							if (Fault)
								printf("Error: Deleting SMS: %08X\n",Fault);
						}
						else
						{
							printf("Deleting Last Message Stored: [%d].\r\n", SMSPointer->NumOfStored);
							if (!Fault) Fault = SMS_DeleteOneSMS(UartModem, SMSPointer->NumOfStored);
							
							//Fault response check with parsed information
							if (Fault)
								printf("Error: Deleting SMS: %08X\n",Fault);
						}
					}
					else
					{
						printf("Deleting Last Message Stored: [%d].\r\n", SMSPointer->NumOfStored);
						if (!Fault) Fault = SMS_DeleteOneSMS(UartModem, SMSPointer->NumOfStored);
						
						//Fault response check with parsed information
						if (Fault)
							printf("Error: Deleting SMS: %08X\n",Fault);
					}

					if (!Fault)
						printf("\r\nMessage Deleted\r\n");
					
				}//Regcheck endif
				else
					printf("Modem Not Registered.\r\n");
				
				//Reset the S memory location, without the number may be retained
				//7 = "delsms "
				memset(s, 0, (7+i)); 
      }
      //==================================================================== socketd
      else if (strnicmp(s, "socketd", 7) == 0)
      {
				int Tries = 4;				//Try to open the socket 4 times if it fails
				int i = 1;						//Try Counter
				char inIP[40];				//Parsed IP, give it a larger buffer for DNS based IP
				char inPort[8];				//Parsed Port, usual 4 characters + extra
				
				memset(inIP, 0, sizeof(inIP)); 
				memset(inPort, 0, sizeof(inPort)); 
				
				//First check for registration/activation
				if (UartModem->NetworkDataRegState && UartModem->NetworkActivationState)
				{
					//Increment by known command, scan until the ':'
					if (sscanf(s+8,"%[^:]",inIP) == 1)
					{
						//Scan standard string for the port
						//Increment again + length of IP + 1 character for the ':'
						if (sscanf(s+(8+strlen(inIP)+1),"%s",inPort) == 1)
						{
							//We have both argument, set up the socket
							Fault = Socket_Setup(UartModem, 1);
						
							//Begin Socket dial
							if (!Fault) 
							{
								printf("Opening Socket to server [%s]:[%s].\r\n", inIP, inPort);
								printf("Limited to [%d] tries.\r\n", Tries);
								
								//Loop for specified trial attempts, double check if socket is already open
								while ((i <= Tries) && (!UartModem->SocketOpen))
								{
									__WFI();
									
									Fault = Socket_Dial(UartModem, 1, inIP, inPort, 0);
									
									i++;
									
									if (Fault == FAULT_OK)
										break;
									
									if (i <= Tries)
										printf("Failed to open, trying again (Try [%d])\r\n", i);
								}
							}
							
							if (!Fault && UartModem->SocketOpen)
							{
								//Socket Open, forward the UART
								printf("Socket open, forwarding UART.\r\n");
								
								//Loop while the SocketOpen variable shows valid connection
								while (UartModem->SocketOpen)
								{
									//Make sure to wait for interrupts to handle the UART FIFO
									__WFI();

									Uart_ForwardFullDuplex(UartModem, UartDB9); // Modem to/from DB9
								}
								
								//Flush DB9 Port
								while(Fifo_Used(UartDB9->Tx)) // Wait for debug FIFO to flush
									Sleep(100);
								
								if (!UartModem->SocketOpen)
									printf("\r\nSocket Closed.\r\n\r\n");
							}
							else
								printf("Socket Unable to Open.\r\n");
								
						}//portscan endif
						else
							printf("No Port Found.\r\n");
						
					}//Ipscan endif
					else
						printf("No IP Found.\r\n");
					
				}//DataRegState endif
				else
					printf("Modem Not Registered OR not context is not open.\r\n");
				
				//Reset the S memory location, without this the IP/Port may be retained
				//8 = "socketd "
				memset(s, 0, (8+sizeof(inIP)+1+sizeof(inPort))); 
      }
			
      //==================================================================== socketl
      else if (strnicmp(s, "socketl", 7) == 0)
      {
				char inPort[8];				//Parsed Port, usual 4 characters + extra
				char *String;					//Local string buffer
				int ConnID = 0;				//Local connection ID buffer
				
				memset(inPort, 0, sizeof(inPort)); 
				
				//First check for registration/activation
				if (UartModem->NetworkDataRegState && UartModem->NetworkActivationState)
				{
					//Scan standard string for the port
					if (sscanf(s+8,"%s",inPort) == 1)
					{
						Fault = Socket_Setup(UartModem, 1);
						
						if (!Fault) Fault = Socket_SetupFirewall(UartModem, "0.0.0.0", "0.0.0.0");
						
						//Begin socket listen
						if (!Fault) 
						{
							printf("Opening Socket Listen [%s]:[%s].\r\n", UartModem->NetworkIP, inPort);
							Fault = Socket_Listen(UartModem, 1, inPort);
						}
						
						if (!Fault)
						{
							printf("Listener Open, waiting for client connection...\r\n");
							
							while (!Fault && UartModem->NetworkActivationState)
							{
								//Make sure to wait for interrupts to handle the UART FIFO
								__WFI();

								//Parse the AT interface, find and strip an ending /r/n if available
								String = Fifo_ParseGenericLine(UartModem->Rx);

								//Strip leading CR LF TAB SPACE if the data is valid
								String = StripWhiteSpace(String); 

								//Buffer is not null, we found something
								if (String)
								{
									//Fast reference to a fault code
									Fault = ModemToFaultCode(String);
									
									//Check for incoming SRING via the standard fault code handler
									// SRING: x
									if (Fault == FAULT_SRING)
									{
										//Scan the string and convert the ID to decimal value
										String += 7;
										sscanf(String, "%d", &ConnID);
										
										//Update the structure with send index
										UartModem->ConnID = ConnID;		
										
										break;
									}
								}	
								
								//Nothing found, check context status
								if (!Fault) Fault = Network_GetContext(UartModem);
							}
							
							//Loop broken, was it an SRING?
							if (Fault == FAULT_SRING)
								Fault = Socket_Accept(UartModem, UartModem->ConnID);
							
							if (Fault == FAULT_CONNECT) 
							{
								printf("Socket Open on connection [%d]\r\n", UartModem->ConnID);
								printf("Forwarding UART.\r\n");
								Fault = FAULT_OK;	//Reset Fault for main.c processing
							}
							
							//Loop while the SocketOpen variable shows valid connection
							while (UartModem->SocketOpen)
							{
								__WFI();

								Uart_ForwardFullDuplex(UartModem, UartDB9); // Modem to/from DB9
							}
							
							//Flush DB9 Port
							while(Fifo_Used(UartDB9->Tx)) // Wait for debug FIFO to flush
								Sleep(100);
							
							if (!UartModem->SocketOpen)
								printf("\r\nSocket Closed.\r\n\r\n");
							
						}//Listenopen endif
						else
							printf("Socket Unable to Open.\r\n");
							
					}//portscan endif
					else
						printf("No Port Found.\r\n");
					
				}//DataRegState endif
				else
					printf("Modem Not Registered OR not context is not open.\r\n");
				
				//Reset the S memory location, without this the IP/Port may be retained
				//8 = "socketl "
				memset(s, 0, (8+1+sizeof(inPort))); 
      }			
			
			
      //==================================================================== forward
      else if (stricmp(s, "forward") == 0)
      {
        SimpleForwardingLoop(); // serial to modem
      }
      //==================================================================== fattest
      //else if (stricmp(s, "fattest") == 0)
      //{
      //  FatTest();
      //}
      //==================================================================== (unknown)
      else
      {
        printf("Unknown command, Type Q to quit\r\n");
      }

      //printf("READY - Stack:%d\n",StackDepth());
			printf("READY\r\n");

      i++;
    }
    else
      Sleep(100);
  }

  printf("Exiting Terminal Mode\r\n");
}
//****************************************************************************
// END T2 Terminal
//****************************************************************************
