/*************************************************
Initializations for the T2 Environment through 
the use of the onInit() automatic call and modules
*************************************************/

/************
//Application Globals
************/
var MDM_BAUD = 115200;   //Modem baud rate to use
var MDM_ONOFF_T = 5000;  //Modem turn on/off pulse time
var DEBUG = true;        //Use print statements or not


/************
//Application includes/modules
************/
//Invoke the hardware - REQUIRED
var myHW = require("T2HW").T2HW();

//Transpose the T2 Structure, noted above, for the CF910 module
//The hardware pins are noted in the module, but commented out. Doing this will allow
//modularity with other systems than the T2.
var CF910_IO = myHW.MODEM;

//Modem control
var mdm = require("CF910").CF910(myHW.MODEM.SERIAL,MDM_BAUD,1);

//AT Command interpreter
var atc = require("ATC").ATC(myHW.MODEM.SERIAL);

//Network Library, you MUST PASS the ATC instance you wish to use
var myNetwork = require("NETWORK").NETWORK(atc, DEBUG);

//Socket Library, you MUST PASS the ATC instance you wish to use
var mySocket = require("SOCKET").SOCKET(atc,1,1,"i2gold", DEBUG);

//Possible secondary socket instance (i.e. 1 client and 1 host)
//var mySocket2 = require("SOCKET").SOCKET(atc,1,2,"i2gold", DEBUG);


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

function onInit() {
  //Alive and kicking
  setTimeout("CycleDown();", 0);
  setTimeout("CycleUp();", 800);
}


/*****************
UART Instances
You can utilize these to talk with the modem directly if needed
Good for debugging or setting something quickly.
*****************/

function mdmTerminal_Disable() {
  myHW.MODEM.SERIAL.removeAllListeners('data');
  myHW.DB9.SERIAL.removeAllListeners('data');
}

function mdmTerminal_RS232(MDMBaud, DB9Baud) {
  //Takes in two baudrate arguments, sets up the serial interfaces
  //Then creates instances wherein any data gets passed through
  //to the other port, unfiltered.
  //After the setup, we move the console to USB specifically
  //This allows a backdoor as well as keeps the UART channels clear.

  console.log("Beginning modem terminal");
  mdm.serial.setup(MDMBaud);
  myHW.DB9.SERIAL.setup(DB9Baud);  
  mdm.serial.on('data', function (data) {myHW.DB9.SERIAL.write(data);});
  myHW.DB9.SERIAL.on('data', function (data) {mdm.serial.write(data);});
  USB.setConsole();
}

/*************************************************
Autonomous Applications
*************************************************/

/*****************
App Globals
*****************/
var AppInterval = 100;  //Set the default "ticktock" interval to 100ms.
var T2AppTick = "";     //Set the default interval ID
var CurrentApp = "";    //Set the default program state switch
var AppSwitch = "";     //Set the default "general" app state swtich
var ConnectionID = "";  //Set the default connection ID


/*****************
User Configuration
*****************/
//General Information/storage
var ConnMode = "Client";
var ConnType = "Data";
var ConnPort = 5556;             //Set the application usage port
var RemoteControl = false;           //Set whether to use remote control or regular DB9 bridge

//Socket Dialing Host Info
var RemoteHost = "your.server.here";  //Set the application remote host

/*****************
Socket Listening Whitelist
Connection is accepted if the criteria is met:
  incoming_IP & <net_mask> = <ip_addr> & <net_mask>
  
Example:
  Possible IP Range = 197.158.1.1 to 197.158.255.255
  We set:
  ClientIP = "197.158.1.1",
  ClientIPMask = "255.255.0.0"
******************/
mySocket.clientIP = "0.0.0.0";      //Set the incoming client IP ddress
mySocket.clientIPMask = "0.0.0.0";  //Set the incoming client IP mask




/*****************
App Calls
*****************/

function RunApp(inApp) {
  //This function pulls in the wanted program/app and will set it to 
  //a default interval
  AppSwitch = "";              //Default "general" app state
    myNetwork.appSwitch = "";  //Default module app state
    mySocket.appSwitch = "";   //Default module app state

  CurrentApp = "";  //Make a default program state before running
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
Test Stub
You can use this area to create stubs to call the prototype functions
in the modules via RunApp(yourapp)

Ex: 
sms.setupSMS()
myNetwork.status()
*****************/
function TestStub() {
  var temp = myNetwork.status();
  if (temp !== 0) {
    //Self terminating
    StopApp();
  }
}

/*****************
Demo Programs
These are the main "Tick" Programs. They will run on an interval
going through what is basically a state machine, calling the apps
necessary to complete the needed autonomous function.
*****************/


function S2GPRSBridge() {
  /***************
  Test Serial Bridge Demo
  Opens a GPRS Bridge connection via socket dial or socket listen

  ConnMode:
  Host = Socket Listener for a remote client
  Client = Socket Dial to a remote host


  ConnType:
  Data = online/data mode (pipe)
  Command = command mode
  
  RemoteControl:
  NOTE: Only usable with ConnMode = Host
  false = Do not use remote control, standard bridge
  true = Use remote control, allowing remote usage of console
  ***************/
  var res = "";

  switch (CurrentApp) {
    case "Start":
      //Wake the modem, check for readiness
      res = T2MDMWakeUp();
      if (res === 1) {
        CurrentApp = "RegCheck";
      }
      break;

    case "RegCheck":
      //Check the network readiness
      res = myNetwork.status();
      if (res === 1) {
        CurrentApp = "ContextSetup";
      }
      break;

    /*********
    Setup Socket
    *********/

    case "ContextSetup":
      //Set the socket context up
      res = mySocket.setContext();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        CurrentApp = "SocketConfig";
      }
      break;
 
    case "SocketConfig":
      //Set the socket config up
      res = mySocket.setSocket();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        CurrentApp = "SkipEscConfig";
      }
      break;

    case "SkipEscConfig":
      //Disable the skip escape sending
      res = mySocket.setSkipEsc();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        CurrentApp = "SocketConfigExt";
      }
      break;

    case "SocketConfigExt":
      //Set the extended socket configuration up
      //We set SRING notification to only be SID number
      res = mySocket.setSocketExt();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        CurrentApp = "OpenSocket";
      }
      break;

    /*********
    Create Socket
    *********/

    case "OpenSocket":
      //Opens the socket in the specified mode, then branches to the handler
      res = mySocket.openSocket(ConnMode, ConnType, RemoteHost, ConnPort);
      if (res === 1 && ConnMode === "Client" && ConnType === "Data") {
        mdmTerminal_RS232(mdm.baudrate,115200); //Create bridge instance @ 115200
        LEDBreathe(LED3,10);
        CurrentApp = "DataClient";
      } else if (res === 1 && ConnMode === "Client" && ConnType === "Command") {
        //Open socket and exit, does nothing for now
        CurrentApp = "Finished";
      } else if (res === 1 && ConnMode === "Host" && ConnType === "Data") {
        console.log("Waiting for client connection...");
        CurrentApp = "DataHostWait";
      } else if (res === 1 && ConnMode === "Host" && ConnType === "Command") {
        //Open socket and exit, does nothing for now
        CurrentApp = "Finished";
      }
      break;

    /*********
    Client Socket Dial
    *********/

    case "DataClient":
      res = mdm.uartGetDCD();
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
      res = mdm.uartGetDCD();
      if (res === 1) {
        //Not online yet, look for an incoming connection
        res = atc.receiveAT("SRING");
        if (res !== -1 && res !== -2) {
          //incoming found, parse the connection ID
          splitList = atc.parseResponse(res).split(' ');
          ConnectionID = splitList[1];
          CurrentApp = "DataHostAccept";
        }
      } else if (res === 0) {
          //Already have a connection open, drop right into the loop
          mdmTerminal_RS232(mdm.baudrate,115200); //Create bridge instance
          LEDBreathe(LED3,10);
          CurrentApp = "DataHostLoop";
      }
      break;


    case "DataHostAccept":
     //Accept the incoming connection, create the bridge when DCD indicates OK
      res = atc.sendAT('AT#SA=' + ConnectionID,"CONNECT");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        console.log("Client connection # " + ConnectionID + " open");
        if (RemoteControl === false) {
          mdmTerminal_RS232(mdm.baudrate,115200); //Create bridge instance
          LEDBreathe(LED3,10);
          CurrentApp = "DataHostLoop";
        } else {
          mdm.serial.setConsole(); //swap the console to the remote port
          LEDBreathe(LED1,10);
          CurrentApp = "DataHostLoop";
        }
      } else {
        console.log("Accepting Client Connection: " + res);
      }
      break;

    case "DataHostLoop":
      res = mdm.uartGetDCD();
      //Loop for Online/Data mode until we break out of the connection
      //Remember that UART inactive state is logic 1.
      //DCD = 1 : No socket
      //DCD = 0 : Socket open
      if (res === 1) {
        if (RemoteControl === false) {
          mdmTerminal_Disable(); //Disable bridge instance
          StopBreathe(LED3);
        } else {
          USB.setConsole(); //Move the console back to USB
          StopBreathe(LED1);
        }
        console.log("Socket Closed.");
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
Main Area Apps
These are the "Tock" applications. They will run through their function
via a state machine, on an interval set by the main program
Returns:
0: Not Complete
1: Complete
else: Returned data
*****************/

/**************************
Setup & Configuration Apps
**************************/

function T2MDMWakeUp() {
  /***************
  Wakeup Application.
  This application wakes up the modem, and gets it ready for usage by 
  removing the local echo. Edit this flow to fit how you want to set the unit up.
  REQUIRED - ATE0 to disable echo.

  Returns:
  0: Not Complete
  1: Complete
  ***************/
  var res = "";
  var rtn = 0;

  switch (AppSwitch) {
   //This area controls the basic sequencing of the modem
   //****************************************************
    case "Start":
      if (mdm.getPWRMON() === 0){
        console.log("-->Powering up the modem");
        mdm.turnOnOff(MDM_ONOFF_T);
      } else {
        console.log("-->Modem On, checking for AT");
        AppSwitch = "ATCheck";
      }
      break;
    case "ATCheck":
      //use the sendAT function to check for a response and turn off echo
      res = atc.sendAT('ATE0',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        AppSwitch = "SetVerbose";
      }
      break;
    case "SetVerbose":
      res = atc.sendAT('AT+CMEE=2',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        AppSwitch = "Finished";
      }
      break;
   //App finished
   //****************************************************
    case "Finished":
      console.log("-->AT Found, Modem Is Ready");
      AppSwitch = "";
      rtn = 1;
      break;
    default:
      console.log(" ");
      console.log("**Running T2 Modem Wakeup App**");
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
