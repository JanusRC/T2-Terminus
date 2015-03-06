/* Copyright (c) 2015 Clayton Knight/Janus RC. See the file LICENSE for copying permission. */
/*

Module for querying the network status of the modem

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
Network Module
************

This module helps make use of the Telit network checks & registration

How to use my module:
var atc = require("ATC").ATC(Serial6);
var myNetwork = require("NETWORK").NETWORK(atc, true);

Example discrete call, an outer state machine with a set interval is best used.
myNetwork.status()
returned: **Running T2 Network Status App**
returned: 0
...
returned: -->Modem Type: HE910
returned: 0
...
returned: -->Registered to: "AT&T"
...
returned: -->Network Status Check Complete.
returned: 1

************/

//Local use, uncomment and use exports.function(); instead of require("NETWORK").function();
//Ex: var myNetwork = exports.NETWORK(atc)
//var exports = {};

//Private module variables
var ModeFlag = 0; 

/* Network Object */
function NETWORK(_atc,_verb) {
  this.atc = _atc;		//ATC Instance to use
		//.serial
		//.CTS
  this.appSwitch = "";  //SMS App case switch
  this.appStatus = 0;	//Status check on the app
  this.verbose =
    (undefined===_verb) ? true : _verb;
  this.modem = "";		//Unit's modem type
  this.firmware = "";   //Unit's firmware
  this.phone = "";		//Unit's phone number
  this.network = "";	//Unit's registered network

}

/**************************
Network Calls
These are some calls that can be made to do certain important functions
These are used in the Apps as well.

Returns:
-1: Not complete
-2: Something other than a valid response
Otherwise: Parsed response
**************************/

NETWORK.prototype.modemType = function () {
  //Checks the modem type
  var res = this.atc.sendAT('AT+CGMM',"OK");
  if (res !== -1 && res !== -2 && res !== "ERROR") {
    //Save the unit type to the object for branches elsewhere
    this.modem = res;
  }
  return res; 
};

NETWORK.prototype.getFirmware = function () {
  //Checks the modem type
  var res = this.atc.sendAT('AT+CGMR',"OK");
  if (res !== -1 && res !== -2 && res !== "ERROR") {
    //Save the unit type to the object for branches elsewhere
    this.firmware = res;
  }
  return res; 
};

NETWORK.prototype.regCheckCell = function () {
  //Checks the raw cellular registration
  return this.atc.sendAT('AT+CREG?',"OK");
};

NETWORK.prototype.regCheckData = function () {
  //Checks the raw cellular data registration
  return this.atc.sendAT('AT+CGREG?',"OK");
};

NETWORK.prototype.getNetwork = function () {
  //Checks the phone number
  var res = this.atc.sendAT('AT+COPS?',"OK");
  if (res !== -1 && res !== -2 && res !== "ERROR") {
    //Parse the P/N and save it for possible use elsewhere
    splitList = res.split(',');
    this.network = splitList[2];
	res = splitList[2];
  }
  return res; 
};

NETWORK.prototype.simCheck = function () {
  //Checks the SIM status
  return this.atc.sendAT('AT+CPIN?',"OK");
};

NETWORK.prototype.getPN = function () {
  //Checks the phone number
  var res = this.atc.sendAT('AT+CNUM',"OK");
  if (res !== -1 && res !== -2 && res !== "ERROR") {
    //Parse the P/N and save it for possible use elsewhere
    splitList = res.split(',');
    this.phone = splitList[1];
	res = splitList[1];
  }
  return res; 
};

NETWORK.prototype.sigQuality = function () {
  //Checks the signal quality (RSSI)
  return this.atc.sendAT('AT+CSQ',"OK");
};

/**************************
Network Registration and Polling Apps
These are "Tock" applications. They will run through their function
via a state machine.
Returns:
0: Not Complete
1: Complete
**************************/

NETWORK.prototype.status = function () {
  /***************
  Network status check app
  this can be called to do a blanket check for network readiness

  Returns:
  0: Not Complete
  1: Complete
  ***************/
  var res = "";
  this.appStatus = 0;

  switch (this.appSwitch) {
   //This area controls the network checking
   //****************************************************
    case "Start":
      //Checks the system registration
      res = this.modemType();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (res === "HE910") {
          if (this.verbose){console.log("-->Modem Type: " + this.modem);}
          this.appSwitch = "SIMCheck";
        } else if (res === "DE910") {
          //Skip SIM card stuff
          if (this.verbose){console.log("-->Modem Type: " + this.modem);}
          this.appSwitch = "CellRegCheck";
        } else {
          if (this.verbose){console.log("-->Modem not recognized: " + res);}
        }
      }
      break;

    case "SIMCheck":
      //Checks for SIM ready
      res = this.simCheck();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (res.indexOf("+CPIN: READY",0) !== -1) {
          if (this.verbose){console.log("-->SIM Ready");}
          this.appSwitch = "GetPN";
        } else {
          if (this.verbose){console.log("-->SIM Not Ready: " + res);}
        }
      }
      break;

    case "GetPN":
      //Gets the phone number of the SIM
      res = this.getPN();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (this.verbose){console.log("-->SIM Phone Number: " + this.phone);}
        this.appSwitch = "CellRegCheck";
      }
      break;

    case "CellRegCheck":
      //Checks the system registration
      res = this.regCheckCell();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (res.indexOf(",1",0) !== -1 || res.indexOf(",5",0) !== -1) {
          if (this.verbose){console.log("-->Modem Registered");}
          this.appSwitch = "SigQualityCheck";
        } else {
          if (this.verbose){console.log("-->Modem not registered: " + res);}
        }
      }
      break;

    case "SigQualityCheck":
      //Checks the system registration
      res = this.sigQuality();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
		if (this.verbose){console.log("-->Signal Quality: " + res);}
		//Only check for data registration if usable
		if (this.modem === "HE910") {
          this.appSwitch = "DataRegCheck";
		} else {
          this.appSwitch = "Finished";
		}
      }
      break;

    case "DataRegCheck":
      //Checks the system registration
      res = this.regCheckData();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (res.indexOf(",1",0) !== -1 || res.indexOf(",5",0) !== -1) {
          if (this.verbose){console.log("-->Modem Ready for Data");}
          this.appSwitch = "NetworkCheck";
        } else {
          if (this.verbose){console.log("-->Modem not data ready: " + res);}
        }
      }
      break;

    case "NetworkCheck":
      //Checks the network attached to
      res = this.getNetwork();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (this.verbose){console.log("-->Registered to: " + this.network);}
        this.appSwitch = "GetFirmware";
      }
      break;

    case "GetFirmware":
      //Checks the network attached to
      res = this.getFirmware();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        if (this.verbose){console.log("-->Modem Firmware: " + this.firmware);}
        this.appSwitch = "Finished";
      }
      break;

   //App finished
   //****************************************************
    case "Finished":
      if (this.verbose){console.log("-->Network Status Check Complete.");}
      this.appSwitch = "";
      this.appStatus = 1;
      break;

    default:
      if (this.verbose){console.log(" ");}
      if (this.verbose){console.log("**Running T2 Network Status App**");}
      //Make sure it always starts at the beginning
      this.appSwitch = "Start";
  }
  return this.appStatus;
};

exports.NETWORK = function (_atc,_verb) {
  return new NETWORK(_atc,_verb);
};
