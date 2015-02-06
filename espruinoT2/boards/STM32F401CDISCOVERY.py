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
  'name' : "STM32F401C Discovery",
  'link' :  [ "http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/LN1848/PF259098" ],
  'default_console' : "EV_SERIAL2",
  'variables' : 3040,
  'binary_name' : 'espruino_%v_stm32f401cdiscovery.bin',
};
chip = {
  'part' : "STM32F401VCT6",
  'family' : "STM32F4",
  'package' : "LQFP100",
  'ram' : 64,
  'flash' : 256,
  'speed' : 84,
  'usart' : 6,
  'spi' : 3,
  'i2c' : 3,
  'adc' : 1,
  'dac' : 0,
  'saved_code' : {
    'address' : 0x08004000,
    'page_size' : 16384, # size of pages
    'page_number' : 1, # number of page we start at (0 based)
    'pages' : 3, # number of pages we're using
    'flash_available' : 256 # binary will have a hole in it, so we just want to test against full size
  },
  'place_text_section' : 0x00010000, # note flash_available above
};
# left-right, or top-bottom order
board = {
  'left' : [ 'GND', 'VDD', 'GND', 'C1','C3','A1','A3','A5','A7','C5','B1','GND','E7','E9','E11','E13','E15','NC','B13','B15','D9','D11','D13','D15','GND'],
  'left2' : [ 'GND', 'VDD', 'NRST', 'C0','C2','A0','A2','A4','A6','C4','B0','B2','E8','E10','E12','E14','B10','B12','B14','D8','D10','D12','D14','NC','GND'],
  'right2' : [ 'GND', '5V', '3V3', 'H0', 'C14','E6','E4','E2','E0','B8','BOOT0','B6','B4','D7','D5','D3','D1','C12','C10','A14','A10','A8','C8','C6','GND'],
  'right' : [ 'GND', '5V', '3V3', 'H1', 'C15','C13','E5','E3','E1','B9','VDD','B7','B5','B3','D6','D4','D2','D0','C11','A15','A13','A9','C9','C7','GND'],
};
devices = {
  'OSC' : { 'pin_1' : 'H0',
            'pin_2' : 'H1' },
  'OSC_RTC' : { 'pin_1' : 'C14',
                'pin_2' : 'C15' },
  'LED1' : { 'pin' : 'D13' },
  'LED2' : { 'pin' : 'D12' },
  'LED3' : { 'pin' : 'D14' },
  'LED4' : { 'pin' : 'D15' },
  'BTN1' : { 'pin' : 'A0' },
  'USB' : { 'pin_otg_pwr' : 'C0',
            'pin_dm' : 'A11',
            'pin_dp' : 'A12',
            'pin_vbus' : 'A9',
            'pin_id' : 'A10', },
  'MEMS' :  {  'device' : 'LSM303DLHC',
             'pin_scl' : 'B6',
             'pin_sda' : 'B9',
             'pin_drdy' : 'E2',
             'pin_int1' : 'E5',
             'pin_int2' : 'E4' },
  'GYRO' : { 'device' : 'L3GD20',
             'pin_cs' : 'E3',
             'pin_sck' : 'A5',
             'pin_miso' : 'A6',
             'pin_mosi' : 'A7',
             'pin_int1' : 'E0',
             'pin_int2' : 'E1',
           },
  'MIC' :  { 'device' : 'MP45DT02',
             'pin_clk' :  'C3',
             'pin_dout' :  'B10', },
  'AUDIO' :  { 'device' : 'CS43L22',
               'pin_sda' :  'B9',
               'pin_scl' :  'B6',
               'pin_mclk' :  'C7',
               'pin_sclk' :  'C10',    
               'pin_sdin' :  'C12',  
               'pin_lrck' :  'A4',
               'pin_nrst' :  'D4',    
                },
};


board_css = """
#board {
  width: 680px;
  height: 1020px;
  left: 200px;
  background-image: url(img/STM32F4DISCOVERY.jpg);
}
#boardcontainer {
  height: 1020px;
}
#left {
  top: 375px;
  right: 590px;
}
#left2 {
  top: 375px;
  left: 105px;
}

#right  {
  top: 375px;
  left: 550px;
}
#right2  {
  top: 375px;
  right: 145px;
}
""";

def get_pins():
  pins = pinutils.scan_pin_file([], 'stm32f401.csv', 5, 8, 9)
  pins = pinutils.scan_pin_af_file(pins, 'stm32f401_af.csv', 0, 1)
  return pinutils.only_from_package(pinutils.fill_gaps_in_pin_list(pins), chip["package"])


