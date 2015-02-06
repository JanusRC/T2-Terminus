#!/bin/false
# This file is part of Espruino, a JavaScript interpreter for Microcontrollers
#
# Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# ----------------------------------------------------------------------------------------
# This file contains information for a specific board - the available pins, and where LEDs,
# Buttons, and other in-built peripherals are. It is used to build documentation as well
# as various source and header files for Espruino.
# ----------------------------------------------------------------------------------------

import pinutils;
info = {
 'name' : "Normal Linux Compile",
 'default_console' : "EV_USBSERIAL",
 'binary_name' : 'espruino_%v_linux',
};
chip = {
  'part' : "LINUX",
  'family' : "LINUX",
  'package' : "",
  'ram' : -1,
  'flash' : -1,
  'speed' : -1,
  'usart' : 6,
  'spi' : 3,
  'i2c' : 3,
  'adc' : 0,
  'dac' : 0,
};
# left-right, or top-bottom order
board = {
};
devices = {
  'USB' : {} # to convince code that we have a USB port (it's used for the console ion Linux)
};

board_css = """
""";

def get_pins():
  pins = pinutils.generate_pins(0,7)  
  # just fake pins D0 .. D7
  return pins
