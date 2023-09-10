#include "drv_ws2812b.h"
#include "sam.h"

struct drv_ws2812b_config drv_ws2812b_config = {
	.pinMap = {
		[DRV_WS2812B_CHANNEL_STATUS_LED] = PIN_PA16,
	}
};