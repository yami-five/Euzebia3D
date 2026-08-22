/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/
#include "IHardware.h"
#include "hardware.h"
#include "ff.h"
#include "diskio.h"
#include "sd_driver.h"
#include "../storage/pins.h"

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

#define SD_CARD 0

#define FLASH_SECTOR_SIZE 512
const e3d_IHardware* hardware;

DSTATUS disk_initialize(
	BYTE drv /* Physical drive nmuber (0..) */
)
{
	hardware = get_hardware();
	uint8_t res = 0;
	switch (drv)
	{
	case SD_CARD:
		res = SD_Initialize(hardware);
		if (res) {
			printf("SD init failed: code=0x%02X stage=%u cmd=%u r1=0x%02X "
			       "R7=%02X%02X%02X%02X OCR=%02X%02X%02X%02X\r\n",
			       res, SD_InitStage, SD_InitLastCmd, SD_InitLastR1,
			       SD_InitR7[0], SD_InitR7[1], SD_InitR7[2], SD_InitR7[3],
			       SD_InitOCR[0], SD_InitOCR[1], SD_InitOCR[2], SD_InitOCR[3]);
			SD_SPI_ReadWriteByte(0xff);
		}
		break;
	default:
		res = 1;
	}
	if (res)
		return STA_NOINIT;
	else
		return 0;
}
DSTATUS disk_status(
	BYTE drv /* Physical drive nmuber (0..) */
)
{
	return 0;
}
DRESULT disk_read(
	BYTE drv,	  /* Physical drive nmuber (0..) */
	BYTE *buff,	  /* Data buffer to store read data */
	DWORD sector, /* Sector address (LBA) */
	BYTE count	  /* Number of sectors to read (1..255) */
)
{
	uint8_t res = 0;
	if (!count)
		return RES_PARERR;
	switch (drv)
	{
	case SD_CARD:
		res = SD_ReadDisk(buff, sector, count);
		if (res) {
			printf("SD read failed: sector=%lu count=%u result=0x%02X "
			       "cmd=%u r1=0x%02X token=0x%02X type=0x%02X\r\n",
			       (unsigned long)sector, (unsigned)count, (unsigned)res,
			       (unsigned)SD_InitLastCmd, (unsigned)SD_InitLastR1,
			       (unsigned)SD_LastDataToken, (unsigned)SD_Type);
			SD_SPI_ReadWriteByte(0xff);
		}
		break;
	default:
		res = 1;
	}
	if (res == 0x00)
		return RES_OK;
	else
		return RES_ERROR;
}
#if _READONLY == 0
DRESULT disk_write(
	BYTE drv,		  /* Physical drive nmuber (0..) */
	const BYTE *buff, /* Data to be written */
	DWORD sector,	  /* Sector address (LBA) */
	BYTE count		  /* Number of sectors to write (1..255) */
)
{
	uint8_t res = 0;
	if (!count)
		return RES_PARERR;
	switch (drv)
	{
	case SD_CARD:
		res = SD_WriteDisk((uint8_t *)buff, sector, count);
		break;
	default:
		res = 1;
	}
	if (res == 0x00)
		return RES_OK;
	else
		return RES_ERROR;
}
#endif /* _READONLY */

DRESULT disk_ioctl(
	BYTE drv,  /* Physical drive nmuber (0..) */
	BYTE ctrl, /* Control code */
	void *buff /* Buffer to send/receive control data */
)
{
	DRESULT res;
	if (drv == SD_CARD)
	{
		switch (ctrl)
		{
		case CTRL_SYNC:
			if (SD_WaitReady() == 0)
				res = RES_OK;
			else
				res = RES_ERROR;
			break;
		case GET_SECTOR_SIZE:
			*(WORD *)buff = 512;
			res = RES_OK;
			break;
		case GET_BLOCK_SIZE:
			*(WORD *)buff = 8;
			res = RES_OK;
			break;
		case GET_SECTOR_COUNT:
			*(DWORD *)buff = SD_GetSectorCount();
			res = RES_OK;
			break;
		default:
			res = RES_PARERR;
			break;
		}
	}
	else
		res = RES_ERROR;
	return res;
}

/*-----------------------------------------------------------------------*/
/* User defined function to give a current time to fatfs module          */
/* 31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */
/* 15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */
DWORD get_fattime(void)
{
	return 0;
}
