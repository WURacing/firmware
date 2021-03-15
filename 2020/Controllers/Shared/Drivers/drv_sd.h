/* 
 * File:   drv_sd.h
 * Author: Shannon
 *
 * Purpose: Implements SD card communication over SPI interface.  Used by FatFs.
 */
 
#ifndef DRV_SD_H
#define	DRV_SD_H

#include <stdint.h>
#include <unistd.h>
#include "drv_spi.h"


#include "ff.h"
#include "diskio.h"

#define SD_SECTOR_SIZE 512
#define SD_TIMEOUT_BYTES 4000
#define SD_RECEIVE_START 0xfe
#define SD_BIGGEST_SECTOR (UINT)(256000000000/SD_SECTOR_SIZE)


// Tying command indices to numbers 
enum cmd_ind {
    CMD0 =   0,  // Software reset
    CMD1 =   1,  // Initiate initialization process
    ACMD41 = 41,  // For only SDC. Initiate initialization process
    CMD8 =   8,  // For only SDC V2. Check voltage range
    CMD9 = 9,           // Read CSD register
    //CMD10 = 10,         // Read CID register
    CMD12 =  12,         // Stop to read data
    CMD16 =  16,  // Change R/W block size
    CMD17 =  17,         // Read block
    CMD18 =  18,         // Read multiple blocks
    //CMD23 = 23,         // DON'T USE - for MMC only. Define number of blocks for next R/W command
    //ACMD23 = 23,        // For only SDC. Define number of blocks to pre-erase with next multi-block write command
    CMD24 = 24,         // Write a block
    CMD25 = 25,         // Write multiple blocks
	CMD32 = 32,		// erase start addr
	CMD33 = 33,		// erase end addr
	CMD38 = 38,		// start erase
    CMD55 = 55,         // Leading command of ACMD<n> command
    CMD58 = 58    // Read OCR
};

void drv_sd_init(void);
void drv_sd_periodic(void);


#endif