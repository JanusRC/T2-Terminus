T2 Terminus
===

www.janus-rc.com/T2Terminus

Janus T2 Terminus repository for demo code and references
The T2 Terminus utilizes an STM32 processor, and has various peripherals for usage.
These demos should aid in getting an application up and running.

### Bootloader
Factory bootloader for the T2 Terminus.

* `F2 Bootloader`: Bootloader demo for the STM32F2 based units (older units)
* `F4 Bootloader`: Bootloader demo for the STM32F4 based units (current production)


### Skeleton File Tree
As the name implies, a simple copy/paste file tree for use with Keil to ensure all
libraries and peripheral drivers are in place.



Demos
=====

Demos for the T2 Terminus, split into F2 and F4 variants if necessary.
The current demos will work for either variant, as they don't really use the extra functionality
of the F4 version.

* `T2 AppPack`: This is utilized to sign the standard .hex output files to a .bin that the demo bootloader will recognize.

### F2 Demos

* `T2_F4_VCP_Demo`: Virtual COM port demo that allows the user to talk with the internal modem via AT commands through the USB port instead of the DB9.

### F4 Demos

* `T2_F4_VCP_Demo`: Virtual COM port demo that allows the user to talk with the internal modem via AT commands through the USB port instead of the DB9.






