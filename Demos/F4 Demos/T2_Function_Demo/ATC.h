//****************************************************************************
// Modem AT Handling
//****************************************************************************
#define PROBE_TIMEOUT_RX  500
#define ATC_STD_TIMEOUT		1500					// 1500 ms ATC standard timeout, more than enough (1s standard)
#define NET_QRY_TIMEOUT		(145 * 1000)	// 145 Seconds 2.4 minutes, max network query time
#define NET_REG_TIMEOUT		(35 * 1000)		// 35s Registration check timeout. Usual time is ~15 seconds from cold start
#define NET_SKT_TIMEOUT		(35 * 1000)		// 35s Socket Dial check timeout. Usually only takes a couple of seconds. Adjust with AT#SCFG connTO setting
#define NET_SMS_TIMEOUT		(35 * 1000)		// 35s SMS Sending timeout. Should never take this much time to get a response

//****************************************************************************

#define FAULT_OK                	0
#define FAULT_FAIL                0xFF000000
#define FAULT_MODEM_NOT_CONNECTED 0xFF000001
#define FAULT_TIMEOUT             0xFF000002
#define FAULT_MODEM_OFF           0xFF000003
#define FAULT_NOT_REGISTERED      0xFF000004
#define FAULT_NOT_ACTIVE          0xFF000005
#define FAULT_ERROR               0xFF000006
#define FAULT_NO_CARRIER          0xFF000007
#define FAULT_DENIED              0xFF000008
#define FAULT_BUSY                0xFF000009
#define FAULT_RING                0xFF00000A
#define FAULT_SRING								0xFF00000B
#define FAULT_CONNECT             0xFF00000C
#define FAULT_NO_DIALTONE         0xFF00000D
#define FAULT_NO_ANSWER           0xFF00000E
#define FAULT_PROMPT_FOUND        0xFF00000F
#define FAULT_CMS                 0xFF100000
#define FAULT_CME                 0xFF200000



