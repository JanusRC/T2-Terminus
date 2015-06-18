//****************************************************************************
// FIFO Library
//****************************************************************************
#include "RingBuffer.h"


extern int Sleep(int Delay);	//From Main

//****************************************************************************
// Simple FIFO buffering example, designed to hold over a second of data
//  at 38400 baud (ie around 3840 bps)
//****************************************************************************

//****************************************************************************

FIFO *Fifo_Open(FIFO *Fifo)
{
  memset(Fifo, 0, sizeof(FIFO));

  Fifo->FifoHead = 0;
  Fifo->FifoTail = 0;
  Fifo->FifoSize = FIFO_SIZE;

  return(Fifo);
}

//****************************************************************************

void Fifo_Close(FIFO *Fifo)
{
  // free(Fifo); // Dynamic implementation
}

//****************************************************************************

DWORD Fifo_Used(FIFO *Fifo)
{
  DWORD Used;

   if (Fifo->FifoHead >= Fifo->FifoTail)
     Used = (Fifo->FifoHead - Fifo->FifoTail);
   else
     Used = ((Fifo->FifoHead + Fifo->FifoSize) - Fifo->FifoTail);

  return(Used);
}

//****************************************************************************

DWORD Fifo_Free(FIFO *Fifo)
{
  return((Fifo->FifoSize - 1) - Fifo_Used(Fifo));
}

//****************************************************************************

void Fifo_Flush(FIFO *Fifo)
{
  Fifo->FifoHead = Fifo->FifoTail;
}

//****************************************************************************

// Insert stream of data into FIFO, assumes stream is smaller than FIFO
//  will fail if insufficient space which will only occur if the worker
//  process is not extracting and processing the data in a timely fashion

BOOL Fifo_Insert(FIFO *Fifo, DWORD Size, BYTE *Buffer)
{
  if (Size > Fifo_Free(Fifo))
    return(FALSE);

  while(Size)
  {
    DWORD Length;

    Length = min(Size, (Fifo->FifoSize - Fifo->FifoHead));

    memcpy(&Fifo->FifoBuffer[Fifo->FifoHead], Buffer, Length);

    Fifo->FifoHead += Length;

    if (Fifo->FifoHead == Fifo->FifoSize)
      Fifo->FifoHead = 0;

    Size -= Length;
    Buffer += Length;
  }

  return(TRUE);
}

//****************************************************************************

int Fifo_Extract(FIFO *Fifo, DWORD Size, BYTE *Buffer)
{
  if (Size > Fifo_Used(Fifo))
    return(FALSE);

  while(Size)
  {
    DWORD Length;

    Length = min(Size, (Fifo->FifoSize - Fifo->FifoTail));

    memcpy(Buffer, &Fifo->FifoBuffer[Fifo->FifoTail], Length);

    Fifo->FifoTail += Length;

    if (Fifo->FifoTail == Fifo->FifoSize)
      Fifo->FifoTail = 0;

    Size -= Length;
    Buffer += Length;
  }

  return(TRUE);
}

//****************************************************************************

void Fifo_Forward(FIFO *FifoA, FIFO *FifoB)
{
  DWORD Used;
  DWORD Free;
  DWORD Size;
  BYTE Buffer[32]; // Could make this bigger

  // Half Duplex, currently not totally interrupt safe, could a) check
  //  the Tx buffer space more often, or b) always limit the apparent free space

  Used = Fifo_Used(FifoA);
  Free = Fifo_Free(FifoB);

  Size = min(Used, Free);

  while(Size) // Either run out of source, or the destination fills
  {
    Size = min(Size, sizeof(Buffer));

    Fifo_Extract(FifoA, Size, Buffer);

    Fifo_Insert(FifoB, Size, Buffer);

    Used -= Size;
    Free -= Size;

    Size = min(Used, Free);
  }
}

//****************************************************************************

char Fifo_PeekChar(FIFO *Fifo)
{
  if (!Fifo_Used(Fifo)) // No Data available
    return(0);

  return(Fifo->FifoBuffer[Fifo->FifoTail]);
}

//****************************************************************************

char Fifo_GetChar(FIFO *Fifo)
{
  char c;

  if (!Fifo_Used(Fifo)) // No data available
    return(0);

  c = Fifo->FifoBuffer[Fifo->FifoTail++];

  if (Fifo->FifoTail == Fifo->FifoSize)
    Fifo->FifoTail = 0;

  return(c);
}

//****************************************************************************

char Fifo_PutChar(FIFO *Fifo, char c)
{
  if (!Fifo_Free(Fifo)) // No space available
    return(0);

  Fifo->FifoBuffer[Fifo->FifoHead++] = c;

  if (Fifo->FifoHead == Fifo->FifoSize)
    Fifo->FifoHead = 0;

  return(c);
}

//****************************************************************************

void Fifo_Pipe(FIFO *Fifo, DWORD Size, BYTE *Buffer)
{
  DWORD Free;
  DWORD Length;

  while(Size) // Run out of source
  {
    Free = Fifo_Free(Fifo);

    Length = min(Size, Free);

    Fifo_Insert(Fifo, Length, Buffer);

    Buffer += Length;
    Size -= Length;

    if (Size)
      Sleep(1);
  }
}

//****************************************************************************

// Returns synchronised whole lines when available, terminated with <CR><LF>
// Used for parsing Modem responses

#define FIFO_LINE_EX 600

char *Fifo_ParseGenericLine(FIFO *Fifo)
{
  static char Line[FIFO_LINE_EX + 1];
  DWORD Used;
  DWORD i, j, k;

  Used = Fifo_Used(Fifo);

  if (!Used) // No Data available
    return(NULL);

//printf("Used#%5d\n",Used);

  i = 0;

  k = Fifo->FifoTail;

  // Find end-of-line, or run to extent of FIFO

  j = i;

  do
  {
    j++;

    k++;

    if (k == Fifo->FifoSize)
      k = 0;
  }
  while((Fifo->FifoBuffer[k] != 0x0D) &&  // <CR><LF> Framing
        (Fifo->FifoBuffer[k] != 0x0A) &&
        (j < Used));

  // Handle case where parsed line is too long

  if ((j - i) >= FIFO_LINE_EX)
  {
    // If parsed data exceed line limit pull past initial character
    //  so that it get purged

    Fifo->FifoTail++;

    if (Fifo->FifoTail == Fifo->FifoSize)
      Fifo->FifoTail = 0;

    return(NULL);
  }

  // Leave if unable to close parsed line

  if (j == Used)
  {
    return(NULL);
  }

  // Copy element into static buffer

  k = 0;

  for(; i<j; i++)
  {
    Line[k++] = Fifo->FifoBuffer[Fifo->FifoTail++];

    if (Fifo->FifoTail == Fifo->FifoSize)
      Fifo->FifoTail = 0;
  }

  Line[k] = 0;

  // Clear out <CR><LF>

  Used -= j;

  while(Used && ((Fifo->FifoBuffer[Fifo->FifoTail] == 0x0D) ||
                 (Fifo->FifoBuffer[Fifo->FifoTail] == 0x0A)))
  {
    Used--;

    Fifo->FifoTail++;

    if (Fifo->FifoTail == Fifo->FifoSize)
      Fifo->FifoTail = 0;
  }

  return(Line);
}
