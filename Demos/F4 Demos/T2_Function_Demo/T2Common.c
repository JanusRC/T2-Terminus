//****************************************************************************
// Common T2 Function calls
//****************************************************************************
#include "main.h"

//****************************************************************************
extern volatile int SystemTick;	//From Main
unsigned int FiveMicro;

//****************************************************************************

// From http://forums.arm.com/index.php?showtopic=13949

volatile unsigned int *DWT_CYCCNT   = (volatile unsigned int *)0xE0001004; //address of the register
volatile unsigned int *DWT_CONTROL  = (volatile unsigned int *)0xE0001000; //address of the register
volatile unsigned int *SCB_DEMCR    = (volatile unsigned int *)0xE000EDFC; //address of the register

//****************************************************************************

void EnableTiming(void)
{
  static int enabled = 0;

  if (!enabled)
  {
    RCC_ClocksTypeDef RCC_Clocks;

    RCC_GetClocksFreq(&RCC_Clocks);

    FiveMicro = RCC_Clocks.HCLK_Frequency / (1000000 / 5); // Clock ticks for ~5us

    *SCB_DEMCR |= 0x01000000;
    *DWT_CYCCNT = 0; // reset the counter
    *DWT_CONTROL |= 1 ; // enable the counter

    enabled = 1;
  }
}

//****************************************************************************

void TimingDelay(unsigned int tick)
{
  unsigned int start, current;

  start = *DWT_CYCCNT;

  do
  {
    current = *DWT_CYCCNT;
  } while((current - start) < tick);
}

//****************************************************************************

// Win32 style implementation of time delay, or yield type functionality
//  Timer increments on timer tick interrupt, so processor halted with
//  Thumb2 WFI instruction so this won't grind in a loop unnecessarily

int Sleep(int Delay)
{
  int Start, Current;

  Start = SystemTick;

  do
  {
    __WFI();                  // Wait for interrupt

    Current = SystemTick;
  }
  while((Current - Start) < Delay);

  return(Current - Start); // Estimate of time past
}

//****************************************************************************
//
// Augment runtime with common functions present in Microsoft C
//
//****************************************************************************

/* lstrncmpi - case insensitive string compare up to n chars */
int strnicmp(char *str1, char *str2, int n)
{
  if (!n)
    return(0);

  while((--n > 0) && *str1)
  {
    int res;
		
		res = toupper(*str1++) - toupper(*str2++);

    if (res)
      return(res);
  }

  return(toupper(*str1) - toupper(*str2));
}

//****************************************************************************

/* lstrcmpi - case insensitive string compare */
int stricmp(char *str1, char *str2)
{
  while(*str1)
  {
    int res;
		
		res = toupper(*str1++) - toupper(*str2++);

    if (res)
      return(res);
  }

  return(toupper(*str1) - toupper(*str2));
}

//****************************************************************************

static const char Hex[] = "0123456789ABCDEF";

//****************************************************************************

// Generic but efficient hexidecimal memory dumping routines

char *DumpLine(DWORD Addr, DWORD Size, BYTE *Buffer)
{
  DWORD i;
  char *p;
  static char OutLine[80];

  p = OutLine;

  *p++ = Hex[((Addr >> 28) & 0x0F)];
  *p++ = Hex[((Addr >> 24) & 0x0F)];
  *p++ = Hex[((Addr >> 20) & 0x0F)];
  *p++ = Hex[((Addr >> 16) & 0x0F)];
  *p++ = Hex[((Addr >> 12) & 0x0F)];
  *p++ = Hex[((Addr >>  8) & 0x0F)];
  *p++ = Hex[((Addr >>  4) & 0x0F)];
  *p++ = Hex[((Addr >>  0) & 0x0F)];

  *p++ = ' ';
  *p++ = ':';
  *p++ = ' ';

  for(i=0; i<Size; i++)
  {
    *p++ = Hex[((Buffer[i] >> 4) & 0x0F)];
    *p++ = Hex[((Buffer[i] >> 0) & 0x0F)];

    if (i == 7)
      *p++ = '-';
    else
      *p++ = ' ';
  }

  for(i=Size; i<16; i++)
  {
    *p++ = ' ';
    *p++ = ' ';
    *p++ = ' ';
  }

  for(i=0; i<Size; i++)
  {
    if ((Buffer[i] >= 32) && (Buffer[i] <= 126))
      *p++ = Buffer[i];
    else
      *p++ = '.';
  }

  *p++ = '\r';
  *p++ = '\n';

  *p++ = 0;

  return(OutLine);
}

//****************************************************************************

void DumpData(UART *Uart, DWORD Size, BYTE *Buffer)
{
  DWORD Addr;
  DWORD Length;

  Addr = 0;

  while(Size)
  {
    Length = min(16, Size);

    Uart_SendMessage(Uart, DumpLine(Addr, Length, Buffer));

    Buffer += Length;

    Addr += Length;
    Size -= Length;
  }
}

//****************************************************************************

void DumpMemory(UART *Uart, DWORD Addr, DWORD Size, BYTE *Buffer)
{
  DWORD Length;

  while(Size)
  {
    Length = min(16, Size);

    Uart_SendMessage(Uart, DumpLine(Addr, Length, Buffer));

    Buffer += Length;

    Addr += Length;
    Size -= Length;
  }
}

//****************************************************************************

//void NukeApplication(void)
//{
//  printf("Nuking..\r\n");

//  while(Fifo_Used(UartDebug->Tx)) // Wait for debug FIFO to flush
//    Sleep(100);

//  Sleep(100);

//  *((unsigned long *)0x2001FFFC) = 0xCAFE0001; // Erase Application

//  SystemReset();
//}

////****************************************************************************

//void BootDfuSe(void)
//{
//  printf("DfuSe..\r\n");

//  //while(Fifo_Used(UartDebug->Tx)) // Wait for debug FIFO to flush
//  //  Sleep(100);

//  Sleep(100);

//  *((unsigned long *)0x2001FFFC) = 0xBEEFBEEF; // Boot to ROM

//  SystemReset();
//}

//****************************************************************************

//void Debug_SendMessage(char *Buffer)
//{
//#ifdef USE_ITM
//  Debug_ITMDebugOutputString(Buffer);
//#else
//  int i;

//  i = strlen(Buffer);

//  while(Fifo_Used(UartDebug->Tx)) Sleep(10);

//  // Echo to Debug
//  Uart_SendBuffer(UartDebug, (DWORD)i, (BYTE *)Buffer);
//#endif
//}

////****************************************************************************

//void Debug_SendChar(char ch)
//{
//#ifdef USE_ITM
//  Debug_ITMDebugOutputChar(ch);
//#else
//  while(!Fifo_Insert(UartDebug->Tx, 1, (void *)&ch)) Sleep(10);
//#endif
//}

////****************************************************************************

//void Debug_Flush(void)
//{
//  while(Fifo_Used(UartDebug->Tx)) // Wait for debug FIFO to flush
//    Sleep(100);
//}

//******************************************************************************

extern void * Stack_Mem;

int StackDepth(void)
{
  int i;
  unsigned long *p;

  p = (unsigned long *)&Stack_Mem;

  i = 0;

  while(!*p++)
    i += 4;

  i = 0x1000 - i;

  return(i);
}

//****************************************************************************

// 0x00000000..000FFFFF SHADOW
// 0x08000000..080FFFFF FLASH (1MB)
// 0x20000000..2001FFFF RAM (128KB)
// 0x40000000..5FFFFFFF PERIPHERALS

//****************************************************************************

int AddressValid(DWORD Addr)
{
  if (/*(Addr >= 0x00000000) && */(Addr <= 0x000FFFFF)) // Boot Mapping
    return(1);

  if ((Addr >= 0x08000000) && (Addr <= 0x080FFFFF)) // 1MB FLASH
    return(1);

  if ((Addr >= 0x20000000) && (Addr <= 0x2001FFFF)) // 128KB RAM
    return(1);

  if ((Addr >= 0x1FFF0000) && (Addr <= 0x1FFF7FFF)) // ROM
    return(1);

  if ((Addr >= 0x1FFFC000) && (Addr <= 0x1FFFC007)) // Option Bytes
    return(1);

  return(0);
}

//****************************************************************************
//ITM Stuff
void Debug_ITMDebugEnable(void)
{
  volatile unsigned int *ITM_TER      = (volatile unsigned int *)0xE0000E00;
  volatile unsigned int *ITM_TCR      = (volatile unsigned int *)0xE0000E80;
  volatile unsigned int *SCB_DEMCR    = (volatile unsigned int *)0xE000EDFC;

  *((u32 *)0xE0042004) |= 0x27;

//printf("%08X %08X %08X\n",*SCB_DEMCR,*ITM_TCR,*ITM_TER);

#if 1
  *SCB_DEMCR |= 0x01000000; // Enable Trace Unit
  *ITM_TCR   |= 0x00000001; // Enable ITM     - Trace Control Register
  *ITM_TER   |= 0x00000001; // Enable Port 0  - Trace Enable Register
#endif
}

//****************************************************************************

void Debug_ITMDebugOutputChar(char ch)
{
  static volatile unsigned int *ITM_STIM0 = (volatile unsigned int *)0xE0000000; // ITM Port 0
  static volatile unsigned int *SCB_DEMCR = (volatile unsigned int *)0xE000EDFC;

//printf("%08X %08X ITMDebugOutputChar\n",*SCB_DEMCR,*ITM_STIM0);

  if (*SCB_DEMCR & 0x01000000)
  {
    while(*ITM_STIM0 == 0);
    *((volatile char *)ITM_STIM0) = ch;
  }
}

//****************************************************************************

void Debug_ITMDebugOutputString(char *Buffer)
{
  while(*Buffer)
    Debug_ITMDebugOutputChar(*Buffer++);
}

//****************************************************************************

void ITM_SendString(char *s)
{
  while(*s)
    ITM_SendChar(*s++);
}

//****************************************************************************

// After Joseph Yiu's implementation

void hard_fault_handler_c(unsigned int * hardfault_args)
{
  printf ("[Hard Fault]\n");

  printf ("r0 = %08X, r1 = %08X, r2 = %08X, r3 = %08X\n",
    hardfault_args[0], hardfault_args[1], hardfault_args[2], hardfault_args[3]);
  printf ("r12= %08X, lr = %08X, pc = %08X, psr= %08X\n",
    hardfault_args[4], hardfault_args[5], hardfault_args[6], hardfault_args[7]);

  printf ("bfar=%08X, cfsr=%08X, hfsr=%08X, dfsr=%08X, afsr=%08X\n",
    *((volatile unsigned long *)(0xE000ED38)),
    *((volatile unsigned long *)(0xE000ED28)),
    *((volatile unsigned long *)(0xE000ED2C)),
    *((volatile unsigned long *)(0xE000ED30)),
    *((volatile unsigned long *)(0xE000ED3C)) );

  //while(1);

  return;
}
