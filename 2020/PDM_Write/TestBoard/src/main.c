/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include "util.h"
#include "CANBus19.h"

void config_pins(void)
{
    struct port_config config_port_pin;
    port_get_config_defaults(&config_port_pin);
    config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(LED_0_PIN, &config_port_pin);
    port_pin_set_config(LED_1_PIN, &config_port_pin);

}

void shift_array_left(char *arr, const size_t size, const size_t bits)
{
    const size_t chunks = bits / (8);

    if (chunks >= size)
    {
        return;
    }

    if (chunks)
    {
        memmove(arr, arr + chunks, size - chunks);
    }

    const size_t left = bits % (8);

    // If we have non directly addressable bits left we need to move the whole thing one by one.
    if (left)
    {
        const size_t right = (8) - left;
        const size_t l = size - chunks - 1;
        for (size_t i = 0; i < l; i++)
        {
            arr[i] = ((arr[i] << left) & ~left) | (arr[i + 1] >> right);
        }
        arr[l] = (arr[l] << left) & ~left;
    }
}

int main(void)
{
    system_init();
    delay_init();

    /*
	CAN
	*/
    configure_can();
	configure_i2c();
	volatile int conftmp = configure_tmp75a();

    irq_initialize_vectors();
    cpu_irq_enable();

    delay_ms(1000);
	
	#define TMP_PKT_ID 0x1f00110

    char test_file_name[50];
    Ctrl_status status;
    FRESULT res;
    FATFS fs;
    FIL file_object;
    FILINFO file_stat;
    char line[2200];
    int buf_len = 0;
    int logno = 0;
    int onetwentyeighths = 0;

    sd_mmc_init();

    /* CONFIG THE PINS */
    config_pins();

    while (1)
    {
        //printf("Please plug an SD, MMC or SDIO card in slot.\n\r");

        /* Wait card present and ready */
        do
        {
            status = sd_mmc_test_unit_ready(0);

            if (CTRL_FAIL == status)
            {
                //printf("Failed to initialize SD card [%d], please re-insert the card.\r\n", status);
                //while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
                //}
            }

        } while (CTRL_GOOD != status);

        port_pin_set_output_level(LED_0_PIN, 1);

        memset(&fs, 0, sizeof(FATFS));
        res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
        if (FR_INVALID_DRIVE == res)
        {
            //printf("Failed to mount FAT32 filesystem on SD card [%d], please check that\r\n", res);
            goto main_end_of_test;
        }

        do
        {
            sprintf(test_file_name, "0:LOG%05d.CSV", logno++);
            bzero(&file_stat, sizeof(file_stat));
            res = f_stat(test_file_name, &file_stat);
        } while (res == FR_OK);

        if (res != FR_NO_FILE)
        {
            //printf("Failed to find new file on card [%d]\r\n", res);
            goto main_end_of_test;
        }

        test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
        res = f_open(&file_object, (char const *)test_file_name, FA_CREATE_ALWAYS | FA_WRITE);
        if (res != FR_OK)
        {
            //printf("Failed to create file on card [%d]\r\n", res);
            goto main_end_of_test;
        }

        //printf("Starting data logging...\r\n");
        res = f_puts("year,month,day,hour,min,sec,ms,id,data\n", &file_object);
        if (res == -1)
            goto sd_cleanup;

        while (1)
        {

            system_interrupt_enable_global();

            // Main l00p
            //port_pin_set_output_level(LED_0_PIN, 1);
            int led_0_state = 0;
            int led_1_state = 0;
            int canline_ptr = 0;

            port_pin_set_output_level(LED_1_PIN, 1);

			int count = 0;
			uint16_t tmp_data[4];

            // MAIN LOOP

            while (1)
            {
                if (buf_len > 2048)
                {
                    f_write(&file_object, line, 2048, NULL);
                    buf_len -= 2048;

                    f_sync(&file_object);

                    shift_array_left(line, 2048, 2200 * 8);
                }

                if (canline_0_updated)
                {

                    volatile can_message_t *cl = canline + (canline_ptr++);

                    buf_len += sprintf(line + buf_len, "%08lx,%02x%02x%02x%02x%02x%02x%02x%02x\n",
                                       cl->id,
                                       cl->data.arr[0], cl->data.arr[1], cl->data.arr[2], cl->data.arr[3],
                                       cl->data.arr[4], cl->data.arr[5], cl->data.arr[6], cl->data.arr[7]);

                    port_pin_set_output_level(LED_0_PIN, led_0_state);
                    led_0_state = !led_0_state;

                    canline_0_updated = 0;
                    if (--canline_i == 0)
                        canline_ptr = 0;
                }
                if (canline_1_updated)
                {
                    volatile can_message_t *cl = canline + (canline_ptr++);

                    buf_len += sprintf(line + buf_len, "%08lx,%02x%02x%02x%02x%02x%02x%02x%02x\n",
                                       cl->id,
                                       cl->data.arr[0], cl->data.arr[1], cl->data.arr[2], cl->data.arr[3],
                                       cl->data.arr[4], cl->data.arr[5], cl->data.arr[6], cl->data.arr[7]);

                    port_pin_set_output_level(LED_0_PIN, led_0_state);
                    led_0_state = !led_0_state;

                    canline_1_updated = 0;
                    //if (--canline_i == 0)
                    canline_ptr = 0;
                }
				
				if (count % 100 == 0) {
					volatile uint16_t tmp = read_tmp75a();
					
					tmp_data[0] = tmp_data[1] = tmp_data[2] = 0;
					
					// CONVERSION = (x >> 4) * 0.0625 
					tmp_data[3] = tmp >> 4;
					
					buf_len += sprintf(line + buf_len, "%08lx,%04x%04x%04x%04x\n",
					TMP_PKT_ID, tmp_data[0], tmp_data[1], tmp_data[2], tmp_data[3]);
					
					
					port_pin_set_output_level(LED_1_PIN, led_1_state);
					led_1_state = !led_1_state;
					
				}
				
				
				count++;
			
			}
        }

    sd_cleanup:
        f_close(&file_object);

        port_pin_set_output_level(LED_1_PIN, 1);
        port_pin_set_output_level(LED_0_PIN, 1);
        delay_ms(1000);

    main_end_of_test:
        //printf("Please unplug the card.\r\n");
        port_pin_set_output_level(LED_0_PIN, 0);
        while (CTRL_NO_PRESENT != sd_mmc_check(0))
        {
        }
    }
}
