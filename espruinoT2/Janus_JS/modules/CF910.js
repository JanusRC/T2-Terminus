/* Copyright (c) 2015 Clayton Knight/Janus RC. See the file LICENSE for copying permission. */
/*

Module for handling of the HSPA based CF unit

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
CF910 Module
************

This module controls the embedded modem of the T2. 
-HSPA910CF
-EVDO910CF
-CDMA910CF

How to use my module:
1. Uncomment the pin definitions and fill them out
2. var mdm = require("CF910").CF910(Serial6,115200,1);

Example discrete calls
mdm.turnOnOff(5000);

************/

//Local use, uncomment and use exports.function(); instead of require("CF910").function();
//Ex: var mdm = exports.CF910(Serial6,115200,1);
//Ex2: var mdm = exports.CF910(Serial6);
//var exports = {};

/* Pin Definitiions */
//Uncomment and fill this out
/*
var CF910_IO = {
  //Control
  ENABLE     : C1,
  ON_OFF     : C2,
  RESET      : C3,
  SERVICE    : C4,
  VBUS_EN    : C5,
  PWRMON     : G0,
  STAT_LED   : G1,
  GPS_RESET  : D10,

  //UART
  RXD        : C7,
  TXD        : C6,
  DSR        : D3,
  RING       : D7,
  DCD        : D8,
  DTR        : D9,
  CTS        : G15,
  RTS        : G12,

  //GPIO
  GPIO1      : G6,
  GPIO2      : G5,
  GPIO3      : G8,
  GPIO4      : G7,
  GPIO5      : G2,
  GPIO6      : G3,
  GPIO7      : G4
};
*/
/* Global Variables */
var StatLEDTick;  //Contains the LED update interrupt ID

/* Modem Object */
function CF910(_serial,_atbaud,_statled) {
  //Takes in the serial port to use, the baud rate, and the status LED flag
  //The baud rate and LED flag can be left out and will default to a normal usage state
  this.serial = _serial; 
  this.baudrate =
    (undefined===_atbaud) ? 115200 : _atbaud;
  this.statusLED =
    (undefined===_statled) ? 1 : _statled;

  //Initialize the modem
  this.init();

  this.status = function () {
      var tmp = this.getPWRMON();
      if (tmp === 1) {
        return "On";
      } else {
        return "Off";
      }
  };
}

CF910.prototype.init = function() {
  this.setIO();        //Set up the required I/O
  this.readyIO();      //Ready the I/O for use
  this.serialInit();   //Initialize the serial interface
  this.disableHWFC();  //Disable hardware flow control

  if (this.statusLED === 1) {
    this.statLED_ON();
  } else {
    this.statLED_OFF();
  }
};


/*************************************************
Modem I/O Control
*************************************************/

CF910.prototype.setIO = function () {
  //This funciton sets the modem I/O to a safe/usable state
  //Modem Control I/O, all open drain config
  pinMode(CF910_IO.ENABLE,'opendrain');    //Modem Enable
  pinMode(CF910_IO.ON_OFF,'opendrain');    //Modem ON/OFF
  pinMode(CF910_IO.RESET,'opendrain');     //Modem Reset
  pinMode(CF910_IO.SERVICE,'opendrain');   //Modem Service
  pinMode(CF910_IO.VBUS_EN,'opendrain');   //Modem Enable VBUS
  pinMode(CF910_IO.GPS_RESET,'opendrain'); //Modem GPS Reset

  //UART
  pinMode(CF910_IO.CTS,'input');           //Modem CTS
  pinMode(CF910_IO.DSR,'input');           //Modem DSR
  pinMode(CF910_IO.DCD,'input');           //Modem DCD
  pinMode(CF910_IO.RING,'input');          //Modem RING
  pinMode(CF910_IO.RTS,'output');          //Modem RTS
  pinMode(CF910_IO.DTR,'output');          //Modem DTR

  //Modem Feedback I/O, set to floating inputs
  pinMode(CF910_IO.PWRMON,'input');        //Modem PWRMON
  pinMode(CF910_IO.STAT_LED,'input');      //Modem CELL LED

  //GPIO, set to open drain as a safe state
  pinMode(CF910_IO.GPIO1,'opendrain');     //Modem GPIO1
  pinMode(CF910_IO.GPIO2,'opendrain');     //Modem GPIO2
  pinMode(CF910_IO.GPIO3,'opendrain');     //Modem GPIO3
  pinMode(CF910_IO.GPIO4,'opendrain');     //Modem GPIO4
  pinMode(CF910_IO.GPIO5,'opendrain');     //Modem GPIO5
  pinMode(CF910_IO.GPIO6,'opendrain');     //Modem GPIO6
  pinMode(CF910_IO.GPIO7,'opendrain');     //Modem GPIO7
};

CF910.prototype.readyIO = function () {
  //This function sets the modem control I/O to the "ready" state
  digitalWrite(CF910_IO.ENABLE,1);         //Enable released
  digitalWrite(CF910_IO.RESET,1);          //Reset released
  digitalWrite(CF910_IO.ON_OFF,1);         //ON/OFF released
  digitalWrite(CF910_IO.VBUS_EN,1);        //VBUS Enable released
};

function statusLED () {
  //Pass the cellular status LED through to LED2
  var Value = digitalRead(CF910_IO.STAT_LED);
  digitalWrite(LED2,Value);
}

CF910.prototype.statLED_ON = function () {
  //Allow the status LED to update LED2 continuously
  StatLEDTick = setInterval(statusLED, 100);
};

CF910.prototype.statLED_OFF = function () {
  //Disable the stat LED, turn it off
  //Only run the interval clear if it's defined

  if (undefined!==StatLEDTick) {
    clearInterval(StatLEDTick);
  }

  digitalWrite(LED2,0);
};

CF910.prototype.getPWRMON = function () {
  //Returns the PWRMON status
  return digitalRead(CF910_IO.PWRMON);
};

CF910.prototype.turnOnOff = function (timeIn) {
  //Takes in a time argument to dynamically set for different terminus modules
  //asserts and releases the ON/OFF I/O
  digitalWrite(CF910_IO.ON_OFF,0); //Assert ON/OFF
  setTimeout("digitalWrite(CF910_IO.ON_OFF,1);", timeIn); //Release ON/OFF
};

CF910.prototype.reset = function () {
  digitalWrite(CF910_IO.RESET,0); //Assert Reset
  setTimeout("digitalWrite(CF910_IO.RESET,1);", 500); //Release Reset
};

CF910.prototype.disable = function () {
  //This disables the terminus power supply
  return digitalWrite(CF910_IO.ENABLE,0); //Enable asserted
};

CF910.prototype.enable = function () {
  //This funciton enables the terminus power supply
  return digitalWrite(CF910_IO.ENABLE,1); //Enable released
};

/*************************************************
Serial communications control

*************************************************/

/*****************
Discrete RS232
*****************/
CF910.prototype.uartSetDTR = function () {
  return digitalWrite(CF910_IO.DTR,1);
};

CF910.prototype.uartClearDTR = function () {
  return digitalWrite(CF910_IO.DTR,0);
};

CF910.prototype.uartSetRTS = function () {
  return digitalWrite(CF910_IO.RTS,1);
};

CF910.prototype.uartClearRTS = function () {
  return digitalWrite(CF910_IO.RTS,0);
};

CF910.prototype.uartGetCTS = function () {
  return digitalRead(CF910_IO.CTS);
};

CF910.prototype.uartGetDTR = function () {
  return digitalRead(CF910_IO.DTR);
};

CF910.prototype.uartGetDSR = function () {
  return digitalRead(CF910_IO.DSR);
};

CF910.prototype.uartGetRING = function () {
  return digitalRead(CF910_IO.RING);
};

CF910.prototype.uartGetDCD = function () {
  return digitalRead(CF910_IO.DCD);
};

/*****************
UART Handling
*****************/

CF910.prototype.serialInit = function () {
  //Initialize the modem serial interface
  //Choose the I/O to avoid alternate config options
  this.serial.setup(this.baudrate, {rx:CF910_RXD,tx:CF910_TXD});

};

CF910.prototype.disableHWFC = function () {
	this.uartClearRTS();
   //pinMode(CF910_IO.RTS,'output'); //RTS init
   //digitalWrite(CF910_IO.RTS,0);   //RTS = 0 to remove FC
};


exports.CF910 = function (_serial,_atbaud,_statled) {
  return new CF910(_serial,_atbaud,_statled);
};
