//****************************************************************************
// Modem SMS Handling
//****************************************************************************

//****************************************************************************
//Main SMS structure, holds all SMS information
//-Technical Notation-
//+CMGR: <index>,<stat>,<oa/da>,<alpha>,<scts><CR><LF><data><CR><LF>
//
//-Plaintext Example-
//+CMGR: 1,"REC UNREAD","+12223334444","","15/02/11,12:37:17-24"
//Test12345
//OK
//****************************************************************************
typedef struct _SMSSTRUCT {

	int NumOfStored;					//Amount of stored SMS
  int StoreIndex;						//Procecced SMS Index number
  char Stat[16];						//Procecced SMS Status type
	char OriginatingPN[16];		//Procecced Originating phone number
  char Alpha[16]; 					//Procecced Alphanumeric representation of O.PN if a known phonebook entry
  char Date[16]; 						//Procecced TP-Service Centre Date Stamp
	char Time[16]; 						//Procecced TP-Service Centre Time Stamp
	int Type;									//Procecced Type of number, 129 is national, 145 is international
	int Length;								//Procecced Length of SMS
	char Data[255];						//Procecced SMS Data
	int OutIndex;							//Special container spot for outbound SMS ID# storage

} SMSSTRUCT;
