Janus_JS
========

http://www.janus-rc.com/T2Terminus

About
-----

Subfolder to contain all of of the Janus espruino based javascript demos and associated files for use with the Terminus T2. Copy all of these to your Espruino "project" folder to make the best use of these.

### Directories and Files

* `binary/`:            Precompiled T2 builds, only direct STLINK flashing is currently supported.
* `modules/`:           Modules created by Janus for use with the T2/910CF
* `projects/`:          Demo code and examples created by Janus
* `snippets/`:          Common code snippets, used with the WebIDE's project ability.

Modules
-----

Use these with the require.('modulename') notation. Usage and example code is noted within each one. For the modules that have larger apps to do more complex things, you can individually set the verbose mode to 'false' to keep the console prints quiet or not. 

It's HIGHLY recommended that if using the WebIDE, you use the "module minification" option. The modules contain a lot of comments that eat up memory if you don't use this option.

### T2HW
Hardware defines for the T2 Terminus, for easier usage in the application. Currently this does not initialize any I/O, it just maps everything for ease. 

### CF910
Hardware handling of the Janus CF module, including powering up/down. This is aimed at the x910 variants of CF modules, not tested/optimized for older hardware. You can uncomment the I/O defines to use the CF modules outside of the T2 Terminus hardware. 

### ATC
AT Command handler, uses an iterative approach to commands since Espruino currently cannot do something like "send AT command, wait for response" in a single loop. It must return to idle state for backend processing of the received data. This may improve later with a more streamlined approach.

### NETWORK
Network interface handling. This includes checking registration and basic modem information such as SIM readiness and given phone number.

### SMS
SMS handling for sending and receiving text messages. Only supports text based messages, not PDU. 

### SOCKET
Socket/GPRS handling. This includes setting context information and activating. This also contains handlers for opening/closing of data or command based sockets.

### BMA222
Driver for the on board accelerometer in the T2. Currently only supports polling, not the interrupts.

Demos and Example Code
----------------------

These are demonstrations written by Janus RC to show how to utilize the above modules in an M2M application. These are based on a clocking structure, codenamed "Tick-Tock". Wherein state machines are used to conform with Espruino's "return to idle" requirement. Information is noted within each one.
Tick Programs - The main Programs/applications that run via state machine
Tock Apps - The module apps/sub-machines that do more complicated things such as opening a socket.

These demos currently use a very basic approach, they do not make use of Espruino's interrupts (other than the main interval setting).

### Demo_Registration
This demonstration simply boots the modem and checks for regisration. 

Once regisration is confirmed, the demo exits.

### Demo-SMSEcho
This demonstration boots the unit, registers, and waits for an incoming SMS. Once found it will echo the SMS back to the sender. 

This is the only demo that contains a full looping/persistent mechanism.

### Demo_Serial2GPRS
This demonstration boots the unit, registers, and depending on how you set the configuration information it will open a socket to a remote host or listen for an incoming connection. Please understand that you will need a routable IP address or VPN to make use of the host option. 

Once a connection has been opened, the DB9/serial port becomes a pipe to the host/client. 

Host mode has a secondary option wherein you can allow remote control of the Espruino by giving the remote client access to the console. You will not be able to run any AT commands but the rest of the hardware can be manipulated. 

Once the connection closes, the demo exits.


===


