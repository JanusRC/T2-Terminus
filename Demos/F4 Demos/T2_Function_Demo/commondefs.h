//****************************************************************************
//
// Header file for inclusion for areas that require these defines
//
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
