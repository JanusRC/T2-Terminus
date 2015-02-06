/* Espruino integration for STM32F2 and F4 SDIO - cturvey@janus-rc.com - clive - 5-Feb-2015
   Target platform STM32F4-DISCO with STM32F4-DIS-BB Breakout Board w/MicroSD slot
   Most pins will remain the same for other F2/F4 platforms, adjust Card Detect pin GPIO
   as required.
*/

#include "platform_config.h"
#include "jsinteractive.h"

#include "ff.h"
#include "diskio.h"
#include "stm32f2_4_sdio_sd.h"

//#include <string.h>

#define BLOCK_SIZE            512 /* Block Size in Bytes */

/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
  BYTE drv /* Physical drive number (0) */
  )
{
  static int InitializedSDIO = 0;
  SD_Error Status;

 	/* Supports only single drive */
  if (drv)
  {
    return STA_NOINIT;
  }

  if (!InitializedSDIO) // As FatFs calls more than once
  {
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = SD_SDIO_DMA_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    Status = SD_Init(); // This will initialize the pins and peripherals

    if (Status!=SD_OK )
    {
//  puts("Initialization Fail");
      return STA_NOINIT;
    }
    else
    {
      InitializedSDIO = 1;

      return RES_OK;
    }
  }

  return RES_OK;
}

/*-----------------------------------------------------------------------*/

/**
  * @brief  This function handles SDIO global interrupt request.
  * @param  None
  * @retval None
  */
//void SDIO_IRQHandler(void)
//{
  /* Process All SDIO Interrupt Sources */
//  SD_ProcessIRQSrc();
//}

void SD_SDIO_DMA_IRQHANDLER(void)
{
  SD_ProcessDMAIRQ();
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
  BYTE drv /* Physical drive number (0) */
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
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
  BYTE drv,     /* Physical drive number (0) */
  BYTE *buff,   /* Pointer to the data buffer to store read data */
  DWORD sector, /* Start sector number (LBA) */
  UINT count    /* Sector count */
  )
{
	SD_Error Status;

//	printf("disk_read %d %p %10d %d\n",drv,buff,sector,count);

	if (SD_Detect() != SD_PRESENT)
		return(RES_NOTRDY);

	if ((DWORD)buff & 3) // DMA Alignment failure, do single up to aligned buffer
	{
		DRESULT res = RES_OK;
		DWORD scratch[BLOCK_SIZE / 4]; // Alignment assured, you'll need a sufficiently big stack

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
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
  BYTE drv,         /* Physical drive number (0) */
  const BYTE *buff, /* Pointer to the data to be written */
  DWORD sector,     /* Start sector number (LBA) */
  UINT count        /* Sector count */
  )
{
	SD_Error Status;

//	printf("disk_write %d %p %10d %d\n",drv,buff,sector,count);

	if (SD_Detect() != SD_PRESENT)
		return(RES_NOTRDY);

	if ((DWORD)buff & 3) // DMA Alignment failure, do single up to aligned buffer
	{
		DRESULT res = RES_OK;
		DWORD scratch[BLOCK_SIZE / 4]; // Alignment assured, you'll need a sufficiently big stack

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

  Status = SD_WriteMultiBlocksFIXED(buff, sector, BLOCK_SIZE, count); // 4GB Compliant

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


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
  BYTE drv, // Physical drive number (0)
  BYTE ctrl, // Control code
  void *buff // Buffer to send/receive control data
  )
{
  DRESULT res = RES_OK;

  return res;
}
