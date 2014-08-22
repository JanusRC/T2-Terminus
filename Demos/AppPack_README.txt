In order to use the T2 Terminus Demos with the factory demo bootloader, you MUST
use the T2AppPack executable to sign the firmware so the bootloader can recognize/use it.

The T2APPPACK executable will sign the .hex output file into a .bin
file that the bootloader can read and verify via 32 bit CRC.

The CreateSignedBin.bat file simply passes the arguments automatically,
please edit it to adjust the .hex file name to your own if different than
what is set.

If you set up the toolchain with the skeleton tree and settings, this is automatically run
post-process as well as a file renaming.

By default the CreateSignedBin.bat file wants the filename TERMINUS2.hex, please edit as you see fit.

The output by default of the executable is t2appack.bin