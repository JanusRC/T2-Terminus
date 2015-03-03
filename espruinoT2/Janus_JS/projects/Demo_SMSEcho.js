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

//SMS Library, you MUST PASS the ATC instance you wish to use
var sms = require("SMS").SMS(atc, DEBUG);


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
  /*****************
  This function runs on powerup, you can put RunApp() or something
  here to run things automatically.
  *****************/

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



/*****************
App Calls
*****************/

function RunApp(inApp) {
  //This function pulls in the wanted program/app and will set it to 
  //a default interval
  AppSwitch = "";              //Default "general" app state
    myNetwork.appSwitch = "";  //Default module app state
    sms.appSwitch = "";        //Default module app state

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

function SMSEcho() {
  /***************
  Test SMS Echo Demo
  This waits for an incoming SMS, and then echoes it back out
  to the originating address.
  ***************/
  var res = "";

  switch (CurrentApp) {
    case "PowerUp":
      res = T2MDMWakeUp();
      if (res === 1) {
        CurrentApp = "SMSInit";
      }
      break;

    case "SMSInit":
      res = sms.setupSMS();
      if (res === 1) {
        CurrentApp = "RegCheck";
      }
      break;

    case "RegCheck":
      res = myNetwork.status();
      if (res === 1) {
        console.log(" ");
        console.log("Listening for incoming SMS");
        //Change the clock to a slower rate
        //Slow check frequency helps catch new messages
        ChangeAppClock(2000);
        CurrentApp = "SMSWait";
      }
      break;

    /*********
    Listen for incoming SMS
    *********/

    case "SMSWait":
      res = sms.checkForSMS();
      if (res !== 0 && res !== 1) {
        //New SMS Received
        //Change the clock back to default
        ChangeAppClock(AppInterval);
        console.log("SMS of length " + sms.info.length + " received from: " + sms.info.OriginatingPN);
        console.log(sms.info.SMS);
        console.log(" ");
        console.log("Echoing the SMS");
        CurrentApp = "SMSEcho";
      } else if (res === 1) {
        //Processing finished, no SMS
        //Check registration on the fly to ensure no issues
        CurrentApp = "RegCheckFast";
      }

      break;

    /*********
    Echo SMS
    *********/

    case "SMSEcho":
      //Echo it back
      res = sms.sendSMS(sms.info.OriginatingPN, sms.info.SMS);
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        //No finish, we just loop, quick reg. check first.
        console.log(" ");
        console.log("Listening for incoming SMS");
        CurrentApp = "RegCheckFast";
      }
      break;

    case "RegCheckFast":
      //Lightweight registration check for quick inline querying.
      res = myNetwork.regCheckCell();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (res.indexOf(",1",0) !== -1 || res.indexOf(",5",0) !== -1) {
          //Valid response, still registered
          //Change the clock to a slower rate
          //Slow check frequency helps catch new messages
          ChangeAppClock(2000);
          CurrentApp = "SMSWait";
        } else {
          //Not a valid response, full registration check
          CurrentApp = "RegCheck";
        }
      }
      break;

    default:
      console.log("Running T2 SMS Echo Program");
      //Make sure it always starts at the beginning
      CurrentApp = "PowerUp";
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
