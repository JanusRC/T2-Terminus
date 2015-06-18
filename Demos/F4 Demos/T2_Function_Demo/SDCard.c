//****************************************************************************
// T2 SD Card Handling
//****************************************************************************
#include "main.h"

// FatFs includes
#include "ff.h"
#include "diskio.h"

FRESULT res;
FILINFO fno;
FIL fil;
DIR dir;
FATFS fs32;
char* path;
char *fn;   /* This function is assuming non-Unicode cfg. */

//****************************************************************************

//typedef enum {
//	FR_OK = 0,					/* (0) Succeeded */
//	FR_DISK_ERR,				/* (1) A hard error occured in the low level disk I/O layer */
//	FR_INT_ERR,					/* (2) Assertion failed */
//	FR_NOT_READY,				/* (3) The physical drive cannot work */
//	FR_NO_FILE,					/* (4) Could not find the file */
//	FR_NO_PATH,					/* (5) Could not find the path */
//	FR_INVALID_NAME,		/* (6) The path name format is invalid */
//	FR_DENIED,					/* (7) Acces denied due to prohibited access or directory full */
//	FR_EXIST,						/* (8) Acces denied due to prohibited access */
//	FR_INVALID_OBJECT,	/* (9) The file/directory object is invalid */
//	FR_WRITE_PROTECTED,	/* (10) The physical drive is write protected */
//	FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
//	FR_NOT_ENABLED,			/* (12) The volume has no work area */
//	FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume */
//	FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any parameter error */
//	FR_TIMEOUT,					/* (15) Could not get a grant to access the volume within defined period */
//	FR_LOCKED,					/* (16) The operation is rejected according to the file shareing policy */
//	FR_NOT_ENOUGH_CORE,			/* (17) LFN working buffer could not be allocated */
//	FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > _FS_SHARE */
//	FR_INVALID_PARAMETER		/* (19) Given parameter is invalid */
//} FRESULT;

//****************************************************************************

//TODO - Add functions
//SDCard_Mount()
//SDCard_Unmount()
//SDCard_List()
//SDCard_Read()
//SDCard_Write

void SDCard_Mount()
{
  memset(&fs32, 0, sizeof(FATFS));

  res = f_mount(0, &fs32);

  if (res != FR_OK)
    printf("res = %d f_mount\n", res);
	else
		printf("Error: %d\r\n",res); 

}


void FatTest(void)
{
  puts("FatFs Testing");

  memset(&fs32, 0, sizeof(FATFS));

  res = f_mount(0, &fs32);

  if (res != FR_OK)
    printf("res = %d f_mount\n", res);

  // Echo MESSAGE.TXT to console

  res = f_open(&fil, "MESSAGE.TXT", FA_READ);

  if (res != FR_OK)
    printf("res = %d f_open MESSAGE.TXT\n", res);

  if (res == FR_OK)
  {
    UINT Total = 0;

    while(1)
    {
      BYTE Buffer[512];
      UINT BytesRead;
      UINT i;

      res = f_read(&fil, Buffer, sizeof(Buffer), &BytesRead);

      if (res != FR_OK)
        printf("res = %d f_read MESSAGE.TXT\n", res);

      if (res != FR_OK)
        break;

      Total += BytesRead;

      for(i=0; i<BytesRead; i++)
        putchar(Buffer[i]);

      if (BytesRead < sizeof(Buffer))
        break;
    } // while

    res = f_close(&fil);

    if (res != FR_OK)
      printf("res = %d f_close MESSAGE.TXT\n", res);

    printf("Total = %d\n", Total);

    // Write length of MESSAGE.TXT to LENGTH.TXT

    res = f_open(&fil, "LENGTH.TXT", FA_CREATE_ALWAYS | FA_WRITE);

    if (res != FR_OK)
      printf("res = %d f_open LENGTH.TXT\n", res);

    if (res == FR_OK)
    {
      UINT BytesWritten;
      char crlf[] = "\r\n";
      char s[16];

      sprintf(s, "%d", Total);

      res = f_write(&fil, s, strlen(s), &BytesWritten);

      res = f_write(&fil, crlf, strlen(crlf), &BytesWritten);

      res = f_close(&fil); // LENGTH.TXT

      if (res != FR_OK)
        printf("res = %d f_close LENGTH.TXT\n", res);
    }
  }

  // Incremental write test

  if (f_open(&fil, "LOG.TXT", FA_OPEN_ALWAYS | FA_WRITE) == FR_OK)
  {
    UINT BytesWritten;
    const char string[] = "Another line gets added\r\n";

    f_lseek(&fil, fil.fsize);
    f_write(&fil, string, strlen(string), &BytesWritten);
    f_close(&fil);
  }

#if 0
  // Write COUNTER.TXT, exercise file system integrity

  res = f_open(&fil, "COUNTER.TXT", FA_CREATE_ALWAYS | FA_WRITE);

  if (res != FR_OK)
    printf("res = %d f_open COUNTER.TXT\n", res);

  if (res == FR_OK)
  {
    int i;

    for(i=0; i<1000000; i++)
    {
      UINT BytesWritten;
      char crlf[] = "\r\n";
      char s[16];

      sprintf(s, "%10d", i);

      res = f_write(&fil, s, strlen(s), &BytesWritten);

      res = f_write(&fil, crlf, strlen(crlf), &BytesWritten);
    }

    res = f_close(&fil); // COUNTER.TXT

    if (res != FR_OK)
      printf("res = %d f_close COUNTER.TXT\n", res);
  }
#endif

#if 0
  // Write COUNTER.TXT, exercise file system integrity

  res = f_open(&fil, "COUNTER.TXT", FA_CREATE_ALWAYS | FA_WRITE);

  if (res != FR_OK)
    printf("res = %d f_open COUNTER.TXT\n", res);

  if (res == FR_OK)
  {
    int i;

    for(i=0; i<10000; i++)
    {
      UINT BytesWritten;

      res = f_write(&fil, (void *)0x1FFFF000, 0x1000, &BytesWritten);
    }

    res = f_close(&fil); // COUNTER.TXT

    if (res != FR_OK)
      printf("res = %d f_close COUNTER.TXT\n", res);
  }
#endif

  // Directory test

  path = "";

  res = f_opendir(&dir, path);

  if (res != FR_OK)
    printf("res = %d f_opendir\n", res);

  if (res == FR_OK)
  {
    while(1)
    {
      char *fn;
#if _USE_LFN
      static char lfn[_MAX_LFN + 1];
#endif

#if _USE_LFN
      fno.lfname = lfn;
      fno.lfsize = sizeof lfn;
#endif

      res = f_readdir(&dir, &fno);

      if (res != FR_OK)
        printf("res = %d f_readdir\n", res);

      if ((res != FR_OK) || (fno.fname[0] == 0))
        break;

#if _USE_LFN
      fn = *fno.lfname ? fno.lfname : fno.fname;
#else
      fn = fno.fname;
#endif

      printf("%c%c%c%c ",
        ((fno.fattrib & AM_DIR) ? 'D' : '-'),
        ((fno.fattrib & AM_RDO) ? 'R' : '-'),
        ((fno.fattrib & AM_SYS) ? 'S' : '-'),
        ((fno.fattrib & AM_HID) ? 'H' : '-') );

      printf("%10d ", fno.fsize);

      printf("%s/%s\n", path, fn);
    }
  }

  puts("Done");
}
