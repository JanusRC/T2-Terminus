//****************************************************************************
// T2 Functionality Demos
//****************************************************************************
#include "main.h"
#include "Modem.h"
#include "ATC.h"

//Modem Characteristics
extern char ModemMake[40];			//CGMI	-	Manufacturer Information
extern char ModemModel[40];			//CGMM	-	Modem Type
extern char ModemFirmware[40];	//CGMR	-	Modem Firmware
extern char ModemSerial[40]; 		//IMEI	- International Mobile Equipment Identity, modem serial number

//SIM Information
extern char SIMSerial[40]; 		//IMSI	- International Mobile Subscriber Identity
extern char SIMIdentity[40]; 	//ICCID	- Integrated Circuit Card Identification
extern char SIMPNumber[40]; 	//CNUM	- SIM Given phone number

//Context Information
char *AccessPointName = "yourAPN";
char *UserName = "";
char *Password = "";

//Socket Information
char *RemoteServer = "your.server.here";
char *RemotePort = "5556";
char *LocalPort = "5556";


//*********************************
// Serial to GPRS Bridge Demo - Dialer/Client
//*********************************
void Demo_SerialBridge_Client(UART *UartDB9, UART *UartModem)
{
	int Fault = FAULT_OK;	//Initialize Fault State = 0
	int Tries = 4;				//Try to open the socket 4 times if it fails
	int i = 1;						//Try Counter
	
	//*********************************
	// Begin Demo
	//*********************************	
	printf("Serial Bridge Client Begin.\r\n\r\n");
	
	//Loop indefinitely
	while (1)
	{
		//Check if the modem is on
		if (!UartModem->ModemOn)
		{
			printf("Starting Modem.\r\n");
			Modem_TurnOn(UartModem, TELIT_TIME_ON);
			Modem_WaitForPowerUp(UartModem); // Wait for modem to warm up
		}
		
		//Modem turned ON, continue with initialization
		if (UartModem->ModemOn)
		{
			printf("Modem On.\r\n");
			printf("Running Modem Initialization.\r\n");
			
			//Empty any clutter from the Fifo
			Fifo_Flush(UartModem->Rx); 

			//Disable echo to keep ATC handler happy
			if (!Fault) Fault = SendAT(UartModem, "ATE0\r\n", ATC_STD_TIMEOUT); 

			//Quick check for UART responsiveness
			if (Fault == FAULT_TIMEOUT)
			{
				fprintf(STDERR, "Error: Modem not responsive.\n");
			}
			
			// Fill in Modem General Information
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGMI\r\n", ModemMake, ATC_STD_TIMEOUT);
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGMM\r\n", ModemModel, ATC_STD_TIMEOUT);
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGMR\r\n", ModemFirmware, ATC_STD_TIMEOUT);
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGSN\r\n", ModemSerial, ATC_STD_TIMEOUT);
			
			if (!Fault) printf("Modem Initialization Complete.\r\n");

			//Wait for SIM to be ready, SIM based units only
			if (strncmp(ModemModel,"HE910",5) == 0) 
			{
				printf("Waitin for SIM to be ready.\r\n");
				while (UartModem->ModemOn && !UartModem->ModemSIMReady)
					Fault = Modem_WaitForSIMReady(UartModem);
				
				if (UartModem->ModemSIMReady)
					printf("SIM Ready.\r\n");
			}
			else
			{
				//Force the structure variable for the next loop if model is not SIM applicable
				if (!Fault) UartModem->ModemSIMReady = 1;	
			}
			
			//While the modem is confirmed as being ON (main systick interrupt updates this automatically)
			//And SIM is deemed ready
			while (UartModem->ModemOn && UartModem->ModemSIMReady)
			{
				//If the SIM is ready (no fault), then check/wait for registration - both cell and data
				if (!Fault) printf("Waiting for Registration...\r\n");
				if (!Fault) Fault = Network_WaitForCellReg(UartModem, NET_REG_TIMEOUT);	//Cellular Registration
				if (!Fault) Fault = Network_WaitForDataReg(UartModem, NET_REG_TIMEOUT);	//Data Registration/readiness
				
				if (Fault)
				{
					printf("Registration Failed.\r\n");
					
					//Check the SIM card in the loop in case someone removes it or it has an issue on the fly
					if (strncmp(ModemModel,"HE910",5) == 0) 
					{
						Fault = Modem_WaitForSIMReady(UartModem);
					}
				}
			
				//Only loop the next area if we have both requirements
				while (UartModem->NetworkCellRegState && UartModem->NetworkDataRegState)
				{
					printf("Registration OK.\r\n");
					
					//Begin Network commands
					printf("Bringing up the context.\r\n");
					
					Fault = Network_SetupContext(UartModem, AccessPointName);
					if (!Fault) Fault = Network_ActivateContext(UartModem, UserName, Password);
					
					if (!Fault) printf("Context Open, Modem IP: [%s]\r\n", UartModem->NetworkIP);
					
					//Set up the socket information for the loop
					if (!Fault) Fault = Socket_Setup(UartModem, 1);
					
					//Loop the connection attempts while we have confirmed activation of the context
					while (UartModem->NetworkActivationState)
					{
						printf("Opening Socket to listener.\r\n");
						
						i=1;	//Initialize counter
						
						//Loop for specified trial attempts
						while ((i <= Tries) && (!UartModem->SocketOpen))
						{
							__WFI();
							
							Fault = Socket_Dial(UartModem, 1, RemoteServer, RemotePort, 0);
							
							i++;
							
							if (Fault == FAULT_OK)
								break;
							
							if (i <= Tries)
								printf("Failed to open, trying again (Try [%d])\r\n", i);
						}
						
						if (!Fault)
						{
							printf("Socket open, forwarding UART.\r\n");
							
							while (UartModem->SocketOpen)
							{
								//Make sure to wait for interrupts to handle the UART FIFO
								__WFI();

								Uart_ForwardFullDuplex(UartModem, UartDB9); // Modem to/from DB9
							}
							
							//Flush DB9 Port
							//Empty any clutter from the Fifo
							Fifo_Flush(UartDB9->Tx); 
							
							if (!UartModem->SocketOpen)
								printf("\r\nSocket Closed.\r\n\r\n");
						}
						else
							printf("\r\nUnable to open socket.\r\n\r\n");
						
						//Re-verify context status
						Fault = Network_GetContext(UartModem);
					}//Activationstate while
					
					printf("Context closed.\r\n");
					
					//Do a quick registration check to update the loop test
					Fault = Network_CheckCellReg(UartModem);	//Cellular Registration
					if (!Fault) Fault = Network_CheckDataReg(UartModem);	//Data Registration/readiness
				}//DataRegState endif
				
			}//modemOn/SIM While
		}//modemOn endif
	}//while end
}
	

//*********************************
// Serial to GPRS Bridge Demo - Listener/Host
//*********************************
void Demo_SerialBridge_Host(UART *UartDB9, UART *UartModem)
{
	int Fault = FAULT_OK;	//Initialize Fault State = 0
	char *String;						//Local string buffer
	int ConnID = 0;
	
	//*********************************
	// Begin Demo
	//*********************************	
	printf("Serial Bridge Host Begin.\r\n\r\n");
	
	//Loop indefinitely
	while (1)
	{
		//Check if the modem is on
		if (!UartModem->ModemOn)
		{
			printf("Starting Modem.\r\n");
			Modem_TurnOn(UartModem, TELIT_TIME_ON);
			Modem_WaitForPowerUp(UartModem); // Wait for modem to warm up
		}
		
		//Modem turned ON, continue with initialization
		if (UartModem->ModemOn)
		{
			printf("Modem On.\r\n");
			printf("Running Modem Initialization.\r\n");
			
			//Empty any clutter from the Fifo
			Fifo_Flush(UartModem->Rx); 

			//Disable echo to keep ATC handler happy
			if (!Fault) Fault = SendAT(UartModem, "ATE0\r\n", ATC_STD_TIMEOUT); 

			//Quick check for UART responsiveness
			if (Fault == FAULT_TIMEOUT)
			{
				fprintf(STDERR, "Error: Modem not responsive.\n");
			}
			
			// Fill in Modem General Information
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGMI\r\n", ModemMake, ATC_STD_TIMEOUT);
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGMM\r\n", ModemModel, ATC_STD_TIMEOUT);
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGMR\r\n", ModemFirmware, ATC_STD_TIMEOUT);
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGSN\r\n", ModemSerial, ATC_STD_TIMEOUT);
			
			if (!Fault) printf("Modem Initialization Complete.\r\n");

			//Wait for SIM to be ready, SIM based units only
			if (strncmp(ModemModel,"HE910",5) == 0) 
			{
				printf("Waitin for SIM to be ready.\r\n");
				while (UartModem->ModemOn && !UartModem->ModemSIMReady)
					Fault = Modem_WaitForSIMReady(UartModem);
				
				if (UartModem->ModemSIMReady)
					printf("SIM Ready.\r\n");
			}
			else
			{
				//Force the structure variable for the next loop if model is not SIM applicable
				if (!Fault) UartModem->ModemSIMReady = 1;	
			}
			
			//While the modem is confirmed as being ON (main systick interrupt updates this automatically)
			//And SIM is deemed ready
			while (UartModem->ModemOn && UartModem->ModemSIMReady)
			{
				//If the SIM is ready (no fault), then check/wait for registration - both cell and data
				if (!Fault) printf("Waiting for Registration...\r\n");
				if (!Fault) Fault = Network_WaitForCellReg(UartModem, NET_REG_TIMEOUT);	//Cellular Registration
				if (!Fault) Fault = Network_WaitForDataReg(UartModem, NET_REG_TIMEOUT);	//Data Registration/readiness
				
				if (Fault)
				{
					printf("Registration Failed.\r\n");
					
					//Check the SIM card in the loop in case someone removes it or it has an issue on the fly
					if (strncmp(ModemModel,"HE910",5) == 0) 
					{
						Fault = Modem_WaitForSIMReady(UartModem);
					}
				}
			
				//Only loop the next area if we have both requirements
				while (UartModem->NetworkCellRegState && UartModem->NetworkDataRegState)
				{
					printf("Registration OK.\r\n");
					
					//Begin Network commands
					printf("Bringing up the context.\r\n");
					
					Fault = Network_SetupContext(UartModem, AccessPointName);
					if (!Fault) Fault = Network_ActivateContext(UartModem, UserName, Password);
					
					if (!Fault) printf("Context Open, Modem IP: [%s]\r\n", UartModem->NetworkIP);
					
					//Set up the socket information for the loop
					if (!Fault) Fault = Socket_Setup(UartModem, 1);
					
					//Set firewall settings
					if (!Fault) Fault = Socket_SetupFirewall(UartModem, "0.0.0.0", "0.0.0.0");
					
					//Loop the connection socket opening while we have confirmed activation of the context
					while (UartModem->NetworkActivationState)
					{
						printf("Opening Listener.\r\n");
						Fault = Socket_Listen(UartModem, 1, LocalPort);
						
						if (!Fault)
						{
							printf("Listener open on port [%s], waiting for client connection...\r\n", LocalPort);
							
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
										
										//Break out of SRING check loop
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
							
							while (UartModem->SocketOpen)
							{
								__WFI();

								Uart_ForwardFullDuplex(UartModem, UartDB9); // Modem to/from DB9
							}
							
							//Flush DB9 Port
							//Empty any clutter from the Fifo
							Fifo_Flush(UartDB9->Tx); 
							
							if (!UartModem->SocketOpen)
								printf("\r\nSocket Closed.\r\n\r\n");
						}
						else
							printf("\r\nUnable to open socket.\r\n\r\n");
						
						//Re-verify context status
						Fault = Network_GetContext(UartModem);
					}//Activationstate while
					
					printf("Context closed.\r\n");
					
					//Do a quick registration check to update the loop test
					Fault = Network_CheckCellReg(UartModem);	//Cellular Registration
					if (!Fault) Fault = Network_CheckDataReg(UartModem);	//Data Registration/readiness
				}//DataRegState while
				
			}//modemOn/SIM While
		}//modemOn endif
	}//while end
}

//*********************************
// SMS Echo Demo
//*********************************
void Demo_SMSEcho(UART *UartDB9, UART *UartModem, SMSSTRUCT *SMSPointer)
{
	int Fault = FAULT_OK;		//Initialize Fault State = 0	
	
	//*********************************
	// Begin Demo
	//*********************************	
	printf("SMS Echo Begin.\r\n\r\n");
	
	//Loop indefinitely
	while (1)
	{
		//Check if the modem is on
		if (!UartModem->ModemOn)
		{
			printf("Starting Modem.\r\n");
			Modem_TurnOn(UartModem, TELIT_TIME_ON);
			Modem_WaitForPowerUp(UartModem); // Wait for modem to warm up
		}
		
		//Modem turned ON, continue with initialization
		if (UartModem->ModemOn)
		{
			printf("Modem On.\r\n");
			printf("Running Modem Initialization.\r\n");
			
			//Empty any clutter from the Fifo
			Fifo_Flush(UartModem->Rx); 

			//Disable echo to keep ATC handler happy
			if (!Fault) Fault = SendAT(UartModem, "ATE0\r\n", ATC_STD_TIMEOUT); 

			//Quick check for UART responsiveness
			if (Fault == FAULT_TIMEOUT)
			{
				fprintf(STDERR, "Error: Modem not responsive.\n");
			}
			
			// Fill in Modem General Information
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGMI\r\n", ModemMake, ATC_STD_TIMEOUT);
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGMM\r\n", ModemModel, ATC_STD_TIMEOUT);
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGMR\r\n", ModemFirmware, ATC_STD_TIMEOUT);
			if (!Fault) Fault = SendATStr(UartModem, "AT+CGSN\r\n", ModemSerial, ATC_STD_TIMEOUT);
			
			if (!Fault) printf("Modem Initialization Complete.\r\n");

			//Wait for SIM to be ready, SIM based units only
			if (strncmp(ModemModel,"HE910",5) == 0) 
			{
				printf("Waitin for SIM to be ready.\r\n");
				while (UartModem->ModemOn && !UartModem->ModemSIMReady)
					Fault = Modem_WaitForSIMReady(UartModem);
				
				if (UartModem->ModemSIMReady)
				{
					printf("SIM Ready.\r\n");
					//Update phone number
					Modem_GetPhoneNum(UartModem, SIMPNumber);
				}
			}
			else
			{
				//Force the structure variable for the next loop if model is not SIM applicable
				if (!Fault) UartModem->ModemSIMReady = 1;	
			}
			
			//While the modem is confirmed as being ON (main systick interrupt updates this automatically)
			//And SIM is deemed ready
			while (UartModem->ModemOn && UartModem->ModemSIMReady)
			{
				//If the SIM is ready (no fault), then check/wait for registration - both cell and data
				if (!Fault) printf("Waiting for Registration...\r\n");
				if (!Fault) Fault = Network_WaitForCellReg(UartModem, NET_REG_TIMEOUT);	//Cellular Registration
				if (!Fault) Fault = Network_WaitForDataReg(UartModem, NET_REG_TIMEOUT);	//Data Registration/readiness
				
				if (Fault)
				{
					printf("Registration Failed.\r\n");
					
					//Check the SIM card in the loop in case someone removes it or it has an issue on the fly
					if (strncmp(ModemModel,"HE910",5) == 0) 
					{
						Fault = Modem_WaitForSIMReady(UartModem);
						
						if (UartModem->ModemSIMReady)
							Fault = Modem_GetPhoneNum(UartModem, SIMPNumber);
					}
				}
				
				if (!Fault) Fault = SMS_SetupSMS(UartModem);	
				
				if (!Fault) printf("Waiting for incoming SMS to Phone Number [%s]...\r\n", SIMPNumber);
			
				//Loop the SMS echo while we have registration
				while (UartModem->NetworkCellRegState)
				{
					//Check for stored SMS
					Fault = SMS_CheckSMS(UartModem, SMSPointer);
					
//					//Fault response check with available messages
//					if (!Fault) 
//						printf("[%d] Messages Available.\r\n", SMSPointer->NumOfStored);
					
					//If we have stored SMS, parse the last one stored
					if (SMSPointer->NumOfStored)
					{
						printf("Processing Message [%d].\r\n", SMSPointer->NumOfStored);
						if (!Fault) Fault = SMS_ProcessSMS(UartModem, SMSPointer, SMSPointer->NumOfStored);
						
						//Fault response check with parsed information
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
							
							//Echo SMS back to sender
							printf("Echoing Message [%d].\r\n", SMSPointer->StoreIndex);
							Fault = SMS_SendSMS(UartModem, SMSPointer, SMSPointer->Data, SMSPointer->OriginatingPN);
							
							//Fault response check with the outbound SMS index
							if (!Fault)
								printf("SMS Sent, Reference Index: [%d].\r\n", SMSPointer->OutIndex);
							
							if (!Fault)
							{
								//Erase the SMS
								printf("Erasing stored SMS [%d].\r\n", SMSPointer->NumOfStored);
								Fault = SMS_DeleteOneSMS(UartModem, SMSPointer->NumOfStored);
								
								if (!Fault) printf("Waiting for incoming SMS to Phone Number [%s]...\r\n", SIMPNumber);
							}
						
						}//Fault endif
					}//NumofStored endif
					
					//Do a quick registration check to update the loop test
					Fault = Network_CheckCellReg(UartModem);	//Cellular Registration
					if (!Fault) Fault = Network_CheckDataReg(UartModem);	//Data Registration/readiness
				}//DataRegState while
				
			}//modemOn While
		}//modemOn/SIM endif
	}//while end
}
