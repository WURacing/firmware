#include "app_main.h"
#include "drv_i2c.h"
#include "sevenseg.h"
#include "FreeRTOS.h"
#include "task.h"

#include "drv_can.h"
#include "drv_can_private.h"

volatile int GearPosition, EngineSpeed;
volatile int failures;

void app_init(void)
{
	failures = sevenseg_init();
	
	GearPosition = 0;
	EngineSpeed = 1000;
}

void app_periodic(void)
{
	//const uint8_t tx[] = {0x00, 0x66};
	//drv_i2c_write_register(DRV_I2C_CHANNEL_EXPANDERS, 0x25, 0x12, tx, 2);
	
    set_gear(failures);
    //set_rpm(EngineSpeed);
    set_rgb_one_digit(GearPosition, RGB_MAGENTA);
    
	unsigned time = xTaskGetTickCount();
	if (time % 1000 == 0)
	{
	if (++GearPosition > 4)
		GearPosition = 0;
	if (++EngineSpeed > 12000)
		EngineSpeed = 0;
	}
	
//    set_gear(GearPosition);
    //set_rpm(EngineSpeed);
    
    struct drv_can_rx_buffer_element* element = drv_can_get_rx_buffer(DRV_CAN_RX_BUFFER_CAN0_PE6);
    uint16_t coolant_temp_raw = element->DB[4] | (element->DB[5] << 8);
    float coolant_temp = coolant_temp_raw * 0.1;
    
    if(coolant_temp > 200) {
        set_gear(1);
    } else {
        set_gear(0);
    }
    
}
