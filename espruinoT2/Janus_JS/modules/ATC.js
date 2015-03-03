/* Copyright (c) 2015 Clayton Knight/Janus RC. See the file LICENSE for copying permission. */
/*

Module for basic AT handling/parsing of a Hayes based modem/device.

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
ATC Module
************

This module controls a more polled ATC handling for any Hayes AT based modem/device.

How to use my module:
var atc = require("ATC").ATC(Serial6);

Example discrete calls
atc.sendAT('ATE0',"OK");
returned: -1
atc.sendAT('ATE0',"OK");
returned: "OK"

************/

//Local use, uncomment and use exports.function(); instead of require("ATC").function();
//Ex: var atc = exports.connect(Serial6)
//var exports = {};

/* ATC Object */
function ATC(_serial) {
  this.serial = _serial; //Serial port to use, Ex: serial6
  this.CTS = 1;          //CTS flag module global
}

ATC.prototype.sendAT = function (inData, inTerminator) {
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

  if (this.CTS === 1) {
    this.serial.println(inData);
    this.CTS = 0;   //Flag CTS
    return -1;
  } else {
    res = this.receiveAT(inTerminator);
    if (res !== -1 && res !== -2) {
      //Got a valid AT in the buffer, parse it
      this.CTS = 1; //Reset the send flag
      return this.parseResponse(res);
    }
    //Got something other than an AT response in the buffer, discard it.
    else {
      this.CTS = 1; //Flag CTS
      return -2;
    }
  }
};

ATC.prototype.parseResponse = function (inSTR) {
  //This function parses out the responses to readable format
  //It recognizes non-AT style responses by the \r\n and just returns them.
  //-Example-
  //Input: "\r\n+CREG: 0,1\r\n\r\nOK\r\n"
  //Output: "+CREG: 0,1"

  if (inSTR !== -1 && inSTR.length > 0) {
    rtnSTR = inSTR;

    if (inSTR.indexOf('\r\n',0) !== -1) {
      splitList = inSTR.split('\r\n');
      rtnSTR = splitList[1];
    }

  }
return rtnSTR;
};

ATC.prototype.receiveAT = function (theTerminator) {
  //This function waits for a AT command response and handles errors
  //This ignores/trashes unsolicited responses.

  /*************
  Returns:
  -1: No data in buffer
  -2: Something other than a valid AT (could be a URC)
  Otherwise: Raw AT response
  Example: "\r\n+CREG: 0,1\r\n\r\nOK\r\n"
  *************/

  ret = -1;
  var check1 = -1;
  var check2 = -1;

  if (this.serial.available() > 0) {
    //Something's in the buffer, read it to clear.
    ret = this.serial.read();
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
};

exports.ATC = function (_serial) {
  return new ATC(_serial);
};
