#include "drv_sd.h"


// QUESTIONS
// What will the SD card be producing as a result of drv_spi_transfer() when it hasn't produced data yet?
// What byte should we send to the SD card while anticipating data, to keep the clock running?
// Why is ACMD two different things in init flowchart?


// Idea: send a command given an enum number, wait for a response, and return the response (1 byte)
// TODO: if we don't get a response in a given number of calls to drv_spi_transfer(), return error, receive response differently 
// based on command (e.g. cmd58 receives 4 bytes) - maybe I don't need to worry about that though?
uint8_t send_cmd(int cmd, int arg) {

    // Send byte with 0 + 1 + command index 
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, (0x7f & cmd) | 0x40);

    // Send 4 bytes in cmd to SD card, one at a time - first byte, then second byte...
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, (arg >> 24) & 0xff);
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, (arg >> 16) & 0xff);
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, (arg >> 8) & 0xff);
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, arg & 0xff);

    // Send dummy CRC (0xff)
    drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xff);
    
    // Keep sending ones to keep clock going until we receive a nonzero 1-byte response, or we exceed attempt limit
    int max = 20;
    uint8_t resp = 0xff;
    do {
        resp = drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xff);
        max--;
    } while (resp == 0xff && max)

    if (!max) return -1;
    
    // Return 1-byte response
    return resp;
}

// pdrv is physical drive number
int disk_initialize (uint8_t pdrv) {
    // Implementation based on SDC/MMC initialization flow diagram here: http://elm-chan.org/docs/mmc/i/sdinit.png

    // Sleep for 1 ms
    sleep(0.001);

    // Send 80 dummy clock ticks (send 10 8-bit data chunks)
    for (int i = 0; i < 10; i++) {
        drv_spi_transfer(DRV_SPI_CHANNEL_SD, 0xff);
    }

    // Send CMD0 (32 bits of 0) until we receive a nonzero result
    uint8_t resp = send_cmd(CMD0);

    // Check if resp is In Idle State bit set (0x01) - if not return error
    if (resp != 0x01) {
        return -1;
    }

    // Send CMD8
    resp = send_cmd(CMD8, 0);

    // If "0x1aa mismatch", return error

    // If "0x1aa match", send ACMD41 until response is 0x00, not 0x01

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
