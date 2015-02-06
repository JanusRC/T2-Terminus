#!/bin/bash

# This file is part of Espruino, a JavaScript interpreter for Microcontrollers
#
# Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# ----------------------------------------------------------------------------------------
# Creates a Zip file of all common Espruino builds
# ----------------------------------------------------------------------------------------

cd `dirname $0`
cd ..

VERSION=`sed -ne "s/^.*JS_VERSION.*\"\(.*\)\"/\1/p" src/jsutils.h`
DIR=`pwd`
ZIPDIR=$DIR/zipcontents
ZIPFILE=$DIR/archives/espruino_${VERSION}.zip
rm -rf $ZIPDIR
mkdir $ZIPDIR

echo ------------------------------------------------------
echo                          Building Version $VERSION
echo ------------------------------------------------------

for BOARDNAME in ESPRUINO_1V3 ESPRUINO_1V3_WIZ ESPRUINO_1V3_ESP ESPRUINO_1V1 NUCLEOF401RE STM32VLDISCOVERY STM32F3DISCOVERY STM32F4DISCOVERY OLIMEXINO_STM32 HYSTM32_24 HYSTM32_28 HYSTM32_32 RASPBERRYPI
do
  echo ------------------------------
  echo                  $BOARDNAME
  echo ------------------------------
  EXTRADEFS=
  EXTRANAME=
  if [ "$BOARDNAME" == "ESPRUINO_1V3_WIZ" ]; then
    BOARDNAME=ESPRUINO_1V3
    EXTRADEFS=WIZNET=1
    EXTRANAME=_wiznet
  fi
  if [ "$BOARDNAME" == "ESPRUINO_1V3_ESP" ]; then
    BOARDNAME=ESPRUINO_1V3
    EXTRADEFS=ESP8266=1
    EXTRANAME=_esp8266
  fi
  BOARDNAMEX=$BOARDNAME
  if [ "$BOARDNAME" == "ESPRUINO_1V3" ]; then
    BOARDNAMEX=ESPRUINOBOARD
  fi
  if [ "$BOARDNAME" == "ESPRUINO_1V1" ]; then
    BOARDNAMEX=ESPRUINOBOARD_R1_1
  fi
  # actually build
  BINARY_NAME=`python scripts/get_board_info.py $BOARDNAMEX "common.get_board_binary_name(board)"`
  rm $BINARY_NAME
  if [ "$BOARDNAME" == "ESPRUINO_1V3" ]; then      
    bash -c "$EXTRADEFS scripts/create_espruino_image_1v3.sh" || { echo "Build of $BOARDNAME failed" ; exit 1; }
  elif [ "$BOARDNAME" == "ESPRUINO_1V1" ]; then      
    bash -c "$EXTRADEFS scripts/create_espruino_image_1v1.sh" || { echo "Build of $BOARDNAME failed" ; exit 1; }
  else 
    bash -c "$EXTRADEFS RELEASE=1 $BOARDNAME=1 make clean"
    bash -c "$EXTRADEFS RELEASE=1 $BOARDNAME=1 make" || { echo "Build of $BOARDNAME failed" ; exit 1; }
  fi
  # rename binary if needed
  if [ -n "$EXTRANAME" ]; then 
    NEW_BINARY_NAME=`basename $BINARY_NAME .bin`$EXTRANAME.bin
  else
    NEW_BINARY_NAME=$BINARY_NAME
  fi
  # copy...
  cp $BINARY_NAME $ZIPDIR/$NEW_BINARY_NAME || { echo "Build of $BOARDNAME failed" ; exit 1; }
done

cd $DIR

sed 's/$/\r/' dist_readme.txt > $ZIPDIR/readme.txt
bash scripts/extract_changelog.sh | sed 's/$/\r/' > $ZIPDIR/changelog.txt
#bash scripts/extract_todo.sh  >  $ZIPDIR/todo.txt
python scripts/build_docs.py  || { echo 'Build failed' ; exit 1; } 
mv $DIR/functions.html $ZIPDIR/functions.html
cp $DIR/dist_licences.txt $ZIPDIR/licences.txt

rm -f $ZIPFILE
cd zipcontents
zip $ZIPFILE *
