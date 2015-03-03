/* Copyright (c) 2015 Clayton Knight/Janus RC. See the file LICENSE for copying permission. */
/*
Raw Module for the Bosch BMA222 digital triaxial accelerometer and temp sensor IC.
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
BMA222 Raw Module
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

************/


/* Module constants*/
var C = {
  DEVICE_ADDR_LO     : 0x08, // SD pin floating/low
  DEVICE_ADDR_HI     : 0x09, // SD pin high (VCC) (T2 Default)
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
  DATA_LEN    : 0x08  // All registers use 1 Byte of data
};

/* Register addresses*/
//64 Addresses 0x00 to 0x3F
//Address 0x00 to 0x0E are read only
var R = {
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

/***********************************
BEGIN FUNCTIONS
***********************************/

/* Read 6 bytes and return 3 signed integer values */
//Incoming data is presented in two's complement format
function GetXYZ() {
  I2C2.writeTo(C.DEVICE_ADDR_LO, R.AXIS_FLAG_X);
  var bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 6);
  var x = bytes[1];
  var y = bytes[3];
  var z = bytes[5];

  x = (x>=127) ? x - 256 : x;
  y = (y>=127) ? y - 256 : y;
  z = (z>=127) ? z - 256 : z;

  console.log("X: " + x + " Y: " + y + " Z: " + z);
  return;
}

/* Get temperature in degrees C from built-in sensor */
//Incoming data is presented in two's complement format
function GetTemp() {
  var CenterTemp = 24; //24 degrees C is center

  I2C2.writeTo(C.DEVICE_ADDR_LO, R.DEVICE_TEMP);
  var bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 1);
  var ReadTemp = bytes[0];

  //convert to signed integer
  ReadTemp = (ReadTemp>=127) ? ReadTemp - 256 : ReadTemp;
  
  //Each LSB tick is 0.5°C
  ReadTemp = ReadTemp/2;
  console.log("Temp: " + (CenterTemp+ReadTemp));
  
return;
}


function FastCompensate() {
  var bytes;
  //write to addres 0x36, compensate type
  //Compensation targets are all 0g by default
  console.log("Calibrating X");
  I2C2.writeTo(C.DEVICE_ADDR_LO, [R.OFFSET_COMP_TYPE, C.COMP_FAST_X]);
  
  I2C2.writeTo(C.DEVICE_ADDR_LO, R.OFFSET_COMP_TYPE);
  bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 1);
  while (bytes[0] !== COMP_FAST_READY) {
    I2C2.writeTo(C.DEVICE_ADDR_LO, R.OFFSET_COMP_TYPE);
    bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 1);
  }
  
  console.log("Calibrating X");
  I2C2.writeTo(C.DEVICE_ADDR_LO, [R.OFFSET_COMP_TYPE, C.COMP_FAST_Y]);
  
  I2C2.writeTo(C.DEVICE_ADDR_LO, R.OFFSET_COMP_TYPE);
  bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 1);
  while (bytes[0] !== COMP_FAST_READY) {
    I2C2.writeTo(C.DEVICE_ADDR_LO, R.OFFSET_COMP_TYPE);
    bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 1);
  }

  console.log("Calibrating X");
  I2C2.writeTo(C.DEVICE_ADDR_LO, [R.OFFSET_COMP_TYPE, C.COMP_FAST_Z]);
  
  I2C2.writeTo(C.DEVICE_ADDR_LO, R.OFFSET_COMP_TYPE);
  bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 1);
  while (bytes[0] !== COMP_FAST_READY) {
    I2C2.writeTo(C.DEVICE_ADDR_LO, R.OFFSET_COMP_TYPE);
    bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 1);
  }

  return 1;
}

function ConfigSave() {
  var bytes;
  //write to addres 0x33, EEPROM Control
  //Must unlock first, then write, then check ready flag
  console.log("Saving to EEPROM");

  //Unlock EEPROM
  I2C2.writeTo(C.DEVICE_ADDR_LO, [R.EEPROM_CONTROL, C.EEPROM_UNLOCK]);

  //Initiate EEPROM image write
  I2C2.writeTo(C.DEVICE_ADDR_LO, [R.EEPROM_CONTROL, C.EEPROM_WRITE]);

  //Read the register status
  //Ready will be high when done, unlock goes back to 0
  bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 1);
  while (bytes[0] !== (C.EEPROM_READY)) {
    I2C2.writeTo(C.DEVICE_ADDR_LO, R.EEPROM_CONTROL);
    bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 1);
  }

  console.log("Save Complete");
  return 1;
}


function SoftReset() {
  //Reset all registers back to default values
  //write to addres 0x14
  I2C2.writeTo(C.DEVICE_ADDR_LO, [R.SOFT_RESET_REG, C.SOFT_RESET]);
  return;
}

function PowerMode(inMode) {
  //write to addres 0x11
  I2C2.writeTo(C.DEVICE_ADDR_LO, [R.POWER_MODE_SET, inMode]);
  return;
}


function DataType(inType) {
  //write to addres 0x13
  //Filtered = 0x00
  //Unfiltered = 0x80
  I2C2.writeTo(C.DEVICE_ADDR_LO, [R.AXIS_DATA_ACQ_TYPE, inType]);
  return;
}

function SetFilterBW(inBW) {
  //write to addres 0x10
  I2C2.writeTo(C.DEVICE_ADDR_LO, [R.BANDWIDTH_SET, inBW]);
  return;
}

function SetGRange(inRange) {
  //write to addres 0x0F
  I2C2.writeTo(C.DEVICE_ADDR_LO, [R.G_RANGE_SET, inRange]);
  return;
}

//Enable, tooled for the T2 I2C lines
function EnableBMA222() {
  var bytes;

  I2C2.setup({scl:F1,sda:F0});
  I2C2.writeTo(C.DEVICE_ADDR_LO, R.CHIP_ID);
  bytes = I2C2.readFrom(C.DEVICE_ADDR_LO, 1);
  return bytes;
}

/***********************************
END FUNCTIONS
***********************************/



