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
 'name' : "T2 STM32F4",
 'link' :  [ "http://www.janus-rc.com" ],
 'default_console' : "EV_SERIAL3", # USART3 by default. USART6 goes to the embedded modem
 'default_console_tx' : "B10", # USART3_TX on PB10
 'default_console_rx' : "B11", # USART3_RX on PB11
 'default_console_baudrate' : "115200", # Baudrate default
 'variables' : 5450,
 'binary_name' : 'espruino_%v_T2stm32f4.bin',
 'bootloader' : 1,
};
chip = {
  'part' : "STM32F405ZGT6",
  'family' : "STM32F4",
  'package' : "LQFP144",
  'ram' : 192,
  'flash' : 1024,
  'speed' : 168,
  'usart' : 6,
  'spi' : 3,
  'i2c' : 3,
  'adc' : 3,
  'dac' : 2,
};
# left-right, or top-bottom order
board = {
};
devices = {
  'OSC' : { 'pin_1' : 'H0',
            'pin_2' : 'H1' },
  'OSC_RTC' : { 'pin_1' : 'C14',
                'pin_2' : 'C15' },
  'LED1' : { 'pin' : 'E12' },
  'LED2' : { 'pin' : 'E13' },
  'LED3' : { 'pin' : 'E14' },
  'USB' : { #'pin_otg_pwr' : 'E14',
            'pin_dm' : 'A11',
            'pin_dp' : 'A12',
            'pin_vbus' : 'A9',
            'pin_id' : 'A10', },
#SDIO/SDHC
  'SD' :  { 'pin_cmd' :  'D2',
            'pin_d0' :  'C8',
            'pin_d1' :  'C9',
            'pin_d2' :  'C10',
            'pin_d3' :  'C11',
            'pin_clk' :  'C12' },
#SD SPI
#  'SD' :  { 'pin_cs' :  'C11',
#            'pin_di' :  'D2',
#            'pin_do' :  'C8',
#            'pin_clk' :  'C12' },
};


board_css = """
""";

def get_pins():
  pins = pinutils.scan_pin_file([], 'stm32f40x.csv', 6, 9, 10)
  return pinutils.only_from_package(pinutils.fill_gaps_in_pin_list(pins), chip["package"])
