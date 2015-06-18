//****************************************************************************
// FIFO Library
//****************************************************************************
#include <string.h>
#include "T2Common.h"

#define FIFO_SIZE 1024
//#define FIFO_SIZE 2048

typedef struct _FIFO {
  DWORD FifoHead;
  DWORD FifoTail;
  DWORD FifoSize;
  BYTE FifoBuffer[FIFO_SIZE];
} FIFO;

//FIFO *Fifo;

FIFO *Fifo_Open(FIFO *Fifo);
void Fifo_Close(FIFO *Fifo);
DWORD Fifo_Used(FIFO *Fifo);
DWORD Fifo_Free(FIFO *Fifo);
void Fifo_Flush(FIFO *Fifo);
BOOL Fifo_Insert(FIFO *Fifo, DWORD Size, BYTE *Buffer);
int Fifo_Extract(FIFO *Fifo, DWORD Size, BYTE *Buffer);
int Fifo_Extract(FIFO *Fifo, DWORD Size, BYTE *Buffer);
void Fifo_Forward(FIFO *FifoA, FIFO *FifoB);
char Fifo_PeekChar(FIFO *Fifo);
char Fifo_GetChar(FIFO *Fifo);
char Fifo_PutChar(FIFO *Fifo, char c);
void Fifo_Pipe(FIFO *Fifo, DWORD Size, BYTE *Buffer);
char *Fifo_ParseGenericLine(FIFO *Fifo);
