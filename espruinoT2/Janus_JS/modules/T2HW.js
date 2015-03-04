/* Copyright (c) 2015 Clayton Knight/Janus RC. See the file LICENSE for copying permission. */
/*

Hardware module for the T2 Terminus

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
Hardware Module
************

This module contains the hardware defines for the device

How to use my module:
var myHW = require("T2HW").T2HW();

Check the version with simply 
myHW.Version
returned: "1.0"

Use the individual hardware objects when needed
Ex:
CF910_IO = myHW.MODEM
var mdm = require("CF910").CF910(myHW.MODEM.SERIAL,115200,1);

I2C2.setup({scl:myHW.ACCEL.SCL,sda:myHW.ACCEL.SDA});
var bma = require("BMA222").connect(I2C2);

************/

//Local use, uncomment and use exports.function(); instead of require("ATC").function();
//Ex: var atc = exports.connect(Serial6)
//var exports = {};

/* SMS Object */
function T2HW() {
  this.Version = "1.0";		//Hardware Version
}

/*******************************
BEGIN T2 STM32F4 Pin definitions and constants
*******************************/

/************
External Peripherals 
************/

/************
//30P Header 
************/
T2HW.prototype.Header = {
//General Use        //Pin # : Default  | Alternate1 |  Alternate 2
//----------------------------------------------------------------
  PIN14      : A15,  //Pin 14: GPIO1    | SPI1_CS
  PIN16      : B3,   //Pin 16: GPIO2    | SPI1_SCK
  PIN18      : B4,   //Pin 18: GPIO3    | SPI1_MISO
  PIN20      : B5,   //Pin 20: GPIO4    | SPI1_MOSI
  PIN22      : B8,   //Pin 22: GPIO5    | I2C1_SCL
  PIN24      : B9,   //Pin 24: GPIO6    | I2C1_SDA
  PIN26      : A0,   //Pin 26: GPIO7    | UART4_TX   |  ADC123_CHO
  PIN28      : A1,   //Pin 28: GPIO8    | UART4_RX   |  ADC123_IN1
  PIN30      : A2,   //Pin 30: GPIO9    | USART2_TX  |  ADC123_IN2
  PIN29      : A3,   //Pin 29: GPIO10   | USART2_RX  |  ADC123_IN3
  PIN25      : A4,   //Pin 25: GPIO11   | ADC12_IN4  |  DAC1
  PIN23      : A5,   //Pin 23: GPIO12   | ADC12_IN5  |  DAC2
  PIN21      : A6,   //Pin 21: GPIO13   | ADC12_IN6
  PIN19      : A7,   //Pin 19: GPIO14   | ADC12_IN7

//Special Use        //Pin #:  Special Function
//----------------------------------------------------------------
  IGNITION   : C13  //Pin 3:  Input, Optically Isolated, must pull up
 
//Special Use (Not MCU connected, just documentation) 
//                    Pin #  :  Description
//----------------------------------------------------------------

                     //Pin 13:  CL1 Power Output
                     //Pin 15:  CL2 Power Output
                     //Pin 1 :  T2 Power Enable
                     //Pin 2 :  T2 12V Input
                     //Pin 5 :  RS485 A
                     //Pin 6 :  RS485 B
                     //Pin 7 :  Modem AUX RS232 RX
                     //Pin 8 :  Modem AUX RS232 TX
                     //Pin 9 :  CAN LO
                     //Pin 10:  CAN HI
};

/************
//DB9
************/
T2HW.prototype.DB9 = {
//RS232              Pin #  :   Description
//----------------------------------------------------------------
  SERIAL     : Serial3,        //Serial interface for the DB9
  TXD        : B10,  //N/A  :  USART3_TX
  RXD        : B11,  //N/A  :  USART3_RX
  CTS        : D11,  //N/A  :  USART3_CTS
  RTS        : D12,  //N/A  :  USART3_RTS
  FORCEOFF   : F4,   //N/A  :  Output, Active high enable
  FORCEON    : F3,   //N/A  :  Output, Active high enable
  INVALID    : E15   //N/A  :  Input, Active high when valid RS232 signal
};


/************
//Header Peripheral Mappings
************/
T2HW.prototype.RS485 = {
//RS485              Pin #  :   Description
//----------------------------------------------------------------
  DIR   : D4,       //N/A  :  Direction | UASRT2_RTS, pulled down
  DIN   : D5,       //N/A  :  Data out  | USART2_TX, pulled up
  RO    : D6       //N/A  :  Data in   | USART2_RX
};

T2HW.prototype.CL = {
//20mA Current Loops Pin #  :   Description
//----------------------------------------------------------------
  CL1_RESET  : D14,  //N/A  :  Output, Active low reset
  CL1_TRIP   : D13,  //N/A  :  Input, Active high overcurrent trip
  CL1_VOUT   : B0,   //N/A  :  ADC Input, ADC12_IN8
  CL2_RESET  : E0,   //N/A  :  Output, Active low reset
  CL2_TRIP   : D15,  //N/A  :  Input, Active high overcurrent trip
  CL2_VOUT   : B1    //N/A  :  ADC Input, ADC12_IN9
};

T2HW.prototype.CAN = {
//CAN               Pin #  :   Description
//----------------------------------------------------------------
  TXD   : D1,        //N/A  :  Data out | CAN1_TX
  RXD   : D0,        //N/A  :  Data in  | CAN1_RX
  EN    : E6        //N/A  :  Data in  | GPIO, pulled up
};

/************
//SD CARD
************/
//Example:
//var fs = require('fs');
//fs.readFileSync();
//----------------------------------------------------------------
//Only for reference. This is defined internally

/************
//LEDs
************/
//----------------------------------------------------------------
//Only for reference. This is defined internally
//LED1 = GREEN
//LED2 = YELLOW
//LED3 = RED


/************
Internal Peripherals 
************/

/************
//BMA222 Accelerometer
************/
//Example:
//I2C2.setup({scl:T2_ACCEL.SCL,sda:T2_ACCEL.SDA});
//var bma = require("BMA222").connect(I2C2);
//bma.getAcceleration();
//Should return: [x,y,z]
T2HW.prototype.ACCEL= {
//                     Pin # :  Description
//----------------------------------------------------------------
  SDA        : F0,    //N/A  :  I2C2 SDA/SDI, externally pulled up
  SCL        : F1,    //N/A  :  I2C2 SCL/SCK, externally pulled up
  SA0        : E1,    //N/A  :  SA0/SDO I/O
  CS         : E2,    //N/A  :  Output, CS
  PS         : E3,    //N/A  :  Input, pulled up externally, forces mode to I2C
  INT1       : E4,    //N/A  :  Input, interrupt1
  INT2       : E5     //N/A  :  Input, interrupt2
};

/************
//Janus CF Embedded Modem
************/
//Example:
//var mdm = require("CF910").CF910(MDM_SERIAL,MDM_BAUD,1);
//mdm.turnOnOff(5000);
//mdm.getPWRMON();
T2HW.prototype.MODEM = {
//                     Pin #  :  Description
//----------------------------------------------------------------
  SERIAL     : Serial6,        //Serial interface for the CF modem
  //Control
  ENABLE     : C1,     //N/A  :  Modem supply enable, O/C config
  ON_OFF     : C2,     //N/A  :  Modem ON/OFF, O/C config
  RESET      : C3,     //N/A  :  Modem Reset, O/C config
  SERVICE    : C4,     //N/A  :  Modem service pin, legacy (DNC)
  VBUS_EN    : C5,     //N/A  :  Modem VBUS Enable, O/C config, active low
  PWRMON     : G0,     //N/A  :  Modem power monitor, input, active high
  STAT_LED   : G1,     //N/A  :  Modem status LED, output, active high
  GPS_RESET  : D10,    //N/A  :  Modem GPS reset, legacy (DNC)
  //UART
  RXD        : C7,     //N/A  :  Modem UART RXD
  TXD        : C6,     //N/A  :  Modem UART TXD
  CTS        : G15,    //N/A  :  Modem UART CTS
  RTS        : G12,    //N/A  :  Modem UART RTS
  DSR        : D3,     //N/A  :  Modem UART DSR
  RING       : D7,     //N/A  :  Modem UART RING
  DCD        : D8,     //N/A  :  Modem UART DCD
  DTR        : D9,     //N/A  :  Modem UART DTR
  //GPIO
  GPIO1      : G6,     //N/A  :  Modem GPIO 1
  GPIO2      : G5,     //N/A  :  Modem GPIO 2
  GPIO3      : G8,     //N/A  :  Modem GPIO 3
  GPIO4      : G7,     //N/A  :  Modem GPIO 4
  GPIO5      : G2,     //N/A  :  Modem GPIO 5
  GPIO6      : G3,     //N/A  :  Modem GPIO 6
  GPIO7      : G4      //N/A  :  Modem GPIO 7
};

/*******************************
END Pin definitions and constants
*******************************/

exports.T2HW = function () {
  return new T2HW();
};
