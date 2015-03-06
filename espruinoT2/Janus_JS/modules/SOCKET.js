/* Copyright (c) 2015 Clayton Knight/Janus RC. See the file LICENSE for copying permission. */
/*

Module for handling of the TCP/IP & UDP socket connections

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
Socket Module
************

This module helps make use of the Telit Socket functionality

How to use my module:
var atc = require("ATC").ATC(Serial6);
var mySocket = require("SOCKET").SOCKET(atc,1,1,"myapn",true);

Example discrete call, an outer state machine with a set interval is best used.
mySocket.openSocket("Client", "Data", "clayton.conwin.com", 5556)
returned: **Running T2 Socket Open App**
returned: 0
...
returned: -->Context Active, Modem IP#: "166.130.102.97"
returned: -->Opening a Data socket to: clayton.conwin.com:5556
returned: 0
...
returned: -->Socket Not Open: -1
...
returned: -->Socket Open Complete.

returned: 1

************/

//Local use, uncomment and use exports.function(); instead of require("SOCKET").function();
//Ex: var mysocket = exports.SOCKET(atc,1,1);
//var exports = {};

//Private module variables
var ModeFlag = 0;

/* Socket Object */
function SOCKET(_atc, _cid, _sid, _apn, _verb) {
  this.atc = _atc;			//ATC Instance to use
		//.serial
		//.CTS
  this.appSwitch = "";      //Socket App case switch
  this.appStatus = 0;		//Socket check on the app
  this.verbose =
   (undefined===_verb) ? true : _verb;
  this.contextID = _cid;	//Socket Context ID to use
  this.socketID = _sid;		//Socket ID to use
  this.apn = _apn;			//This CID's APN to use
  this.localIP = "";		//This CID's IP address
  this.localPort = "";		//This socket's used port
  this.connMode = "";		//This socket's mode
  this.connType = "";		//This socket's type
  this.remoteHost = "";		//Remote host name being used
  this.remotePort = "";		//Remote port being used
  this.clientIP = "";		//Set the incoming client IP address
  this.clientIPMask = "";	//Set the incoming client IP mask

  this.status =  function () {
      //Check the status of this connection
      //Example: #ST: 1,0,0
      res = _atc.sendAT('AT#ST=' + this.socketID,"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
		var splitList = res.split(',');
		if (splitList[1] === '1' && splitList[2] === '1') {
          return "TCP Client";
		} else if (splitList[1] === '1' && splitList[2] === '2') {
          return "TCP Host";
		} else if (splitList[1] === '2' && splitList[2] === '1') {
          return "UDP Client";
		} else if (splitList[1] === '2' && splitList[2] === '2') {
          return "UDP Host";
		} else {
          //Erase parameters
          this.localIP = "";
          this.localPort = "";
          this.connMode = "";
          this.connType = "";
          this.remoteHost = "";
          this.remotePort = "";
          this.clientIP = "";
          this.clientIPMask = "";
          return "No Socket";
		}
      } else {
		return res;
      }
  };
}

/**************************
Socket Calls
These are some calls that can be made to do certain important functions
These are used in the Apps as well.

Returns:
-1: Not complete
-2: Something other than a valid response
Otherwise: Parsed response
**************************/

SOCKET.prototype.setContext = function () {
  //Sets the context information for use with the socket/context
  return this.atc.sendAT('AT+CGDCONT=' + this.contextID + ',"IP","' + this.apn + '",',"OK");
};

SOCKET.prototype.setSocket = function () {
  //Sets the socket information via the SCFG command
  //SCFG=1,1,1500,0,300,1
  //     1,1,300,90,600,50
  return this.atc.sendAT('AT#SCFG=' + this.socketID + ',' + this.contextID + ',1500,0,600,1',"OK");
};

SOCKET.prototype.setSocketExt = function () {
  //Sets the socket information via the SCFGEXT command
  //AT#SCFGEXT=1,0,0,30,0,0
  //           1,2,0,30,1,0
  return this.atc.sendAT('AT#SCFGEXT=' + this.socketID + ',0,0,30,0,0',"OK");
};

SOCKET.prototype.setSkipEsc = function () {
  //Sets the skip escape sequence 
  //AT#SKIPESC=1
  return this.atc.sendAT('AT#SKIPESC=1',"OK");
};


SOCKET.prototype.getContext = function () {
  //Checks for an active context and returns the IP
  //If one is not active, it creates one and returns the IP
  res = -1;

  if (ModeFlag === 0) {
    //Check for an active context, if it's available leave it alone and move on.
    //We can use CGPADDR to retreive the current CID's IP.
    res = this.atc.sendAT('AT#CGPADDR=' + this.contextID,"OK");
    if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
      //Parse and look for a valid IP via the period, split on the comma
      splitList = res.split(',');
      this.localIP = splitList[1];
      if (this.localIP.indexOf('.',0) === -1) {
        //No IP, move to activate a context and get one
        ModeFlag = 1;
		res = -1; //force a "not ready" state
      } else {
		res = this.contextID + ":" + this.localIP; //Display the CID/IP
    }

    }
  } else {
    //Activate the context
    res = this.atc.sendAT('AT#SGACT=' + this.contextID + ',1',"SGACT:");
    if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
      //Parse the IP and save it
      splitList = res.split(' ');
      this.localIP = splitList[1];
      res = this.contextID + ":" + this.localIP; //Display the CID/IP
      ModeFlag = 0;
    } else {
      if (this.verbose){console.log("-->Context Not Active: " + res);}
    }
  }
 return res;
};

SOCKET.prototype.closeContext = function () {
  //Sets the socket information via the SCFGEXT command
  //AT#SCFGEXT=1,0,0,30,0,0
  return this.atc.sendAT('AT#SGACT=' + this.contextID + ',0',"OK");
};

SOCKET.prototype.enterCMDMode = function () {
  //Drop from data mode back to command mode
  //Raw delay isn't great in this environment but in this case
  //it's useful to delay the escape sequence entries
  res = -1;

  if (ModeFlag === 0) {
    this.atc.serial.write('+');
    delay_ms(10);
    this.atc.serial.write('+');
    delay_ms(10);
    this.atc.serial.write('+');
    delay_ms(10);
    ModeFlag = 1;
  } else {
	//Look for the OK
	//Ex: \r\n\r\nOK\r\n"
    res = this.atc.receiveAT("OK");
    if (res !== -1 && res !== -2 && res !== "ERROR") {
		ModeFlag = 0;
		this.connType = "Command";
		//Don't parse, just send OK
		res = "OK";
	}
  }
 return res;
};

SOCKET.prototype.enterDataMode = function () {
  //Re-enters online/data mode from command mode
  //this is useful if you want to drop back to command to check things
  //then re-open the data pipe.
  res = this.atc.sendAT('AT#SO=' + this.socketID,"CONNECT");
  if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
	this.connType = "Data";
  }
  return res;
};

function delay_ms(a) {
  var et=getTime()+a/1000;
  while (getTime() < et) {
 }
}

SOCKET.prototype.sendCMDMode = function (inData) {
  /***************
  Data sending function, useful when in command mode
  
  inData:
  The data to send
  ***************/
  var res = -1;

  if (ModeFlag === 0) {
      //Send the AT command to start, look for the > prompt
      res = this.atc.sendAT('AT#SSEND=' + this.socketID, ">");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        ModeFlag = 1;
		res = -1; //show that this isn't complete yet
      }
    } else {
      if (this.atc.CTS === 1) {
        //Prompt received, pass in the data to send, raw.
        this.atc.serial.write(inData);
        //Finish the send with a CTRL+Z (0x1a)
        this.atc.serial.write(0x1a);
        this.atc.CTS = 0; //Clear send flag
      } else {
        //Wait for the OK acknowledgement
        res = this.atc.receiveAT("OK");
        if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
          this.atc.CTS = 1; //Set the send flag again
          ModeFlag = 0;
          //Parse the response, return it
          return this.atc.parseResponse(res);
        }
      }
	}
  return res;
};

SOCKET.prototype.receiveCMDMode = function (inLength) {
  /***************
  Data receiving function, useful when in command mode
  
  inLength:
  The amount of data to grab from the buffer
  ***************/
  var res = -1;

  if (ModeFlag === 0) {
      //Send the AT command to start via println
      this.atc.serial.println('AT#SRECV=' + this.socketID + ',' + inLength);
      this.atc.CTS = 0; //Clear send flag
      ModeFlag = 1;
      res = -1; //show that this isn't complete yet
    } else {
      //Wait for the OK
      res = this.atc.receiveAT("OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
		//May receive +CME ERROR: activation failed if EOD
		this.atc.CTS = 1; //Set the send flag again
		ModeFlag = 0;
        splitList = res.split('\r\n');
		res = splitList[2];
      }
	}
  return res;
};

/**************************
Socket Apps
These are "Tock" applications. They will run through their function
via a state machine.
Returns:
0: Not Complete
1: Complete
**************************/

SOCKET.prototype.openSocket = function (inMode, inType, inIP, inPort) {
  /***************
  Opens a TCP connection via socket dial or socket listen
  Note: When inMode = Host, inType is DNC

  inMode:
  Host = Socket Listener for a remote client
  Client = Socket Dial to a remote host
  
  inType:
  Data = online/data mode (pipe)
  Command = command mode
  
  inIP:
  The Remote IP to use in the socket
  
  inPort:
  The Local or Remote Port to use in the socket

  Returns:
  0: Not Complete
  1: Complete
  ***************/
  var res = "";
  this.appStatus = 0;

  switch (this.appSwitch) {
   //This area controls socket opening
   //****************************************************
    case "Start":
      //Sets the system context information in case the program doesn't
      //Even if a context is already opened on the same CID, the unit OK's this
      res = this.setContext();
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        this.appSwitch = "CheckContext";
      }
      break;

    case "CheckContext":
      //Check for an active context, if it's available leave it alone and move on.
      //We can use CGPADDR to retrieve the current CID's IP.
      res = this.getContext();
      if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
        this.appSwitch = "SocketInfo";
		if (this.verbose){console.log("-->Context " + this.contextID + " Active, Modem IP#: " + this.localIP);}
      }
      break;

    case "SocketInfo":
      //Display socket connection information
      //Check for already opened sockets on command mode based things
      this.connMode = inMode;
      if (inMode === "Client") {
        if (this.verbose){console.log("-->Opening a " + inType + " socket to: " + inIP + ":" + inPort);}
        if (inType === "Data"){
          this.connType = inType;
          this.appSwitch = "DataSocket";
        } else if (inType === "Command") {
          this.connType = inType;
          this.appSwitch = "CommandSocket";
        } else {
          if (this.verbose){console.log("-->Unknown Socket Type: " + inType);}
        }
      } else if (inMode === "Host") {
        if (this.verbose){console.log("-->Opening a listener at: " + this.localIP + ":" + inPort);}
        this.appSwitch = "FireWallClear";
      }
      break;

    case "DataSocket":
      //Open the socket in online/data mode
      res = this.atc.sendAT('AT#SD=' + this.socketID + ',0,' + inPort + ',"' + inIP + '",0,0,0',"CONNECT");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
		this.remoteHost = inIP;
		this.remotePort = inPort;
        this.appSwitch = "Finished";
      } else {
        if (this.verbose){console.log("-->Socket Not Open: " + res);}
      }
      break;

    case "CommandSocket":
      //Open the socket in command mode
      res = this.atc.sendAT('AT#SD=' + this.socketID + ',0,' + inPort + ',"' + inIP + '",0,0,1',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
		this.remoteHost = inIP;
		this.remotePort = inPort;
        this.appSwitch = "Finished";
      } else {
        if (this.verbose){console.log("-->Socket Not Open: " + res);}
      }
      break;

    case "FireWallClear":
      //Clear the previous whitelist if there is one
      res = this.atc.sendAT('AT#FRWL=2',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        this.appSwitch = "FireWallSetup";
      }
      break;

    case "FireWallSetup":
      //We have to set up a whitelist or we'll see nothing incoming
      //This is to avoid accidental data charges/connections
      res = this.atc.sendAT('AT#FRWL=1,"' + this.clientIP + '","' + this.clientIPMask + '"',"OK");
      if (res !== -1 && res !== -2 && res !== "ERROR") {
        this.appSwitch = "DataListener";
      }
      break;

    case "DataListener":
      //Open the socket listen on specified port
      res = this.atc.sendAT('AT#SL=' + this.socketID + ',1,' + inPort,"OK");
      if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
		this.localPort = inPort;
        this.appSwitch = "Finished";
      } else if (res === "+CME ERROR: already listening") {
		//Already have a listener up, move on
		this.localPort = inPort;
        this.appSwitch = "Finished";
      } else {
        if (this.verbose){console.log("-->Socket Not Open: " + res);}
      }
      break;

   //App finished
   //****************************************************
    case "Finished":
      if (this.verbose){console.log("-->Socket Open Complete.");}
      this.appSwitch = "";
      this.appStatus = 1;
      break;

    default:
      if (this.verbose){console.log(" ");}
      if (this.verbose){console.log("**Running T2 Socket Open App**");}
      //Make sure it always starts at the beginning
      this.appSwitch = "Start";
  }
  return this.appStatus;
};

SOCKET.prototype.closeSocket = function () {
  /***************
  Closes an open socket, the type of close
  is determined by the object settings

  Returns:
  0: Not Complete
  1: Complete
  ***************/
  var res = "";
  this.appStatus = 0;

  switch (this.appSwitch) {
   //This area controls socket closing
   //****************************************************
    case "Start":
      //Check the type to branch to up front
      if (this.connType === "Command") {
        this.appSwitch = "CloseCMDMode";
      } else {
        //Drop to command mode first, then exit socket
        //If ther's nothing set, do this anyway just in case.
		res = this.enterCMDMode();
		if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
          this.connType = "Command";
          this.appSwitch = "CloseCMDMode";
		}
      }
      break;

    case "CloseCMDMode":
      //Close the socket
      res = this.atc.sendAT('AT#SH=' + this.socketID,"OK");
      if (res !== -1 && res !== -2 && res.indexOf('ERROR',0) === -1) {
        this.appSwitch = "Finished";
      }
      break;

   //App finished
   //****************************************************
    case "Finished":
      if (this.verbose){console.log("-->Socket Close Complete.");}
      this.appSwitch = "";
      this.appStatus = 1;
      break;

    default:
      if (this.verbose){console.log(" ");}
      if (this.verbose){console.log("**Running T2 Socket Close App**");}
      //Make sure it always starts at the beginning
      this.appSwitch = "Start";
  }
  return this.appStatus;
};

exports.SOCKET = function (_atc, _cid, _sid, _apn, _verb) {
  return new SOCKET(_atc, _cid, _sid, _apn, _verb);
};
