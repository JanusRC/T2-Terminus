//****************************************************************************
// Header file for some basic defines that the project may us
//****************************************************************************

//Board/Device Revision
//#define REV1 // PB2409R0
#define REV2 // PB2409R1

//****************************************************************************

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL;

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define FALSE 0
#define TRUE (!FALSE)

#define STDERR ((void *)0x12345678)
