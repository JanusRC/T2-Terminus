#!/usr/bin/python

# This file is part of Espruino, a JavaScript interpreter for Microcontrollers
#
# Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# ----------------------------------------------------------------------------------------
# Reads board information from boards/BOARDNAME.py and uses it to generate a header file
# which describes the available peripherals on the board
# ----------------------------------------------------------------------------------------
import subprocess;
import re;
import json;
import sys;
import os;
import importlib;
import common;

scriptdir = os.path.dirname(os.path.realpath(__file__))
# added os.path.normpath to get a correct reckognition of the subsequent path
# by Ubuntu 14.04 LTS
basedir = os.path.normpath(scriptdir+"/../")
# added leading / as a consequence of use of os.path.normpath
sys.path.append(basedir+"/scripts");
sys.path.append(basedir+"/boards");

import pinutils;

# -----------------------------------------------------------------------------------------

# Now scan AF file
print "Script location "+scriptdir

if len(sys.argv)!=2:
  print "ERROR, USAGE: build_platform_config.py BOARD_NAME"
  exit(1)
boardname = sys.argv[1]
headerFilename = "gen/platform_config.h"
print "HEADER_FILENAME "+headerFilename
print "BOARD "+boardname
# import the board def
board = importlib.import_module(boardname)
pins = board.get_pins()
# -----------------------------------------------------------------------------------------

LINUX = board.chip["family"]=="LINUX"

if not "default_console" in board.info:
  board.info["default_console"] = "EV_SERIAL1"

has_bootloader = False
if "bootloader" in board.info and board.info["bootloader"]!=0:
  has_bootloader = True

if not LINUX:
  # 100xB and 103xB are mid-density, so have 1k page sizes
  if board.chip["part"][:7]=="STM32F1" and board.chip["part"][10]=="B": board.chip["subfamily"]="MD";

  # how much room for stack (and EVERYTHING else)
  space_for_stack = 4 #kB
  if board.chip["ram"] > 20: space_for_stack = 5
  variable_storage = board.chip["ram"] - space_for_stack
  # work out # of variables
  # We need to know if we should be using 8 or 16 bit addresses
  #variables_8bit = (variable_storage*1024 ) / 12
  #variables_16bit = (variable_storage*1024) / 16
  #if variables_8bit > 254 and variables_16bit > 254:
  #  variables = variables_16bit
  #else:
  #  variables = variables_8bit
  # But in some cases we may not have enough flash memory!
  variables=board.info["variables"]

       
  var_size = 12 if variables<1023 else 16
  # the 'packed bits mean anything under 1023 vars gets into 12 byte JsVars
  var_cache_size = var_size*variables
  flash_needed = var_cache_size + 4 # for magic number
  flash_page_size = 1024 # just a guess
  flash_saved_code_sector = ""
  if board.chip["family"]=="STM32F1": flash_page_size = 1024 if "subfamily" in board.chip and board.chip["subfamily"]=="MD" else 2048
  if board.chip["family"]=="STM32F2":
    flash_page_size = 128*1024
    flash_saved_code_sector = 11
  if board.chip["family"]=="STM32F3": flash_page_size = 2*1024
  if board.chip["family"]=="STM32F4":
    flash_page_size = 128*1024
    flash_saved_code_sector = 11
  # F4 has different page sizes in different places
  flash_saved_code_pages = (flash_needed+flash_page_size-1)/flash_page_size
  total_flash = board.chip["flash"]*1024

  if "saved_code" in board.chip:
    flash_saved_code_start = board.chip["saved_code"]["address"]
    flash_page_size = board.chip["saved_code"]["page_size"]
    flash_saved_code_sector = board.chip["saved_code"]["page_number"]
    flash_saved_code_pages = board.chip["saved_code"]["pages"]
    flash_available_for_code = board.chip["saved_code"]["flash_available"]*1024
  else:
    flash_saved_code_start = "(FLASH_START + FLASH_TOTAL - FLASH_SAVED_CODE_LENGTH)"
    flash_available_for_code = total_flash - (flash_saved_code_pages*flash_page_size)
    if has_bootloader: flash_available_for_code -= common.get_bootloader_size(board)

  print "Variables = "+str(variables)
  print "JsVar size = "+str(var_size)
  print "VarCache size = "+str(var_cache_size)
  print "Flash page size = "+str(flash_page_size)
  print "Flash pages = "+str(flash_saved_code_pages)
  print "Total flash = "+str(total_flash)
  print "Flash available for code = "+str(flash_available_for_code)


# -----------------------------------------------------------------------------------------
headerFile = open(headerFilename, 'w')
def codeOut(s): headerFile.write(s+"\n");
# -----------------------------------------------------------------------------------------
def die(err):
  print("ERROR: "+err)
  sys.exit(1)

def toPinDef(pin):
  for p in pins:
    if p["name"]=="P"+pin:
      return str(pins.index(p))+"/* "+pin+" */";
  die("Pin named '"+pin+"' not found");

def codeOutDevice(device):
  if device in board.devices:
    codeOut("#define "+device+"_PININDEX "+toPinDef(board.devices[device]["pin"]))
    if device=="BTN1":
      codeOut("#define "+device+"_ONSTATE "+("0" if "inverted" in board.devices[device] else "1"))
      if "pinstate" in board.devices[device]:
        codeOut("#define "+device+"_PINSTATE JSHPINSTATE_GPIO_"+board.devices[device]["pinstate"]);

def codeOutDevicePin(device, pin, definition_name):
  if device in board.devices:
    codeOut("#define "+definition_name+" "+toPinDef(board.devices[device][pin]))
# -----------------------------------------------------------------------------------------


codeOut("""
// Automatically generated header file for """+boardname+"""
// Generated by scripts/build_platform_config.py

#ifndef _PLATFORM_CONFIG_H
#define _PLATFORM_CONFIG_H

""");

codeOut("#define PC_BOARD_ID \""+boardname+"\"")
codeOut("#define PC_BOARD_CHIP \""+board.chip["part"]+"\"")
codeOut("#define PC_BOARD_CHIP_FAMILY \""+board.chip["family"]+"\"")

codeOut("")

if board.chip["family"]=="LINUX":
  board.chip["class"]="LINUX"
elif board.chip["family"]=="STM32F1":
  board.chip["class"]="STM32"
  codeOut('#include "stm32f10x.h"')
elif board.chip["family"]=="STM32F2":
  board.chip["class"]="STM32"
  codeOut('#include "stm32f2xx.h"')
  codeOut("#define STM32API2 // hint to jshardware that the API is a lot different")
elif board.chip["family"]=="STM32F3":
  board.chip["class"]="STM32"
  codeOut('#include "stm32f30x.h"')
  codeOut("#define STM32API2 // hint to jshardware that the API is a lot different")
  codeOut("#define USB_INT_DEFAULT") # hack
elif board.chip["family"]=="STM32F4":
  board.chip["class"]="STM32"
  codeOut('#include "stm32f4xx.h"')
  codeOut("#define STM32API2 // hint to jshardware that the API is a lot different")
elif board.chip["family"]=="LPC1768":
  board.chip["class"]="MBED"
elif board.chip["family"]=="AVR":
  board.chip["class"]="AVR"
else:
  die('Unknown chip family '+board.chip["family"])

if board.chip["class"]=="MBED":
  codeOut("""
  #pragma diag_suppress 1295 // deprecated decl
  #pragma diag_suppress 188 // enumerated type mixed with another type
  #pragma diag_suppress 111 // statement is unreachable
  #pragma diag_suppress 68 // integer conversion resulted in a change of sign
  """);

codeOut("""

// SYSTICK is the counter that counts up and that we use as the real-time clock
// The smaller this is, the longer we spend in interrupts, but also the more we can sleep!
#define SYSTICK_RANGE 0x1000000 // the Maximum (it is a 24 bit counter) - on Olimexino this is about 0.6 sec
#define SYSTICKS_BEFORE_USB_DISCONNECT 2

#define DEFAULT_BUSY_PIN_INDICATOR (Pin)-1 // no indicator
#define DEFAULT_SLEEP_PIN_INDICATOR (Pin)-1 // no indicator

// When to send the message that the IO buffer is getting full
#define IOBUFFER_XOFF ((TXBUFFERMASK)*6/8)
// When to send the message that we can start receiving again
#define IOBUFFER_XON ((TXBUFFERMASK)*3/8)

""");

if board.chip["class"]=="STM32":
  if (board.chip["part"][:9]=="STM32F401") | (board.chip["part"][:9]=="STM32F411"):
# FIXME - need to remove TIM5 from jspininfo
   codeOut("""
// Used by various pins, but always with other options
#define UTIL_TIMER TIM5
#define UTIL_TIMER_IRQn TIM5_IRQn
#define UTIL_TIMER_IRQHandler TIM5_IRQHandler
#define UTIL_TIMER_APB1 RCC_APB1Periph_TIM5
""")
  elif "subfamily" in board.chip and board.chip["subfamily"]=="MD":

   codeOut("""
// frustratingly the 103_MD (non-VL) chips in Olimexino don't have any timers other than 1-4
#define UTIL_TIMER TIM4
#define UTIL_TIMER_IRQn TIM4_IRQn
#define UTIL_TIMER_IRQHandler TIM4_IRQHandler
#define UTIL_TIMER_APB1 RCC_APB1Periph_TIM4
""")
  else:
   codeOut("""
// nice timer not used by anything else
#define UTIL_TIMER TIM7
#define UTIL_TIMER_IRQn TIM7_IRQn
#define UTIL_TIMER_IRQHandler TIM7_IRQHandler
#define UTIL_TIMER_APB1 RCC_APB1Periph_TIM7
""")

codeOut("");
# ------------------------------------------------------------------------------------- Chip Specifics
codeOut("#define RAM_TOTAL ("+str(board.chip['ram'])+"*1024)")
codeOut("#define FLASH_TOTAL ("+str(board.chip['flash'])+"*1024)")
codeOut("");
if LINUX:
  codeOut('#define RESIZABLE_JSVARS // Allocate variables in blocks using malloc')
else:
  codeOut("#define JSVAR_CACHE_SIZE                "+str(variables)+" // Number of JavaScript variables in RAM")
  codeOut("#define FLASH_AVAILABLE_FOR_CODE        "+str(flash_available_for_code))
  codeOut("#define FLASH_PAGE_SIZE                 "+str(flash_page_size))
  codeOut("#define FLASH_SAVED_CODE_PAGES          "+str(flash_saved_code_pages))
  codeOut("#define FLASH_START                     "+hex(0x08000000))
  if flash_saved_code_sector!="": codeOut("#define FLASH_SAVED_CODE_SECTOR                 "+str(flash_saved_code_sector))
  if has_bootloader: 
    codeOut("#define BOOTLOADER_SIZE                 "+str(common.get_bootloader_size(board)))
    codeOut("#define ESPRUINO_BINARY_ADDRESS         "+hex(common.get_espruino_binary_address(board)))
  codeOut("")
  codeOut("#define FLASH_SAVED_CODE_LENGTH (FLASH_PAGE_SIZE*FLASH_SAVED_CODE_PAGES)")
  codeOut("#define FLASH_SAVED_CODE_START "+str(flash_saved_code_start))
  codeOut("#define FLASH_MAGIC_LOCATION (FLASH_SAVED_CODE_START + FLASH_SAVED_CODE_LENGTH - 4)")
  codeOut("#define FLASH_MAGIC 0xDEADBEEF")
codeOut("");
codeOut("#define USARTS                          "+str(board.chip["usart"]))
codeOut("#define SPIS                            "+str(board.chip["spi"]))
codeOut("#define I2CS                            "+str(board.chip["i2c"]))
codeOut("#define ADCS                            "+str(board.chip["adc"]))
codeOut("#define DACS                            "+str(board.chip["dac"]))
codeOut("");
codeOut("#define DEFAULT_CONSOLE_DEVICE              "+board.info["default_console"]);
if "default_console_tx" in board.info:
  codeOut("#define DEFAULT_CONSOLE_TX_PIN "+toPinDef(board.info["default_console_tx"]))
if "default_console_rx" in board.info:
  codeOut("#define DEFAULT_CONSOLE_RX_PIN "+toPinDef(board.info["default_console_rx"]))
if "default_console_baudrate" in board.info:
  codeOut("#define DEFAULT_CONSOLE_BAUDRATE "+board.info["default_console_baudrate"])


codeOut("");
if LINUX:
  bufferSizeIO = 256
  bufferSizeTX = 256
  bufferSizeTimer = 16
else:
  bufferSizeIO = 64 if board.chip["ram"]<20 else 128
  bufferSizeTX = 32 if board.chip["ram"]<20 else 128
  bufferSizeTimer = 4 if board.chip["ram"]<20 else 16

if 'util_timer_tasks' in board.info:
  bufferSizeTimer = board.info['util_timer_tasks']

codeOut("#define IOBUFFERMASK "+str(bufferSizeIO-1)+" // (max 255) amount of items in event buffer - events take ~9 bytes each")
codeOut("#define TXBUFFERMASK "+str(bufferSizeTX-1)+" // (max 255)")
codeOut("#define UTILTIMERTASK_TASKS ("+str(bufferSizeTimer)+") // Must be power of 2 - and max 256")

codeOut("");

simpleDevices = [
 "LED1","LED2","LED3","LED4","LED5","LED6","LED7","LED8",
 "BTN1","BTN2","BTN3","BTN4"];
usedPinChecks = ["false"];
ledChecks = ["false"];
btnChecks = ["false"];
for device in simpleDevices:
  if device in board.devices:
    codeOutDevice(device)
    check = "(PIN)==" + toPinDef(board.devices[device]["pin"])
    if device[:3]=="LED": ledChecks.append(check)
    if device[:3]=="BTN": btnChecks.append(check)
#   usedPinChecks.append(check)
# Actually we don't care about marking used pins for LEDs/Buttons

if "USB" in board.devices:
  if "pin_disc" in board.devices["USB"]: codeOutDevicePin("USB", "pin_disc", "USB_DISCONNECT_PIN")

if "LCD" in board.devices:
  for i in range(0,16):
    codeOutDevicePin("LCD", "pin_d"+str(i), "LCD_FSMC_D"+str(i))
  codeOutDevicePin("LCD", "pin_rd", "LCD_FSMC_RD")
  codeOutDevicePin("LCD", "pin_wr", "LCD_FSMC_WR")
  codeOutDevicePin("LCD", "pin_cs", "LCD_FSMC_CS")
  if "pin_rs" in board.devices["LCD"]:
    codeOutDevicePin("LCD", "pin_rs", "LCD_FSMC_RS")

if "SD" in board.devices:
  if not "pin_d3" in board.devices["SD"]: # NOT SDIO - normal SD
    if "pin_cs" in board.devices["SD"]: codeOutDevicePin("SD", "pin_cs", "SD_CS_PIN")
    if "pin_di" in board.devices["SD"]: codeOutDevicePin("SD", "pin_di", "SD_DI_PIN")
    if "pin_do" in board.devices["SD"]: codeOutDevicePin("SD", "pin_do", "SD_DO_PIN")
    if "pin_clk" in board.devices["SD"]:
      codeOutDevicePin("SD", "pin_clk", "SD_CLK_PIN")
      sdClkPin = pinutils.findpin(pins, "P"+board.devices["SD"]["pin_clk"], False)
      spiNum = 0
      for func in sdClkPin["functions"]:
        if func[:3]=="SPI": spiNum = int(func[3])
      if spiNum==0: die("No SPI peripheral found for SD card's CLK pin")
      codeOut("#define SD_SPI EV_SPI"+str(spiNum))


for device in ["USB","SD","LCD","JTAG"]:
  if device in board.devices:
    for entry in board.devices[device]:
      if entry[:3]=="pin": usedPinChecks.append("(PIN)==" + toPinDef(board.devices[device][entry])+"/* "+device+" */")

# Specific hacks for nucleo boards
if "NUCLEO_A" in board.devices:
  for n,pin in enumerate(board.devices["NUCLEO_A"]):
      codeOut("#define NUCLEO_A"+str(n)+" "+toPinDef(pin))
if "NUCLEO_D" in board.devices:
  for n,pin in enumerate(board.devices["NUCLEO_D"]):
      codeOut("#define NUCLEO_D"+str(n)+" "+toPinDef(pin))

codeOut("")

codeOut("// definition to avoid compilation when Pin/platform config is not defined")
codeOut("#define IS_PIN_USED_INTERNALLY(PIN) (("+")||(".join(usedPinChecks)+"))")
codeOut("#define IS_PIN_A_LED(PIN) (("+")||(".join(ledChecks)+"))")
codeOut("#define IS_PIN_A_BUTTON(PIN) (("+")||(".join(btnChecks)+"))")


codeOut("""
#endif // _PLATFORM_CONFIG_H
""");
