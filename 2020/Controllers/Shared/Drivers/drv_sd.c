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
uint8_t send_cmd(int cmd, int arg, uint8_t crc) {

    // Send byte with 0 + 1 + command index 
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, (0x7f & cmd) | 0x40);

    // Send 4 bytes in cmd to SD card, one at a time - first byte, then second byte...
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, (arg >> 24) & 0xff);
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, (arg >> 16) & 0xff);
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, (arg >> 8) & 0xff);
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, arg & 0xff);
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, crc);
    
    // Keep sending ones to keep clock going until we receive a nonzero 1-byte response, or we exceed attempt limit
    int max = 1000;
    uint8_t resp = 0xff;
    do {
        resp = drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xff);
        max--;
    } while (resp == 0xff && max);

    if (!max) {
		return 0x69;
	}
    
    // Return 1-byte response
    return resp;
}

// pdrv is physical drive number
int disk_initialize (uint8_t pdrv) {
    // Implementation based on SDC/MMC initialization flow diagram here: http://elm-chan.org/docs/mmc/i/sdinit.png
	
	//SETUP****************************************************************************************
	//Set CS high during 74+ clock pulses
	PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA20;

    // Sleep for at least 1 ms
    vTaskDelay(3);
	
	//SOFTWARE RESET*******************************************************************************
    // Send 80 dummy clock ticks (send 10 8-bit data chunks). Also sets DI high.
    for (int i = 0; i < 10; i++) {
        drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xff);
    }

    // Set CS low and send CMD0 (32 bits of 0) until we receive a nonzero result
	PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA20;
    uint8_t resp = 0;
	
	do {
		resp = send_cmd(CMD0, 0x0, 0x95);
	} while (resp == 0);

    // Check if resp is In Idle State bit set (0x01) - if not return error
    if (resp != 0x01) {
        return -1;
    }

	//INITIALIZATION*******************************************************************************
    // Send CMD8 and check response
    resp = send_cmd(CMD8, 0x1aa, 0x87);
	if (resp != 0x01) {
        return -1;
    }
	
	int success = 0;
	
	while (!success) {
		// Send CMD55 - if response is 0x01, good; if response is 0x05, retry w/ CMD1 instead
		
		do {
			resp = send_cmd(CMD55, 0x0, 0x65); // response here is 0x69, so we return a few lines later. what's going on TODO. We're maxing out :(
		} while ((resp != 0x01) && (resp != 0x05));
		
		if (resp == 0x05) {
			// Old card (ACMD command rejected) - try CMD1
			resp = send_cmd(CMD1, 0x0, 0xf9);
		} 

//		if (resp != 0x01) { //we get to here
//			return -1;
//		}

		// Send ACMD41 - if 0x0, we're done; if 0x01 or 0x05, repeat loop; else, return error
		resp = send_cmd(ACMD41, 0x40000000, 0xff);
		if (resp == 0x0) {
			success = 1;
		} 
//		else if (!(resp == 0x05 || resp == 0x01)) {
//			return -1;
//		}
	
	}

    return resp;

}

int disk_read (uint8_t pdrv, uint8_t* buff, LBA_t sector, uint8_t count) {

}

int disk_write (uint8_t pdrv, const uint8_t* buff, LBA_t sector, uint8_t count) {

}

int disk_ioctl (uint8_t pdrv, uint8_t cmd, void* buff) {

}

int disk_status (uint8_t pdrv) {

}
