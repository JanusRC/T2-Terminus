/* Copyright (c) 2015 Clayton Knight/Janus RC. See the file LICENSE for copying permission. */
/*
Module for the Bosch BMA222 digital triaxial accelerometer and temp sensor IC.
Only I2C is supported.

Parts of the module is based on the MPU6050 driver written by Lars Toft Jacobsen:
I2Cdev device library code is placed under the MIT license
Copyright (c) 2012 Jeff Rowberg

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
BMA222 Module
************

You can wire this up as follows:
The device supports both SPI/I2C, but we wire it to force I2C only.
Janus T2 STM32F4 pre-set pinout for reference.

| Device Pin | Espruino |     Janus T2    |
| ---------- | -------- | --------------- |
| 1  (SDO)   | GND      | E1              |
| 2  (SDA)   | B7       | F0 (PU to 2.8v) |
| 3  (VDDIO) | 3.3v     | 2.8v            |
| 4  (NC)    | -        | -               |
| 5  (INT1)  | GPIO     | E4              |
| 6  (INT2)  | GPIO     | E5              |
| 7  (VDD)   | 3.3v     | 2.8v            |
| 8  (GNDIO) | GND      | GND             |
| 9  (GND)   | GND      | GND             |
| 10 (CS)    | -        | E2              |
| 11 (PS)    | 3.3v     | E3 (PU to 2.8v) |
| 12 (SCx)   | B6       | F1 (PU to 2.8v) |


T2 Alignment
X + = 30P Header side
X - = DB9 side
Y + = LED side
Y - = USB side
Z + = Case top side
Z - = Case bottom side

How to use my module:

I2C2.setup({scl:F1,sda:F0});
var bma = require("BMA222").connect(I2C2);

Error check via:
bma.connected
="Connected"
or
="Error"

Example discrete calls
bma.getAcceleration(); // returns an [x,y,z] array with raw accl. data
bma.getTemp();         // returns the temperature in C°
bma.calibrate();       // runs a fast calibration to align the axis, based on a target of 0.0G
bma.setPowerMode(BMA222_C.MODE_SUSPEND); //Puts the unit into suspension
bma.setPowerMode(BMA222_C.MODE_RUN);     //Puts the unit into full run
bma.setGRange(BMA222_C.G_RANGE_2G);      //Sets the range to +/-2G
bma.setFilterBW(BMA222_C.BW_31_25HZ);    //Sets the bandwidth update at 16ms
bma.saveConfig();      //Saves the register image to EEPROM

Continuous Poll:
Stub function to poll
function testpoll () {
  var temp = bma.getAcceleration();
  console.log(temp);
}

Run poll
var acceltest = setInterval(testpoll, 250);

Stop poll
clearInterval(acceltest);

For use of more than one chip per I2C bus, the address can be selected 
with an additional parameter to the connect call:

var bma = require("BMA222").connect(I2C2, false); // SDO pin wired to GND or floating
var bma = require("BMA222").connect(I2C2, true);  // SDO pin wired to Vcc

************/

//Local use, uncomment and use exports.function(); instead of require("BMA222").function();
//Ex: var bma = exports.connect(I2C2)
//var exports = {};

/* Module constants*/
var BMA222_C = {
  DEVICE_ID          : 0x03, // BMA222 Device ID = 3
  DEVICE_ADDR_LO     : 0x08, // SD pin floating/low (T2 Default)
  DEVICE_ADDR_HI     : 0x09, // SD pin high (VCC)
  SOFT_RESET         : 0xB6, // Written to the SR register will default everything.

  //Sleep/Low power modes
  MODE_RUN           : 0x00, // Full Run
  MODE_SUSPEND       : 0x80, // Suspend bit high
  MODE_SLEEP_1MS     : 0x46, // 1mS Sleep
  MODE_SLEEP_10MS    : 0x4A, // 10mS Sleep
  MODE_SLEEP_25MS    : 0x4B, // 25mS Sleep
  MODE_SLEEP_50MS    : 0x4C, // 50mS Sleep
  MODE_SLEEP_100MS   : 0x4D, // 100mS Sleep
  MODE_SLEEP_500MS   : 0x4E, // 500mS Sleep
  MODE_SLEEP_1S      : 0x4F, // 1S Sleep

  //Data Type
  DATA_FILTERED      : 0x00, // Filtered data, defaults to 1Khz
  DATA_UNFILTERED    : 0x80, // Unfiltered Data, BW is 2Khz

  //Filtered bandwidth
  BW_7_81HZ          : 0x08, // 64mS
  BW_15_63HZ         : 0x09, // 32mS
  BW_31_25HZ         : 0x0A, // 16mS
  BW_62_5HZ          : 0x0B, // 8mS
  BW_125HZ           : 0x0C, // 4mS
  BW_2501HZ          : 0x0D, // 2mS
  BW_5001HZ          : 0x0E, // 1mS
  BW_1000HZ          : 0x0F, // 0.5mS (Default)

  //G Range Selection
  G_RANGE_2G         : 0x03, // +/-2G (Default)
  G_RANGE_4G         : 0x05, // +/-4G
  G_RANGE_8G         : 0x08, // +/-8G
  G_RANGE_16G        : 0x0C, // +/-16G

  //Compensation
  COMP_OFFSET_RESET  : 0x80,
  //Default value of offset targets is 0g, run compensate when still.
  //Fast Compensation
  COMP_FAST_READY    : 0x10,
  COMP_FAST_X        : 0x20, // Calibrate X
  COMP_FAST_Y        : 0x40, // Calibrate Y
  COMP_FAST_Z        : 0x60, // Calibrate Z
  //Slow Compensation
  COMP_SLOW_XYZ      : 0x07, // Calibrate all axis
  COMP_SLOW_X        : 0x01, // Calibrate X only
  COMP_SLOW_Y        : 0x02, // Calibrate Y only
  COMP_SLOW_Z        : 0x04, // Calibrate Z only

  //EEPROM Control
  EEPROM_UNLOCK      : 0x01, // Must unlock before a write
  EEPROM_WRITE       : 0x02, // Write the image to the EEPROM
  EEPROM_READY       : 0x04, // 0 if not ready, 1 ready for a write
  EEPROM_LOAD        : 0x08, // Load the EEPROM contents to the reg. image

  //Misc.
  DATA_LEN           : 0x08  // All registers use 1 Byte of data
};

/* Register addresses*/
//64 Addresses 0x00 to 0x3F
//Address 0x00 to 0x0E are read only
var BMA222_R = {
  CHIP_ID             : 0x00,
  CHIP_VERSION        : 0x01,
  //Device Info and new data flags
  AXIS_FLAG_X         : 0x02,
  AXIS_DATA_X         : 0X03,
  AXIS_FLAG_Y         : 0x04,
  AXIS_DATA_Y         : 0x05,
  AXIS_FLAG_Z         : 0x06,
  AXIS_DATA_Z         : 0x07,
  DEVICE_TEMP         : 0x08,

  //Interrupt status
  INT_STATUS          : 0x09,
  NEW_DATA_STATUS     : 0x0A,
  TAP_SLOPE_STATUS    : 0x0B,
  FLAG_ORIENT_STATUS  : 0x0C,

  //BMA222 Configuration Registers
  G_RANGE_SET         : 0x0F,
  BANDWIDTH_SET       : 0x10,
  POWER_MODE_SET      : 0x11,
  LOW_NOISE_CNTRL     : 0x12,
  AXIS_DATA_ACQ_TYPE  : 0x13,
  SOFT_RESET_REG      : 0x14,

  //Interrupts
  INT_ENABLE_LO       : 0x16,
  INT_ENABLE_HI       : 0x17,
  INT_PIN_MAP_LO      : 0x19, //Interrupt pin 1
  INT_PIN_MAP_MID     : 0x1A, //Interrupt pin 1&2
  INT_PIN_MAP_HI      : 0x1B, //Interrupt pin 2
  INT_SRC_DEF         : 0x1E,
  INT_BEHAVE_DEF      : 0x20,
  INT_RESET           : 0x21,
  INT_LOW_G_DELAY     : 0x22,
  INT_LOW_G_THRESH    : 0x23,
  INT_LOW_G_MODE      : 0x24,
  INT_HI_G_DELAY      : 0x25,
  INT_HI_G_THRESH     : 0x26,
  INT_HI_G_MODE       : 0x27,
  INT_SLOPE_THRESH    : 0x28,
  INT_TAP_TIMING      : 0x2A,

  //Sample number and angle settings
  SAMPLENUM_SET       : 0x2B,
  HYSTERESIS_SET      : 0x2C,
  THETA_BLOCK_ANG     : 0x2D,
  FLAT_THRESH_ANG     : 0x2E,
  FLAT_HOLD_TIME      : 0x2F,

  //Other
  SELF_TEST_ACT       : 0x32,
  EEPROM_CONTROL      : 0x33,
  INTERFACE_WDT       : 0x34,
  OFFSET_COMP_TYPE    : 0x36,
  OFFSET_COMP_TARGET  : 0x37,
  F_OFFSET_COMP_Y     : 0x38,
  F_OFFSET_COMP_x     : 0x39,
  F_OFFSET_COMP_Z     : 0x3A,
  U_OFFSET_COMP_Y     : 0x3B,
  U_OFFSET_COMP_x     : 0x3C,
  U_OFFSET_COMP_Z     : 0x3D
};

/* BMA222 Object */
function BMA222(_i2c, _addr) {
  this.i2c = _i2c;
  this.addr =
    (undefined===_addr || false===_addr) ? BMA222_C.DEVICE_ADDR_LO :
    (true===_addr) ? BMA222_C.DEVICE_ADDR_HI :
    _addr;
  //var temp = this.initialize();
  if (this.initialize() === 1) {
    //Little bit of error handling here
    this.connected = "Connected";
  } else {
    this.connected = "Error";
  }
}

/* Initialize the chip */
BMA222.prototype.initialize = function() {
  var bytes;

  this.i2c.writeTo(this.addr, BMA222_R.CHIP_ID);
  bytes = this.i2c.readFrom(this.addr, 1);

  //Device ID is correct, set some basic initializations
  if (bytes[0] === BMA222_C.DEVICE_ID) {
    this.setPowerMode(BMA222_C.MODE_RUN);
    this.setDataType(BMA222_C.DATA_FILTERED);
    this.setFilterBW(BMA222_C.BW_31_25HZ);
    this.setGRange(BMA222_C.G_RANGE_2G);
    return 1;
  } else {
    return 0;
  }
};


BMA222.prototype.setPowerMode = function (inMode) {
  //Set the power mode
  this.i2c.writeTo(this.addr, [BMA222_R.POWER_MODE_SET, inMode]);
};


BMA222.prototype.setDataType = function (inType) {
  //Filtered = 0x00
  //Unfiltered = 0x80
  this.i2c.writeTo(this.addr, [BMA222_R.AXIS_DATA_ACQ_TYPE, inType]);
};


BMA222.prototype.setFilterBW = function (inBW) {
  //Set the bandwidth of the updates
  this.i2c.writeTo(this.addr, [BMA222_R.BANDWIDTH_SET, inBW]);
};


BMA222.prototype.setGRange = function (inRange) {
  //Set +/- G range
  this.i2c.writeTo(this.addr, [BMA222_R.G_RANGE_SET, inRange]);
};

BMA222.prototype.softReset = function () {
  //Reset all registers back to default values
  I2C2.writeTo(this.addr, [BMA222_R.SOFT_RESET_REG, BMA222_C.SOFT_RESET]);
};



/* Read 6 bytes and return 3 signed integer values */
//Incoming data is presented in two's complement format
//Acceleration data starts at the X axis new data flag register
BMA222.prototype.getXYZ = function () {
  this.i2c.writeTo(this.addr, BMA222_R.AXIS_FLAG_X);
  var bytes = this.i2c.readFrom(this.addr, 6);
  var x = bytes[1];
  var y = bytes[3];
  var z = bytes[5];

  x = (x>=127) ? x - 256 : x;
  y = (y>=127) ? y - 256 : y;
  z = (z>=127) ? z - 256 : z;

  return [x, y ,z];
};

/* Get raw acceleration */
BMA222.prototype.getAcceleration = function() {
  return this.getXYZ();
};


/* Get temperature in degrees C from built-in sensor */
//Incoming data is presented in two's complement format
BMA222.prototype.getTemp = function () {
  var CenterTemp = 24; //24 degrees C is center

  this.i2c.writeTo(this.addr, BMA222_R.DEVICE_TEMP);
  var bytes = I2C2.readFrom(this.addr, 1);
  var ReadTemp = bytes[0];

  //convert to signed integer
  ReadTemp = (ReadTemp>=127) ? ReadTemp - 256 : ReadTemp;

  //Each LSB tick is 0.5°C
  ReadTemp = ReadTemp/2;

  return CenterTemp+ReadTemp;
};


BMA222.prototype.calibrate = function () {
  var bytes;

  //Compensation targets are all 0g by default
  //Compensate when completely still
  this.i2c.writeTo(this.addr, [BMA222_R.OFFSET_COMP_TYPE, BMA222_C.COMP_FAST_X]);

  this.i2c.writeTo(this.addr, BMA222_R.OFFSET_COMP_TYPE);
  bytes = this.i2c.readFrom(this.addr, 1);
  while (bytes[0] !== BMA222_C.COMP_FAST_READY) {
    this.i2c.writeTo(this.addr, BMA222_R.OFFSET_COMP_TYPE);
    bytes = this.i2c.readFrom(this.addr, 1);
  }

  this.i2c.writeTo(this.addr, [BMA222_R.OFFSET_COMP_TYPE, BMA222_C.COMP_FAST_Y]);

  this.i2c.writeTo(this.addr, BMA222_R.OFFSET_COMP_TYPE);
  bytes = this.i2c.readFrom(this.addr, 1);
  while (bytes[0] !== BMA222_C.COMP_FAST_READY) {
    this.i2c.writeTo(this.addr, BMA222_R.OFFSET_COMP_TYPE);
    bytes = this.i2c.readFrom(this.addr, 1);
  }

  this.i2c.writeTo(this.addr, [BMA222_R.OFFSET_COMP_TYPE, BMA222_C.COMP_FAST_Z]);

  this.i2c.writeTo(this.addr, BMA222_R.OFFSET_COMP_TYPE);
  bytes = this.i2c.readFrom(this.addr, 1);
  while (bytes[0] !== BMA222_C.COMP_FAST_READY) {
    this.i2c.writeTo(this.addr, BMA222_R.OFFSET_COMP_TYPE);
    bytes = this.i2c.readFrom(this.addr, 1);
  }

  return 1;
};

BMA222.prototype.saveConfig = function () {
  var bytes;

  //Unlock EEPROM
  this.i2c.writeTo(this.addr, [BMA222_R.EEPROM_CONTROL, BMA222_C.EEPROM_UNLOCK]);

  //Initiate EEPROM image write
  this.i2c.writeTo(this.addr, [BMA222_R.EEPROM_CONTROL, BMA222_C.EEPROM_WRITE]);

  //Read the register status
  //Ready will be high when done, unlock goes back to 0
  bytes = this.i2c.readFrom(this.addr, 1);
  while (bytes[0] !== (BMA222_C.EEPROM_READY)) {
    this.i2c.writeTo(this.addr, BMA222_R.EEPROM_CONTROL);
    bytes = this.i2c.readFrom(this.addr, 1);
  }

  return 1;
};

exports.connect = function (_i2c,_addr) {
  return new BMA222(_i2c,_addr);
};
