/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"

#include <stdio.h>
#include <string.h> // memcpy

#include "stm32_t2.h"
#include "stm32_t2_sdio_sd.h"

#define BLOCK_SIZE            512 /* Block Size in Bytes */

//#define DBG_DISKIO

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
        BYTE drv                                /* Physical drive nmuber (0..) */
)
{
  SD_Error  Status;

#ifdef DBG_DISKIO
  printf("disk_initialize %d\n", drv);
#endif

  /* Supports only single drive */
  if (drv)
  {
    return STA_NOINIT;
  }

  /*-------------------------- SD Init ----------------------------- */
  Status = SD_Init();

  if (Status!=SD_OK )
  {
#ifdef DBG_DISKIO
    puts("Initialization Fail");
#endif
    return STA_NOINIT;
  }
  else
  {
    return RES_OK;
  }
}


/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
        BYTE drv                /* Physical drive nmuber (0..) */
)
{
  DSTATUS stat = 0;

  if (SD_Detect() != SD_PRESENT)
    stat |= STA_NODISK;

  // STA_NOTINIT - Subsystem not initailized
  // STA_PROTECTED - Write protected, MMC/SD switch if available

  return(stat);
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
        BYTE drv,               /* Physical drive nmuber (0..) */
        BYTE *buff,             /* Data buffer to store read data */
        DWORD sector,           /* Sector address (LBA) */
        BYTE count              /* Number of sectors to read (1..255) */
)
{
  SD_Error Status;
#ifdef DBG_DISKIO
  printf("disk_read  %d %p %10d %d\n",drv,buff,sector,count);
#endif

  if (SD_Detect() != SD_PRESENT)
    return(RES_NOTRDY);

  if ((DWORD)buff & 3) // DMA Alignment failure, do single up to aligned buffer
  {
    DRESULT res = RES_OK;
    DWORD scratch[BLOCK_SIZE / 4]; // Alignment assured, you'll need a sufficiently big stack

#ifdef DBG_DISKIO
		puts("DMA");
#endif

    while(count--)
    {
      res = disk_read(drv, (void *)scratch, sector++, 1);

      if (res != RES_OK)
        break;

      memcpy(buff, scratch, BLOCK_SIZE);

      buff += BLOCK_SIZE;
    }

    return(res);
  }

  Status = SD_ReadMultiBlocksFIXED(buff, sector, BLOCK_SIZE, count); // 4GB Compliant

  if (Status == SD_OK)
  {
    SDTransferState State;

    Status = SD_WaitReadOperation(); // Check if the Transfer is finished

    while((State = SD_GetStatus()) == SD_TRANSFER_BUSY); // BUSY, OK (DONE), ERROR (FAIL)

    if ((State == SD_TRANSFER_ERROR) || (Status != SD_OK))
      return(RES_ERROR);
    else
      return(RES_OK);
  }
  else
    return(RES_ERROR);
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
#if 1 /* !VERIFY */
DRESULT disk_write (
        BYTE drv,               /* Physical drive nmuber (0..) */
        const BYTE *buff,       /* Data to be written */
        DWORD sector,           /* Sector address (LBA) */
        BYTE count              /* Number of sectors to write (1..255) */
)
{
	SD_Error Status;

#ifdef DBG_DISKIO
	printf("disk_write %d %p %10d %d\n",drv,buff,sector,count);
#endif

	if ((DWORD)buff & 3) // DMA Alignment failure, do single up to aligned buffer
	{
		DRESULT res = RES_OK;
		DWORD scratch[BLOCK_SIZE / 4]; // Alignment assured, you'll need a sufficiently big stack

#ifdef DBG_DISKIO
		puts("DMA");
#endif
		
		while(count--)
		{
			memcpy(scratch, buff, BLOCK_SIZE);

			res = disk_write(drv, (void *)scratch, sector++, 1);

			if (res != RES_OK)
				break;
			
			buff += BLOCK_SIZE;
		}
		
		return(res);
	}

  Status = SD_WriteMultiBlocksFIXED((uint8_t *)buff, sector, BLOCK_SIZE, count); // 4GB Compliant

	if (Status == SD_OK)
	{
		SDTransferState State;

		Status = SD_WaitWriteOperation(); // Check if the Transfer is finished

		while((State = SD_GetStatus()) == SD_TRANSFER_BUSY); // BUSY, OK (DONE), ERROR (FAIL)

		if ((State == SD_TRANSFER_ERROR) || (Status != SD_OK))
			return(RES_ERROR);
		else
			return(RES_OK);
	}
	else
		return(RES_ERROR);
}
#else
DRESULT disk_write_actual (
        BYTE drv,                       /* Physical drive nmuber (0..) */
        const BYTE *buff,       /* Data to be written */
        DWORD sector,           /* Sector address (LBA) */
        BYTE count                      /* Number of sectors to write (1..255) */
)
{
	SD_Error Status;

  Status = SD_WriteMultiBlocksFIXED((uint8_t *)buff, sector, BLOCK_SIZE, count); // 4GB Compliant

	if (Status == SD_OK)
	{
		SDTransferState State;

		Status = SD_WaitWriteOperation(); // Check if the Transfer is finished

		while((State = SD_GetStatus()) == SD_TRANSFER_BUSY); // BUSY, OK (DONE), ERROR (FAIL)

		if ((State == SD_TRANSFER_ERROR) || (Status != SD_OK))
			return(RES_ERROR);
		else
			return(RES_OK);
	}
	else
		return(RES_ERROR);
}

DRESULT disk_write (
        BYTE drv,                       /* Physical drive nmuber (0..) */
        const BYTE *buff,       /* Data to be written */
        DWORD sector,           /* Sector address (LBA) */
        BYTE count                      /* Number of sectors to write (1..255) */
)
{
	DRESULT res = RES_OK;
	DWORD scratch[BLOCK_SIZE / 4]; // Alignment assured, you'll need a sufficiently big stack
	int i;
	int retry;

#ifdef DBG_DISKIO
	printf("disk_write_verify %d %p %10d %d\n",drv,buff,sector,count);
#endif
		
	while(count--)
	{
		for(i=0; i<BLOCK_SIZE; i++)
			if (buff[i]) break;
			
		if (i == BLOCK_SIZE)
			printf("verify zero %10d\n", sector);

		for(retry=0; retry<3; retry++)
		{
			memcpy(scratch, buff, BLOCK_SIZE);

			res = disk_write_actual(drv, (void *)scratch, sector, 1);

			if (res != RES_OK)
			{
				printf("write fail %10d\n", sector);
				break;
			}

			res = disk_read(drv, (void *)scratch, sector, 1);
			
			if (res != RES_OK)
			{	
				printf("read fail %10d\n", sector);
				break;
			}

			if (memcmp(scratch, buff, BLOCK_SIZE) != 0)
				printf("verify error %10d %d\n", sector, retry);
			else
				break;
			
//return(RES_ERROR); // Verify Error - Die Immediately
		}

		if (res != RES_OK)
			break;

		sector++;
		buff += BLOCK_SIZE;
	}
		
	return(res);
}

#endif /* VERIFY */
#endif /* _READONLY */


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
        BYTE drv,               /* Physical drive nmuber (0..) */
        BYTE ctrl,              /* Control code */
        void *buff              /* Buffer to send/receive control data */
)
{
        return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Get current time                                                      */
/*-----------------------------------------------------------------------*/
DWORD get_fattime(void){
  return  ((2012UL-1980) << 25)       // Year = 2012
      | (5UL << 21)      // Month = May
      | (3UL << 16)      // Day = 3
      | (9U << 11)       // Hour = 9
      | (0U << 5)        // Min = 0
      | (0U >> 1)        // Sec = 0
      ;
}

