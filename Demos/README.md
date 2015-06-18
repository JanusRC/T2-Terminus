STM32 C Language Demos
=====

Demos for the T2 Terminus, split into F2 and F4 variants if necessary.
The current demos will work for either variant, as they don't really use the extra functionality
of the F4 version.

`T2 AppPack`: This is utilized to sign the standard .hex output files to a .bin that the demo bootloader will recognize.

### F2 Demos

`T2_F4_VCP_Demo`: Virtual COM port demo that allows the user to talk with the internal modem via AT commands through the USB port instead of the DB9.

### F4 Demos

`bin`: Container for pre-compiled binary files, ready for production bootloader upload.

`T2_F4_VCP_Demo`: Virtual COM port demo that allows the user to talk with the internal modem via AT commands through the USB port instead of the DB9.

`T2_Function_Demo`: This is a more general/all encompassing demonstration of the T2. It contains:

* Basic up and run of the unit
* Modem control
* Uart control w/ ring buffer
* Front end terminal for manual control of the modem (ON/OFF, Registration/Activation, SMS, Socket Handling)
* 3 Full demonstrations containing persistent network handling and continuous function
* -Socket Client
* -Socket Host
* -SMS Echo
* Modularized code for easy edit

The demonstrations are commented out initially, if the Terminal is exited a standard UART access is granted @ 115200 baud rate to the modem directly. 


