/*******************************
T2 Pin definitions
*******************************/
//30P Header 
//General Use        //Pin #:  Default | Alternate1 |  Alternate 2
//----------------------------------------------------------------
var GPIO1 = A15;     //Pin 14: GPIO    | SPI1_CS
var GPIO2 = B3;      //Pin 16: GPIO    | SPI1_SCK
var GPIO3 = B4;      //Pin 18: GPIO    | SPI1_MISO
var GPIO4 = B5;      //Pin 20: GPIO    | SPI1_MOSI
var GPIO5 = B8;      //Pin 22: GPIO    | I2C1_SCL
var GPIO6 = B9;      //Pin 24: GPIO    | I2C1_SDA
var GPIO7 = A0;      //Pin 26: GPIO    | UART4_TX   |  ADC123_CHO
var GPIO8 = A1;      //Pin 28: GPIO    | UART4_RX   |  ADC123_IN1
var GPIO9 = A2;      //Pin 30: GPIO    | USART2_TX  |  ADC123_IN2
var GPIO10 = A3;     //Pin 29: GPIO    | USART2_RX  |  ADC123_IN3
var GPIO11 = A4;     //Pin 25: GPIO    | ADC12_IN4  |  DAC1
var GPIO12 = A5;     //Pin 23: GPIO    | ADC12_IN5  |  DAC2
var GPIO13 = A6;     //Pin 21: GPIO    | ADC12_IN6
var GPIO14 = A7;     //Pin 19: GPIO    | ADC12_IN7

//Special Use        //Pin #:  Special Function
//----------------------------------------------------------------
var IGNITION = C13;  //Pin 3:  Input, Optically Isolated, must pull up
//20mA Current Loop
var CL1_RESET = D14; //N/A  :  Output, Active low reset
var CL1_TRIP = D13;  //N/A  :  Input, Active high overcurrent trip
var CL1_VOUT = B0;   //N/A  :  ADC Input, ADC12_IN8
var CL2_RESET = E0;  //N/A  :  Output, Active low reset
var CL2_TRIP = D15;  //N/A  :  Input, Active high overcurrent trip
var CL2_VOUT = B1;   //N/A  :  ADC Input, ADC12_IN9


//Special Use (Not MCU connected, just documentation) 
//Pin #:  Description
//----------------------------------------------------------------
//Pin 13:  CL1 Power Output
//Pin 15:  CL2 Power Output
//Pin 1:   T2 Power Enable
//Pin 2:   T2 12V Input
//Pin 5:   RS485 A
//Pin 6:   RS485 B
//Pin 7:   Modem AUX RS232 RX
//Pin 8:   Modem AUX RS232 TX
//Pin 9:   CAN LO
//Pin 10:  CAN HI


//LEDs
//Only for reference. These are defined internally in the python board script.
//LED1 = GREEN
//LED2 = YELLOW
//LED3 = RED


//Embedded Modem
//----------------------------------------------------------------
//Control
var MDM_ENABLE = C1;
var MDM_ON_OFF = C2;
var MDM_RESET = C3;
var MDM_SERVICE = C4;
var MDM_VBUS_EN = C5;
var MDM_PWRMON = G0;
var MDM_STAT_LED = G1;
var MDM_GPS_RESET = D10;
//UART
var RS232_DSR = D3;
var RS232_RING = D7;
var RS232_DCD = D8;
var RS232_DTR = D9;
var RS232_CTS = G15;
var RS232_RTS = G12;
var FORCEOFF = F4;
var FORCEON = F3;
var INVALID = E15;
//GPIO
var MDM_GPIO1 = G6;
var MDM_GPIO2 = G5;
var MDM_GPIO3 = G8;
var MDM_GPIO4 = G7;
var MDM_GPIO5 = G2;
var MDM_GPIO6 = G3;
var MDM_GPIO7 = G4;
//Misc.
var CRLF = '\r\n'; //helpful global

/*************************************************
Initializations for the T2 Environment
These are all called/used in the onInit() function
so they get run at startup.
*************************************************/

function CycleDown() {
  digitalWrite([LED1,LED2,LED3],0b100);
  setTimeout("digitalWrite([LED1,LED2,LED3],0b010);", 200);
  setTimeout("digitalWrite([LED1,LED2,LED3],0b001);", 400);
  setTimeout("digitalWrite([LED1,LED2,LED3],0);", 600);
}
function CycleUp() {
  digitalWrite([LED1,LED2,LED3],0b001);
  setTimeout("digitalWrite([LED1,LED2,LED3],0b010);", 200);
  setTimeout("digitalWrite([LED1,LED2,LED3],0b100);", 400);
  setTimeout("digitalWrite([LED1,LED2,LED3],0);", 600);
}

function mdmSetIO() {
  //This funciton sets the modem I/O to a safe/usable state
  //Modem Control I/O, all open drain config
  pinMode(MDM_ENABLE,'opendrain');    //Modem Enable
  pinMode(MDM_ON_OFF,'opendrain');    //Modem ON/OFF
  pinMode(MDM_RESET,'opendrain');     //Modem Reset
  pinMode(MDM_SERVICE,'opendrain');   //Modem Service
  pinMode(MDM_VBUS_EN,'opendrain');   //Modem Enable VBUS
  pinMode(MDM_GPS_RESET,'opendrain'); //Modem GPS Reset

  //USART
  //Controls commented out since we've manually patched it temporarily
  //pinMode(INVALID,'input');       //INVALID
  //pinMode(FORCEON,'output');      //FORCEON
  //pinMode(FORCEOFF,'output');     //FORCEOFF
  pinMode(RS232_DSR,'input');     //DSR
  pinMode(RS232_RING,'input');    //RING
  pinMode(RS232_DCD,'input');     //DCD
  pinMode(RS232_DTR,'output');    //DTR

  //Modem Feedback I/O, set to floating inputs
  pinMode(MDM_PWRMON,'input');        //Modem PWRMON
  pinMode(MDM_STAT_LED,'input');      //Modem CELL LED

  //GPIO, set to open drain as a safe state
  pinMode(MDM_GPIO1,'opendrain');     //Modem GPIO1
  pinMode(MDM_GPIO2,'opendrain');     //Modem GPIO2
  pinMode(MDM_GPIO3,'opendrain');     //Modem GPIO3
  pinMode(MDM_GPIO4,'opendrain');     //Modem GPIO4
  pinMode(MDM_GPIO5,'opendrain');     //Modem GPIO5
  pinMode(MDM_GPIO6,'opendrain');     //Modem GPIO6
  pinMode(MDM_GPIO7,'opendrain');     //Modem GPIO7
}

function mdmReadyIO() {
  //This function sets the modem control I/O to the "ready" state
  digitalWrite(MDM_ENABLE,1);         //Enable released
  digitalWrite(MDM_RESET,1);          //Reset released
  digitalWrite(MDM_ON_OFF,1);         //ON/OFF released
  digitalWrite(MDM_VBUS_EN,1);        //VBUS Enable released
}

function mdmCellLED() {
  //Pass the cellular status LED through to LED2
  var Value = 0; //Initialize value
  Value = digitalRead(MDM_STAT_LED);
  digitalWrite(LED2,Value);
}

function onInit() {
//Alive and kicking
setTimeout("CycleDown();", 0);
setTimeout("CycleUp();", 800);

//Set up T2 for usage with cell status LED routed through
mdmSetIO();             //Set T2 I/O Up
mdmReadyIO();           //Set T2 control I/O to proper states
mdmCellLED();           //Pass the LED through to LED2
mdmSerialInit(115200);  //Initialize the serial interface to the default 115200 baud
var StatLEDTick = setInterval(mdmCellLED, 100);  //Allow the status LED to update LED2
}

/*************************************************
Modem Control and communications
These are all called discretely for now
*************************************************/

function mdmGetPWRMON() {
  //Returns the PWRMON status
  return digitalRead(MDM_PWRMON);
}

function mdmTurnOnOff(TimeIn) {
  //Takes in a time argument to dynamically set for different terminus modules
  //asserts and releases the ON/OFF I/O
  digitalWrite(MDM_ON_OFF,0); //Assert ON/OFF
  setTimeout("digitalWrite(ON_OFF,1);", TimeIn); //Release ON/OFF
}

function mdmReset() {
  digitalWrite(MDM_RESET,0); //Assert Reset
  setTimeout("digitalWrite(RESET,1);", 500); //Release Reset
}

function mdmDisable() {
  //This disables the terminus power supply
  return digitalWrite(MDM_ENABLE,0); //Enable asserted
}

function mdmEnable() {
  //This funciton enables the terminus power supply
  return digitalWrite(MDM_ENABLE,1); //Enable released
}

/*************************************************
Serial communications control

*************************************************/

/*****************
Discrete RS232
*****************/
function mdmDTROn() {
  return digitalWrite(RS232_DTR,1);
}

function mdmDTROff() {
  return digitalWrite(RS232_DTR,0);
}

function mdmGetDTR() {
  return digitalRead(RS232_DTR);
}

function mdmGetDSR() {
  return digitalRead(RS232_DSR);
}

function mdmGetRING() {
  return digitalRead(RS232_RING);
}

function mdmGetDCD() {
  return digitalRead(RS232_DCD);
}

/*****************
UART Handling
*****************/

function mdmSerialInit(MDMBaud) {
  //Initialize the modem serial interface
  Serial6.setup(MDMBaud);

  //Disable the flow control for now to avoid issues with sockets
  mdmDisableHWFC();
}

function mdmDisableHWFC() {
   pinMode(RS232_RTS,'output'); //RTS init
   digitalWrite(RS232_RTS,0);   //RTS = 0 to remove FC
}

/*****************
UART Instances
*****************/

function mdmTerminal_Disable() {
  Serial6.removeAllListeners('data');
  Serial3.removeAllListeners('data');
}

function mdmTerminal_RS232(MDMBaud, DB9Baud) {
  //Takes in two baudrate arguments, sets up the serial interfaces
  //Then creates instances wherein any data gets passed through
  //to the other port, unfiltered.
  //After the setup, we move the console to USB specifically
  //This allows a backdoor as well as keeps the UART channels clear.

  console.log("Beginning modem terminal");
  Serial6.setup(MDMBaud);
  Serial3.setup(DB9Baud);  
  Serial6.on('data', function (data) {Serial3.write(data);});
  Serial3.on('data', function (data) {Serial6.write(data);});
  USB.setConsole();
}


/*****************
AT Handling
*****************/

function mdmSendAT(inData, inTerminator) {
  /**************************************
  This function acts as a send/receive front end for AT commands
  If the CTS flag is set, we send the incoming AT command, and then clear it
  If called again, it looks for an incoming, valid, AT response
  The receiveAT function grabs data if there is any, which will discard URCs
  If a URC or otherwise is found, we return a -2 and let the handler decide
  what to do.
  NOTE: Ensure local echo is off on the modem, otherwise this won't work right.
  ***************************************/

  /*************
  Returns:
  -1: No data in buffer
  -2: Something other than a valid AT (could be a URC)
  Otherwise: Parsed AT response
  *************/
  res = "";

  if (CTS === 1) {
    Serial6.println(inData);
    CTS = 0;   //Flag CTS
    return -1;
  } else {
    res = mdmReceiveAT(inTerminator);
    if (res !== -1 && res !== -2) {
      //Got a valid AT in the buffer, parse it
      CTS = 1; //Reset the send flag
      return mdmParseResponse(res);
    }
    //Got something other than an AT response in the buffer, discard it.
    else {
      CTS = 1; //Flag CTS
      return -2;
    }
  }
}

function mdmParseResponse(inSTR) {
  //This function parses out the responses to readable format
  //It recognizes non-AT style responses by the \r\n and just returns them.

  if (inSTR !== -1 && inSTR.length > 0) {
    rtnSTR= inSTR;

    if (inSTR.indexOf('\r\n',0) !== -1) {
      splitList = inSTR.split('\r\n');
      rtnSTR = splitList[1];
    }

  }
return rtnSTR;
}

function mdmReceiveAT(theTerminator) {
  //This function waits for a AT command response and handles errors
  //This ignores/trashes unsolicited responses.

  /*************
  Returns:
  -1: No data in buffer
  -2: Something other than a valid AT (could be a URC)
  Otherwise: Raw AT response
  *************/

  ret = -1;
  var check1 = -1;
  var check2 = -1;

  if (Serial6.available() > 0) {
    //Something's in the buffer, read it to clear.
    ret = Serial6.read();
    check1 = ret.indexOf("ERROR",0);
    check2 = ret.indexOf(theTerminator,0);

    if (check1 !== -1 || check2 !== -1) {
      return ret;
    } else {
      //Data in buffer is not a wanted AT response
      return -2;
      }
  }
  //No data in the buffer, return -1
return ret;
}


/*************************************************
Autonomous Applications
*************************************************/

/*****************
App Globals
*****************/
var AppInterval = 1000; //Set the default "ticktock" interval to 1s.
var T2AppTick = "";     //Set the default interval ID
var CurrentApp = "";    //Sets the current program state switch
var AppSwitch = "";     //Set the default app state swtich
var CTS = 1;            //Set the default CTS value
var ModemType = "";     //Set the default modem type
var ConnectionID = "";  //Set the default connection ID

/*****************
User Configuration
*****************/
//General Information/storage
var myAPN = "i2gold";         //Set the application APN
var myPhone = "";             //Set the application phone number
var LocalIP = "";             //Set the default application local IP address
var LocalPort = 5556;         //Set the default application local port

var ConnMode = "Host";
var ConnType = "Data";

//Socket Dialing Host Info
var RemoteHost = "clayton.conwin.com";  //Set the application remote host
var RemotePort = 5556;                  //Set the application remote port

//Socket Listening Whitelist
/*
Connection is accepted if the criteria is met:
  incoming_IP & <net_mask> = <ip_addr> & <net_mask>
  
Example:
  Possible IP Range = 197.158.1.1 to 197.158.255.255
  We set:
  ClientIP = "197.158.1.1",
  ClientIPMask = "255.255.0.0"
*/
var ClientIP = "0.0.0.0";      //Set the incoming client IP ddress
var ClientIPMask = "0.0.0.0";  //Set the incoming client IP mask



/*****************
App Calls
*****************/

function RunApp(inApp) {
  //This function pulls in the wanted program/app and will set it to 
  //a default interval
  AppSwitch = "";   //Make a default state before running
  CurrentApp = "";  //Make a default state before running
  T2AppTick = setInterval(inApp, AppInterval);
}

function StopApp() {
  //Kill the application interval, stopping the running program/application loop
  clearInterval(T2AppTick);
}

function ChangeAppClock(newInterval) {
  //Change the interval at which the program/application runs
  changeInterval(T2AppTick, newInterval);
}

/*****************
Programs
These are the main "Tick" Programs. They will run on an interval
going through what is basically a state machine, calling the apps
necessary to complete the needed autonomous function.
*****************/

function TestProgram() {
  /***************
  Test overlord test program
  ***************/
  var res = "";

  switch (CurrentApp) {
    case "PowerUp":
      res = T2MDMWakeUp();
      if (res === 1) {
        CurrentApp = "RegCheck";
      }
      break;

    case "RegCheck":
      res = T2MDMRegCheck();
      if (res === 1) {
        CurrentApp = "Finished";
      }
      break;

    case "Finished":
      console.log("Overlord Test Program Finished.");
      CurrentApp = "";
      StopApp();
      break;

    default:
      console.log("Running T2 Test Overlord Program");
      //Make sure it always starts at the beginning
      CurrentApp = "PowerUp";
  }
}

function S2GPRSBridge() {
  /***************
  Test Serial Bridge Demo
  Opens a GPRS Bridge connection via socket dial or socket listen
  Note: When ConnMode = Host, ConnType is DNC

  ConnMode:
  Host = Socket Listener for a remote client
  Client = Socket Dial to a remote host


  ConnType:
  Data = online/data mode (pipe)
  Command = command mode
  ***************/
  var res = "";

  switch (CurrentApp) {
    case "Start":
      res = T2MDMWakeUp();
      if (res === 1) {
        CurrentApp = "RegCheck";
      }
      break;

    case "RegCheck":
      res = T2MDMRegCheck();
      if (res === 1) {
        CurrentApp = "NetworkSetup";
      }
      break;

    case "NetworkSetup":
      res = T2MDMGSMNetworkSetup();
      if (res === 1) {
        CurrentApp = "OpenSocket";
      }
      break;

    /*********
    Create Socket
    *********/

    case "OpenSocket":
      //Opens the socket in the specified mode, then branches to the handler
      res = T2MDMOpenSocket(ConnMode,ConnType);
      if (res === 1 && ConnMode === "Client" && ConnType === "Data") {
        mdmTerminal_RS232(115200,115200); //Create bridge instance
        LEDBreathe(LED3,10);
        CurrentApp = "DataClient";
      } else if (res === 1 && ConnMode === "Client" && ConnType === "Command") {
        //Do nothing for now, just exit
        CurrentApp = "Finished";
      } else if (res === 1 && ConnMode === "Host") {
        console.log("Waiting for client connection...");
        CurrentApp = "DataHostWait";
      }
      break;

    /*********
    Client Socket Dial
    *********/

    case "DataClient":
      res = mdmGetDCD();
      //Loop for Online/Data mode until we break out of the connection
      //Remember that UART inactive state is logic 1.
      //DCD = 1 : No socket
      //DCD = 0 : Socket open
      if (res === 1) {
        console.log("Socket Closed.");
        mdmTerminal_Disable();
        StopBreathe(LED3);
        CurrentApp = "Finished";
      }
      break;

    /*********
    Host Socket Listen
    *********/

    case "DataHostWait":
      //Check to make sure we aren't online already, check for incoming connection
      res = mdmGetDCD();
      if (res === 1) {
        //Not online yet, look for an incoming connection
        res = mdmReceiveAT("SRING");
        if (res !== -1 && res !== -2) {
          //incoming found, parse the connection ID
          var splitList = mdmParseResponse(res).split(' ');
          ConnectionID = splitList[1];
          CurrentApp = "DataHostAccept";
        }
      } else if (res === 0) {
          //Already have a connection open, drop right into the loop
          mdmTerminal_RS232(115200,115200); //Create bridge instance
          LEDBreathe(LED3,10);
          CurrentApp = "DataHostLoop";
      }
      break;

    case "DataHostAccept":
     //Accept the incoming connection, create the bridge when DCD indicates OK
      res = mdmSendAT('AT#SA=' + ConnectionID,"CONNECT");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        console.log("Client connection # " + ConnectionID + " open");
        mdmTerminal_RS232(115200,115200); //Create bridge instance
        LEDBreathe(LED3,10);
        CurrentApp = "DataHostLoop";
      } else {
        console.log("Accepting Client Connection: " + res);
      }
      break;

    case "DataHostLoop":
      res = mdmGetDCD();
      //Loop for Online/Data mode until we break out of the connection
      //Remember that UART inactive state is logic 1.
      //DCD = 1 : No socket
      //DCD = 0 : Socket open
      if (res === 1) {
        console.log("Socket Closed.");
        mdmTerminal_Disable();
        StopBreathe(LED3);
        CurrentApp = "Finished";
      }
      break;

    /*********
    End
    *********/

    case "Finished":
      console.log("Program Finished.");
      CurrentApp = "";
      StopApp();
      break;

    default:
      console.log("Running Serial 2 GPRS Bridge Demo");
      //Make sure it always starts at the beginning
      CurrentApp = "Start";
  }
}

/*****************
Apps
These are the "Tock" applications. They will run through their function
via a state machine, on an interval set by the main program
Returns:
0: Not Complete
1: Complete
*****************/

function T2MDMWakeUp() {
  /***************
  Wakeup Application.
  This application wakes up the modem, and gets it ready for usage by 
  removing the local echo. 
  ***************/
  var res = "";
  var rtn = 0;

  switch (AppSwitch) {
   //This area controls the basic sequencing of the modem
   //****************************************************
    case "Start":
      if (mdmGetPWRMON() === 0){
        console.log("-->Powering up the modem");
        mdmTurnOnOff(5000);
      } else {
        mdmSerialInit(115200);
        console.log("-->Modem On, checking for AT");
        AppSwitch = "ATCheck";
      }
      break;
    case "ATCheck":
      //use the sendAT function to check for a response and turn off echo
      res = mdmSendAT('ATE0',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        AppSwitch = "SetVerbose";
      }
      break;
    case "SetVerbose":
      res = mdmSendAT('AT+CMEE=2',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        AppSwitch = "Finished";
      }
      break;
   //App finished
   //****************************************************
    case "Finished":
      console.log("-->AT Found, Modem is ready");
      AppSwitch = "";
      rtn = 1;
      break;
    default:
      console.log("");
      console.log("**Running T2 Modem Wakeup App**");
      //Make sure it always starts at the beginning
      AppSwitch = "Start";
  }

  return rtn;
}


function T2MDMRegCheck() {
  /***************
  Network check application
  This application builds on the above and 
  checks for network availability
  ***************/
  var res = "";
  var rtn = 0;

  switch (AppSwitch) {
   //This area controls the network checking
   //****************************************************
    case "Start":
      //Checks the system registration
      res = mdmSendAT('AT+CGMM',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (res.indexOf("HE910",0) !== -1) {
            ModemType = res;
            AppSwitch = "SIMCheck";
        } else if (res.indexOf("DE910",0) !== -1) {
            ModemType = res;
            AppSwitch = "CellRegCheck";
        } else {
          console.log("-->Modem not recognized: " + res);
        }
      }
      break;

    case "SIMCheck":
      //Checks for SIM ready
      res = mdmSendAT('AT+CPIN?',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (res.indexOf("+CPIN: READY",0) !== -1) {
            console.log("-->SIM Ready");
            AppSwitch = "CellRegCheck";
        } else {
          console.log("-->SIM Not Ready: " + res);
        }
      }
      break;

    case "CellRegCheck":
      //Checks the system registration
      res = mdmSendAT('AT+CREG?',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (res.indexOf(",1",0) !== -1 || res.indexOf(",5",0) !== -1) {
            console.log("-->Modem Registered");
            console.log("-->checking data availability");
            AppSwitch = "DataRegCheck";
        } else {
          console.log("-->Modem not registered: " + res);
        }
      }
      break;

    case "DataRegCheck":
      //Checks the system registration
      res = mdmSendAT('AT+CGREG?',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (res.indexOf(",1",0) !== -1 || res.indexOf(",5",0) !== -1) {
            console.log("-->Modem Ready for Data");
            AppSwitch = "Finished";
        } else {
          console.log("-->Modem not data ready: " + res);
        }
      }
      break;

   //App finished
   //****************************************************
    case "Finished":
      console.log("-->Network reg. check complete.");
      AppSwitch = "";
      rtn = 1;
      break;

    default:
      console.log("");
      console.log("**Running T2 RegCheck App**");
      //Make sure it always starts at the beginning
      AppSwitch = "Start";
  }
  return rtn;
}


function T2MDMGSMNetworkSetup() {
  /***************
  Network setup application
  This sets the context information
  ***************/
  var res = "";
  var rtn = 0;

  switch (AppSwitch) {
   //This area controls the network information setup
   //****************************************************
    case "Start":
      //Checks the system registration
      res = mdmSendAT('AT+CGDCONT=1,"IP","' + myAPN + '",',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        AppSwitch = "Finished";
      }
      break;

   //App finished
   //****************************************************
    case "Finished":
      console.log("-->Network setup complete.");
      AppSwitch = "";
      rtn = 1;
      break;

    default:
      console.log("");
      console.log("**Running T2 GSM Network Set App**");
      //Make sure it always starts at the beginning
      AppSwitch = "Start";
  }
  return rtn;
}

function T2MDMOpenSocket(inMode, inType) {
  /***************
  Opens a TCP connection via socket dial or socket listen
  Note: When inMode = Host, inType is DNC

  inMode:
  Host = Socket Listener for a remote client
  Client = Socket Dial to a remote host


  inType:
  Data = online/data mode (pipe)
  Command = command mode
  ***************/
  var res = "";
  var rtn = 0;

  switch (AppSwitch) {
   //This area controls socket opening
   //****************************************************
    case "Start":
      //Disable HW flow control.
      res = mdmDisableHWFC();
      AppSwitch = "CheckContext";
      break;

    case "CheckContext":
      //Disable the context to ensure a known state and fresh context activation.
      //This will always report an OK, so it's safe to do this.
      res = mdmSendAT('AT#SGACT=1,0',"OK");
      if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
          AppSwitch = "ActivateContext";
      }
      break;

    case "ActivateContext":
      //Activate the context
      res = mdmSendAT('AT#SGACT=1,1',"SGACT:");
      if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
        //Parse the IP and save it
        splitList = res.split(' ');
        LocalIP = splitList[1];
        console.log("-->Context Active, Modem IP#: " + LocalIP);
        AppSwitch = "SocketInfo";
      } else {
        console.log("-->Context Not Active: " + res);
      }
      break;

    case "SocketInfo":
      //Display socket connection information
      if (inMode === "Client") {
        console.log("-->Opening a " + inType + " socket to: " + RemoteHost + ":" + RemotePort);
        if (inType === "Data"){
          AppSwitch = "DataSocket";
        } else if (inType === "Command") {
          AppSwitch = "CommandSocket";
        } else {
          console.log("-->Unknown Socket Type: " + inType);
        }
      } else if (inMode === "Host") {
        console.log("-->Opening a listener at: " + LocalIP + ":" + LocalPort);
        AppSwitch = "FireWallSetup";
      }
      break;

    case "DataSocket":
      //Open the socket in online/data mode
      res = mdmSendAT('AT#SD=1,0,' + RemotePort + ',"' + RemoteHost + '",0,0,0',"CONNECT");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        AppSwitch = "Finished";
      } else {
        console.log("-->Socket Not Open: " + res);
      }
      break;

    case "CommandSocket":
      //Open the socket in command mode
      res = mdmSendAT('AT#SD=1,0,' + RemotePort + ',"' + RemoteHost + '",0,0,1',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        AppSwitch = "Finished";
      } else {
        console.log("-->Socket Not Open: " + res);
      }
      break;

    case "FireWallSetup":
      //We have to set up a whitelist or we'll see nothing incoming
      //This is to avoid accidental data charges/connections
      res = mdmSendAT('AT#FRWL=1,"' + ClientIP + '","' + ClientIPMask + '"',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        AppSwitch = "DataListener";
      }
      break;

    case "DataListener":
      //Open the socket listen on specified port
      res = mdmSendAT('AT#SL=1,1,' + LocalPort,"OK");
      if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
        AppSwitch = "Finished";
      } else {
        console.log("-->Socket Not Open: " + res);
      }
      break;

   //App finished
   //****************************************************
    case "Finished":
      console.log("-->Socket open complete.");
      AppSwitch = "";
      rtn = 1;
      break;

    default:
      console.log("");
      console.log("**Running T2 Socket Open App**");
      //Make sure it always starts at the beginning
      AppSwitch = "Start";
  }
  return rtn;
}





/*************************************************
Misc.
*************************************************/

/*****************
LED Fun
It appears that LED1 and LED2 can't really be utilized together for a breathing effect
Likely beccause of a hardware limitation/contention.
You can do a breathe on either LED1 or LED2, and then a 
digital or analog write discretely on the other, but you cannot have a breathe effect on both.

This is fine for the T2 as the yellow LED is usually utilized as the Status LED anyway.
*****************/

function LEDBreathe(LEDIn,BreatheMs) {

  if (LEDIn === LED1){
    LED1Tick = setInterval(LED1Breathe, BreatheMs);
  }
  else if (LEDIn === LED2) {
    LED2Tick = setInterval(LED2Breathe, BreatheMs);
  }
  else if (LEDIn === LED3) {
    LED3Tick = setInterval(LED3Breathe, BreatheMs);
  }
}

function ChangeBreathe(LEDIn, BreatheMs) {
  if (LEDIn === LED1){
    changeInterval(LED1Tick, BreatheMs);
  }
  else if (LEDIn === LED2) {
    changeInterval(LED2Tick, BreatheMs);
  }
  else if (LEDIn === LED3) {
    changeInterval(LED3Tick, BreatheMs);
  }
}

function StopBreathe(LEDIn) {
  if (LEDIn === LED1){
    clearInterval(LED1Tick);
    digitalWrite(LED1,0); //Turn it off
  }
  else if (LEDIn === LED2) {
    clearInterval(LED2Tick);
    digitalWrite(LED2,0); //Turn it off
  }
  else if (LEDIn === LED3) {
    clearInterval(LED3Tick);
    digitalWrite(LED3,0); //Turn it off
  }
}

var LEDCounter1 = 0;
var CountDirection1 = "Up";
function LED1Breathe() {

  var Value = LEDCounter1/100;
  analogWrite(LED1,Value);

  if (CountDirection1 === "Up") {
    LEDCounter1++;
  }
  else {
    LEDCounter1--;
  }

  if (LEDCounter1 > 99) {
    CountDirection1 = "Down";
  }
  else if (LEDCounter1 < 1) {
    CountDirection1 = "Up";
  }

}

var LEDCounter2 = 0;
var CountDirection2 = "Up";
function LED2Breathe() {

  var Value = LEDCounter2/100;
  analogWrite(LED2,Value);

  if (CountDirection2 === "Up") {
    LEDCounter2++;
  }
  else {
    LEDCounter2--;
  }

  if (LEDCounter2 > 99) {
    CountDirection2 = "Down";
  }
  else if (LEDCounter2 < 1) {
    CountDirection2 = "Up";
  }

}

var LEDCounter3 = 0;
var CountDirection3 = "Up";
function LED3Breathe() {

  var Value = LEDCounter3/100;
  analogWrite(LED3,Value);

  if (CountDirection3 === "Up") {
    LEDCounter3++;
  }
  else {
    LEDCounter3--;
  }

  if (LEDCounter3 > 99) {
    CountDirection3 = "Down";
  }
  else if (LEDCounter3 < 1) {
    CountDirection3 = "Up";
  }

}



  
function delay_ms(a) {
  var et=getTime()+a/1000;
  while (getTime() < et) {
 }
}
