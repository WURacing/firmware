#include "drv_sd.h"
#include "FreeRTOS.h"
#include "task.h"


// QUESTIONS
// What will the SD card be producing as a result of drv_spi_transfer() when it hasn't produced data yet?
// What byte should we send to the SD card while anticipating data, to keep the clock running?
// Why is ACMD two different things in init flowchart?


// Idea: send a command given an enum number, wait for a response, and return the response (1 byte)
// TODO: if we don't get a response in a given number of calls to drv_spi_transfer(), return error, receive response differently 
// based on command (e.g. cmd58 receives 4 bytes) - maybe I don't need to worry about that though?

uint8_t send_cmd(int cmd, int arg, uint8_t crc)
{

	// Send byte with 0 + 1 + command index 
	drv_spi_transfer(DRV_SPI_CHANNEL_SD, (0x7f & cmd) | 0x40);

	// Send 4 bytes in cmd to SD card, one at a time - first byte, then second byte...
	drv_spi_transfer(DRV_SPI_CHANNEL_SD, (arg >> 24) & 0xff);
	drv_spi_transfer(DRV_SPI_CHANNEL_SD, (arg >> 16) & 0xff);
	drv_spi_transfer(DRV_SPI_CHANNEL_SD, (arg >> 8) & 0xff);
	drv_spi_transfer(DRV_SPI_CHANNEL_SD, arg & 0xff);
	drv_spi_transfer(DRV_SPI_CHANNEL_SD, crc);

	// Keep sending ones to keep clock going until we receive a nonzero 1-byte response, or we exceed attempt limit
	int max = SD_TIMEOUT_BYTES;
	uint8_t resp = 0xff;
	do
	{
		resp = drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xff);
		max--;
	}
	while (resp == 0xff && max);

	// Return 1-byte response
	return resp;
}


// pdrv is physical drive number

DSTATUS disk_initialize(BYTE pdrv)
{
	// Implementation based on SDC/MMC initialization flow diagram here: http://elm-chan.org/docs/mmc/i/sdinit.png
	if (pdrv != 0)
	{
		return STA_NOINIT;
	}

	//SETUP****************************************************************************************
	//Set CS high during 74+ clock pulses
	PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA20;

	// Sleep for at least 1 ms
	vTaskDelay(3);

	//SOFTWARE RESET*******************************************************************************
	// Send 80 dummy clock ticks (send 10 8-bit data chunks). Also sets DI high.
	for (int i = 0; i < 10; i++)
	{
		drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xff);
	}

	// Set CS low and send CMD0 (32 bits of 0) until we receive a nonzero result
	PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA20;
	uint8_t resp = 0;

	do
	{
		resp = send_cmd(CMD0, 0x0, 0x95);
	}
	while (resp == 0);

	// Check if resp is In Idle State bit set (0x01) - if not return error
	if (resp != 0x01)
	{
		return STA_NOINIT;
	}

	//INITIALIZATION*******************************************************************************
	// Send CMD8 and check response
	resp = send_cmd(CMD8, 0x1aa, 0x87);
	if (resp != 0x01)
	{
		return STA_NOINIT;
	}

	int success = 0;

	while (!success)
	{
		// Send CMD55 - if response is 0x01, good; if response is 0x05, retry w/ CMD1 instead

		do
		{
			resp = send_cmd(CMD55, 0x0, 0x65);
		}
		while ((resp != 0x01) && (resp != 0x05));

		if (resp == 0x05)
		{
			// Old card (ACMD command rejected) - try CMD1
			resp = send_cmd(CMD1, 0x0, 0xf9);
		}

		// Send ACMD41 - if 0x0, we're done; if 0x01 or 0x05, repeat loop; else, return error
		resp = send_cmd(ACMD41, 0x40000000, 0xff);
		if (resp == 0x0)
		{
			success = 1;
		}

	}

	//CMD16 will set block size to 512 bytes to work with FatFS

	return resp;
}

// pdrv: physical drive number (0)
// buff: [out] pointer to read data buffer
// sector: start sector number
// count: number of sectors to read

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
	uint8_t resp;
	int read_attempts;
	
	if (pdrv != 0 || count < 1 || count > SD_BIGGEST_SECTOR || sector > SD_BIGGEST_SECTOR || buff == 0)
	{
		return RES_PARERR;
	}

	// Get initial 1-byte response from SD card
	if (count == 1)
	{
		resp = send_cmd(CMD17, sector, 0);
	}
	else
	{
		resp = send_cmd(CMD18, sector, 0);
	}
	if (resp == 0xff)
	{
		return RES_NOTRDY;
	}

	for (UINT j = 0; j < count; j++)
	{
		read_attempts = resp = 0;
		while (resp != SD_RECEIVE_START && read_attempts < SD_TIMEOUT_BYTES)
		{
			resp = drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xff);
			read_attempts++;
		}
		// If response token is 0xfe, the next byte will contain data!!
		if (resp == SD_RECEIVE_START)
		{
			for (int i = 0; i < SD_SECTOR_SIZE; i++)
			{
				buff[i + SD_SECTOR_SIZE * j] = drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xff);
			}
		}
		else
		{
			resp = send_cmd(CMD12, 0, 0);
			return RES_ERROR;
		}
	}
	
	if (count > 1)
	{
		// Send CMD12 to stop reading data
		resp = send_cmd(CMD12, 0, 0);
	}
	
	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
	uint8_t resp;
	
	if (pdrv != 0 || count < 1 || count > SD_BIGGEST_SECTOR || sector > SD_BIGGEST_SECTOR || buff == 0)
	{
		return RES_PARERR;
	}
	
	// Get initial 1-byte response from SD card
	if (count == 1)
	{
		resp = send_cmd(CMD24, sector, 0);
	}
	else
	{
		resp = send_cmd(CMD25, sector, 0);
	}
	if (resp == 0xff)
	{
		return RES_NOTRDY;
	}

	for (UINT j = 0; j < count; j++)
	{
		// Send one 0xff byte to prep for sending data block
		drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xff);

		// Send data token (0xfe for CMD24, 0xfc for CMD25)
		if (count == 1) drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xfe);
		else drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xfc);

		for (int i = 0; i < SD_SECTOR_SIZE; i++)
		{
			drv_spi_transfer(DRV_SPI_CHANNEL_SD, buff[i + SD_SECTOR_SIZE * j]);
		}
	}
	
	if (count > 1)
	{
		// Send Stop Tran token to stop reading data
		drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xfd);
	}
	
	return RES_OK;
	
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{

}

DSTATUS disk_status(BYTE pdrv)
{

}
