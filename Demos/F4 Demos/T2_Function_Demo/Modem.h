//****************************************************************************
// Terminus Modem Library
//****************************************************************************
#define HE910
//#define DE910
//#define CC864

#ifdef HE910
#define TELIT_TIME_ON  5000 // HE910
#define TELIT_TIME_OFF 3300
#endif // HE910

#ifdef DE910
#define TELIT_TIME_ON  1250
#define TELIT_TIME_OFF 2500
#endif // DE910

#define MODEM_POWER_DELAY 10000  // 10 Seconds - More than enough for the modem to initialize
//#define MODEM_POWER_DELAY 5500 // Takes upto 6 to read SIM
//#define MODEM_POWER_DELAY 8000  // 8 Seconds

