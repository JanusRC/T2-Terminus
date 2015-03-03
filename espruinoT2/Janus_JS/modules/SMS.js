/* Copyright (c) 2015 Clayton Knight/Janus RC. See the file LICENSE for copying permission. */
/*

Module for SMS handling of the Telit modem.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
*/

/************
SMS Module
************

This module helps make use of the Telit SMS functionality

How to use my module:
var atc = require("ATC").ATC(Serial6);
var sms = require("SMS").SMS(atc);

Example discrete call, an outer state machine with a set interval is best used.
sms.setupSMS();
returned: **Running SMS Setup App**
returned: 0

sms.setupSMS();
returned: 0

...

sms.setupSMS();
returned: 
-->SMS Setup Complete.
returned: 1

************/

//Local use, uncomment and use exports.function(); instead of require("SMS").function();
//Ex: var sms = exports.SMS(atc)
//var exports = {};

//Private module variables
var ModeFlag = 0;
var SMSraw = "";        //Special container for the SMS parsing
var SMSParts1 = "";     //Special container for the SMS Parsing
var SMSParts2 = "";     //Special container for the SMS Parsing


/* SMS Object */
function SMS(_atc,_verb) {
  this.atc = _atc;		//ATC Instance to use
		//.serial
		//.CTS
  this.appSwitch = "";  //SMS App case switch
  this.appStatus = 0;	//Status check on the app
  this.verbose =
   (undefined===_verb) ? true : _verb;
}

//Set up a public SMS info container. We only want to adjust this one instead of creating anew each time we
//receive a new SMS, otherwise we risk chewing memory up on accident.
//Example:
//+CMGL: <index>,<stat>,<oa/da>,<alpha>,<scts>,<tooa/toda>,<length><CR><LF><data><CR><LF>
SMS.prototype.info = {
  index:"",          //Latest SMS Index number
  stat:"",           //SMS Status type
  OriginatingPN:"",  //Originating phone number
  alpha:"",          //Alphanumeric representation of O.PN if a known phonebook entry
  date:"",           //TP-Service Centre Date Stamp
  time:"",           //TP-Service Centre Time Stamp
  type:"",           //Type of number, 129 is national, 145 is international
  length:"",         //Length of SMS
  SMS:"",            //SMS Data
  outID:""           //Special container spot for outbound SMS ID# storage
};

/**************************
SMS Calls
These are some calls that can be made to do certain important functions
These are used in the Apps as well.

Returns:
-1: Not complete
-2: Something other than a valid response
Otherwise: Parsed response
**************************/

SMS.prototype.sendSMS = function (inPhone, inMessage) {
  /***************
  SMS Sending function
  inPhone:
  The destination phone number in 145, AKA '+' format for international usage
  Ex: +12223334444

  inMessage:
  The message to send
  ***************/
  var res = -1;

  if (ModeFlag === 0) {
      //Send the AT command to start, look for the > prompt
      res = this.atc.sendAT('AT+CMGS="' + inPhone + '",145', ">");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        ModeFlag = 1;
		res = -1; //show that this isn't complete yet
      }
    } else {
      if (this.atc.CTS === 1) {
        //Prompt received, pass in the data to send, raw.
        this.atc.serial.write(inMessage);
        //Finish the send with a CTRL+Z (0x1a)
        this.atc.serial.write(0x1a);
        this.atc.CTS = 0; //Clear send flag
      } else {
        //Wait for the OK acknowledgement
        res = this.atc.receiveAT("OK");
        if (res !== -1 && res !== -2 && res !== "ERROR") {
          this.atc.CTS = 1; //Set the send flag again
		  ModeFlag = 0;
          //Parse the response, this includes the SMS ID if we want to use it.
          this.info.outID = this.atc.parseResponse(res);
		  res = "OK"; //Just send OK
        }
      }
	}
  return res;
};


/**************************
SMS Apps
These are "Tock" applications. They will run through their function
via a state machine.
Returns:
0: Not Complete
1: Complete
else: Returned data
**************************/

SMS.prototype.setupSMS = function () {
  /***************
  SMS Setup Application
  This will set the modem's SMS properties as we need them

  Returns:
  0: Not Complete
  1: Complete
  ***************/
  var res = "";
  this.appStatus = 0;

  switch (this.appSwitch) {
   //This area controls the SMS setup
   //****************************************************
    case "Start":
      //Enable TEXT format for SMS Message
      res = this.atc.sendAT('AT+CMGF=1',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        this.appSwitch = "Indications";
      }
      break;

    case "Indications":
      //No indications, we will poll manually
      res = this.atc.sendAT('AT+CNMI=0,0,0,0,0',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        this.appSwitch = "StorageLocation";
      }
      break;

    case "StorageLocation":
      //Storage location
      res = this.atc.sendAT('AT+CPMS="SM"',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        this.appSwitch = "SMSInfoDisplay";
      }
      break;

    case "SMSInfoDisplay":
      //#Received SMS extra information display
      res = this.atc.sendAT('AT+CSDH=1',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        this.appSwitch = "Finished";
      }
      break;

   //App finished
   //****************************************************
    case "Finished":
	  if (this.verbose){console.log("-->SMS Setup Complete.");}
      this.appSwitch = "";
      this.appStatus = 1;
      break;

    default:
      if (this.verbose){console.log(" ");}
      if (this.verbose){console.log("**Running SMS Setup App**");}
      //Make sure it always starts at the beginning
      this.appSwitch = "Start";
  }
  return this.appStatus;
}

SMS.prototype.checkForSMS = function () {
  /***************
  Checks for a new SMS, if one isn't found we clear the buffer anyway to ensure a clean
  read on the following pass.

  Returns:
  0: Not Complete
  1: Complete, no new messages
  Else: Parsed SMS String
  ***************/
  var res = "";
  this.appStatus = 0;


  switch (this.appSwitch) {
   //This area controls the SMS Send
   //****************************************************
    case "Start":
      //Try to list all newly received SMS. We need to send the command this way
      //because mdmSendAT will accidentally parse needed information.
      if (this.atc.CTS === 1) {
        this.atc.serial.println('AT+CMGL="REC UNREAD"');
        this.atc.CTS = 0; //Clear send flag
      } else {
        //Wait for the OK acknowledgement
        res = this.atc.receiveAT("OK");
        if (res !== -1 && res !== -2 && res !== "ERROR") {
          this.atc.CTS = 1; //Set the send flag again
          //Look for a valid index
          if (res.indexOf('+CMGL: 1',0) !== -1) {
            //New message found
            SMSraw = res;
            this.appSwitch = "ParseSMS";
          } else {
            //No new messages, delete the SMS buffer to keep it clean
            this.appSwitch = "DeleteSMS";
          }
        }
      }
      break;

    case "ParseSMS":
      /*************
      New message found, let's parse the useful information
      First, let's split the received information so we can do what we want with it.
      Below is the normal response we are parsing:
      -Technical Notation-
      +CMGL: <index>,<stat>,<oa/da>,<alpha>,<scts>,<tooa/toda>,<length><CR><LF><data><CR><LF>

      -Plaintext Example-
      +CMGL: 1,"REC UNREAD","+12223334444","","15/02/11,12:37:17-24",145,9
      Test12345
      OK
      *************/

      //Separate by \r\n to separate the SMS data, remember there's always a front /r/n
      //in the responses, so the info is in [1], data is in [2]
      SMSParts1 =             SMSraw.split('\r\n');

      this.info.SMS =           SMSParts1[2];

      //Now split by comma to get individual data from the main chunk of information
      SMSParts2 =             SMSParts1[1].split(',');

      this.info.index =         SMSParts2[0];
      this.info.stat =          SMSParts2[1];
      this.info.OriginatingPN = SMSParts2[2];
      this.info.alpha =         SMSParts2[3];
      this.info.date =          SMSParts2[4];
      this.info.time =          SMSParts2[5];
      this.info.type =          SMSParts2[6];
      this.info.length =        SMSParts2[7];

      this.appStatus = this.info.SMS;
      this.appSwitch = "DeleteSMS";
      break;

    case "DeleteSMS":
      //Delete all messages in memory to ensure a clean read every time.
      res = this.atc.sendAT('AT+CMGD=1,4',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        this.appSwitch = "Finished";
      }
      break;

   //App finished
   //****************************************************
    case "Finished":
      if (this.verbose){console.log("-->SMS Check Complete.");}
      this.appStatus = 1;
      this.appSwitch = "";
      break;

    default:
      if (this.verbose){console.log(" ");}
      if (this.verbose){console.log("**Running Check SMS App**");}
      //Make sure it always starts at the beginning
      this.appSwitch = "Start";
  }
  return this.appStatus;
}

exports.SMS = function (_atc,_verb) {
  return new SMS(_atc,_verb);
};
