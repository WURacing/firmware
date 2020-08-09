#include "drv_can.h"
#include "sam.h"

static struct drv_can_standard_filter can0_standard_filters[CAN0_STANDARD_FILTERS_NUM];
static struct drv_can_extended_filter can0_extended_filters[CAN0_EXTENDED_FILTERS_NUM];
static struct drv_can_rx_fifo_0_element can0_rx_fifo_0[CAN0_RX_FIFO_0_NUM];
static struct drv_can_rx_fifo_1_element can0_rx_fifo_1[CAN0_RX_FIFO_1_NUM];
static struct drv_can_rx_buffer_element can0_rx_buffers[CAN0_RX_BUFFERS_NUM];
static CanMramTxefe can0_tx_event_fifo[CAN0_TX_EVENT_FIFO_NUM];
static struct drv_can_tx_buffer_element can0_tx_buffers[CAN0_TX_BUFFERS_NUM + CAN0_TX_FIFO_NUM];

void drv_can_init(void)
{
	{ // set up CAN supplying clock generator at 8MHz
		GCLK_GENCTRL_Type clock = {
			.bit = {
				.GENEN = 1,
				.SRC = GCLK_GENCTRL_SRC_OSC48M_Val,
				.DIV = 6,
				.DIVSEL = 0,
				.RUNSTDBY = 1
			}
		};
		GCLK->GENCTRL[8] = clock;
	}
	
	// sync
	while (GCLK->SYNCBUSY.bit.GENCTRL8) {};
	
	{ // set up CAN peripheral clocks
		GCLK_PCHCTRL_Type pclock = {
			.bit = {
				.CHEN = 1,
				.GEN = 8
			}
		};
		GCLK->PCHCTRL[CAN0_GCLK_ID] = pclock;
#ifdef CAN1
		GCLK->PCHCTRL[CAN1_GCLK_ID] = pclock;
#endif
	}
	
	// set up MCLK AHB for CAN (synchronized peripheral clock)
	MCLK->AHBMASK.bit.CAN0_ = 1;
#ifdef CAN1
	MCLK->AHBMASK.bit.CAN1_ = 1;
#endif
	
	// set up pin mux for CAN pins
	PORT->Group[0].PMUX[PIN_PA24/2].bit.PMUXE = MUX_PA24G_CAN0_TX;
	PORT->Group[0].PMUX[PIN_PA25/2].bit.PMUXO = MUX_PA25G_CAN0_RX;
	PORT->Group[0].PINCFG[PIN_PA24].bit.PMUXEN = 1;
	PORT->Group[0].PINCFG[PIN_PA25].bit.PMUXEN = 1;
	
	// TODO: set up NVIC interrupts
	
	// put CAN module into configuration mode
	CAN0->CCCR.bit.INIT = 1;
	while (CAN0->CCCR.bit.INIT != 1) {}; // wait
	CAN0->CCCR.bit.CCE = 1;
	while (CAN0->CCCR.bit.CCE != 1) {}; // wait
		
	// copy TX buffer config
	for (enum drv_can_tx_buffer_table i = (enum drv_can_tx_buffer_table) 0; i < DRV_CAN_TX_BUFFER_COUNT; ++i)
	{
		can0_tx_buffers[i].TXBE_0 = drv_can_config.transmit_config[i].TXBE_0;
		can0_tx_buffers[i].TXBE_1 = drv_can_config.transmit_config[i].TXBE_1;
	}
	
	// copy standard filters from flash to RAM (CAN peripheral can't access flash)
	for (int i = 0; i < CAN0_STANDARD_FILTERS_NUM; ++i)
	{
		can0_standard_filters[i].SIDFE_0 = drv_can_config.standard_filters[i].SIDFE_0;
	}

	// copy extended filters from flash to RAM
	for (int i = 0; i < CAN0_EXTENDED_FILTERS_NUM; ++i)
	{
		can0_extended_filters[i].XIDFE_0 = drv_can_config.extended_filters[i].XIDFE_0;
		can0_extended_filters[i].XIDFE_1 = drv_can_config.extended_filters[i].XIDFE_1;
	}
	
	{ // Standard filters storage
		CAN_SIDFC_Type sidfc = {
			.bit = {
				.FLSSA = (uint32_t) can0_standard_filters,
				.LSS = CAN0_STANDARD_FILTERS_NUM,
			}
		};
		CAN0->SIDFC.reg = sidfc.reg;
	}
	{ // Extended filters storage
		CAN_XIDFC_Type xidfc = {
			.bit = {
				.FLESA = (uint32_t) can0_extended_filters,
				.LSE = CAN0_EXTENDED_FILTERS_NUM,
			}
		};
		CAN0->XIDFC.reg = xidfc.reg;
	}
	{ // RX FIFO 0 storage
		CAN_RXF0C_Type rxf0c = {
			.bit = {
				.F0SA = (uint32_t) can0_rx_fifo_0,
				.F0S = CAN0_RX_FIFO_0_NUM,
				.F0OM = CAN0_RX_FIFO_0_OPERATION_MODE,
				.F0WM = CAN0_RX_FIFO_0_HIGH_WATER_INT_LEVEL,
			}
		};
		CAN0->RXF0C.reg = rxf0c.reg;
	}
	{ // RX FIFO 1 storage
		CAN_RXF1C_Type rxf1c = {
			.bit = {
				.F1SA = (uint32_t) can0_rx_fifo_1,
				.F1S = CAN0_RX_FIFO_1_NUM,
				.F1OM = CAN0_RX_FIFO_1_OPERATION_MODE,
				.F1WM = CAN0_RX_FIFO_1_HIGH_WATER_INT_LEVEL,
			}
		};
		CAN0->RXF1C.reg = rxf1c.reg;
	}
	{ // RX buffers storage
		CAN_RXBC_Type rxbc = {
			.bit = {
				.RBSA = (uint32_t) can0_rx_buffers,
			}
		};
		CAN0->RXBC.reg = rxbc.reg;
	}
	{ // RX element size (for FD mode) configuration
		CAN_RXESC_Type rxesc = {
			.bit = {
				.F0DS = CAN0_RX_FIFO_0_DATA_SIZE,
				.F1DS = CAN0_RX_FIFO_1_DATA_SIZE,
				.RBDS = CAN0_RX_BUFFERS_DATA_SIZE,
			}
		};
		CAN0->RXESC.reg = rxesc.reg;
	}
	{ // TX buffers storage
		CAN_TXBC_Type txbc = {
			.bit = {
				.TFQM = 0, /* fifo mode */
				.TFQS = CAN0_TX_FIFO_NUM,
				.NDTB = CAN0_TX_BUFFERS_NUM,
				.TBSA = (uint32_t) can0_tx_buffers,
			}
		};
		CAN0->TXBC.reg = txbc.reg;
	}
	{ // TX event FIFO storage
		CAN_TXEFC_Type txefc = {
			.bit = {
				.EFS = CAN0_TX_EVENT_FIFO_NUM,
				.EFSA = (uint32_t) can0_tx_event_fifo,
				.EFWM = CAN0_TX_EVENT_FIFO_HIGH_WATER_INT_LEVEL,
			}
		};
		CAN0->TXEFC.reg = txefc.reg;
	}
	{ // TX element size (for FD mode) configuration
		CAN_TXESC_Type txesc = {
			.bit = {
				.TBDS = CAN_TXESC_TBDS_DATA8_Val,
			}
		};
		CAN0->TXESC.reg = txesc.reg;
	}
	{ // bit rate timing (currently unused)
		// With a GCLK_CAN of 8MHz, the reset value 0x00000A33 configures the CAN for a fast bit rate of 500 kBits/s.
		//CAN_DBTP_Type dbtp;
	}
	{ // nominal bit rate timing and prescaler
		// With a CAN clock (GCLK_CAN) of 8MHz, the reset value 0x06000A03 configures the CAN for a bit rate of 500 kBits/s.
		//CAN_NBTP_Type nbtp;
	}
	{ // timestamping. we should reset TSCV every ms so we know correct microsecond timing or something like that
		CAN_TSCC_Type tscc = {
			.bit = {
				.TCP = 0, /* prescaler = 1 (times the CAN bit time) */
				.TSS = 1, /* enable tscc counter */
			}
		};
		CAN0->TSCC.reg = tscc.reg;
	}
	{ // enable interrupts. probably want to 
		CAN_IE_Type ie = {
			.bit = {
				.RF1NE = 1, /* interrupt on new FIFO message */
				.RF0NE = 1, /* interrupt on new FIFO message */
			}
		};
		CAN0->IE.reg = ie.reg;
	}
	{ // enable interrupt line 0. this is the default for all interrupts unless changed
		CAN_ILE_Type ile = {
			.bit = {
				.EINT0 = 1,
			}
		};
		CAN0->ILE.reg = ile.reg;
	}
	{ // general filter configuration
		CAN_GFC_Type gfc = {
			.bit = {
				.ANFS = 0, /* accept nonmatching standard in FIFO 0 */
				.ANFE = 0, /* accept nonmatching extended in FIFO 0 */
				.RRFS = 1, /* reject standard remote frames */
				.RRFE = 1, /* reject extended remote frames */
			}
		};
		CAN0->GFC.reg = gfc.reg;
	}
	
	// drop out of configuration mode and start
	CAN0->CCCR.bit.INIT = 0;
}

void CAN0_Handler()
{
	
}

void CAN1_Handler()
{
	
}

struct drv_can_rx_buffer_element * drv_can_get_rx_buffer(int id)
{
	if (id < CAN0_RX_BUFFERS_NUM)
	{
		return &can0_rx_buffers[id];
	}
	else
	{
		return NULL;
	}
}

struct drv_can_tx_buffer_element * drv_can_get_tx_buffer(int id)
{
	if (id < CAN0_TX_BUFFERS_NUM)
	{
		return &can0_tx_buffers[id];
	}
	else
	{
		return NULL;
	}
}

void drv_can_queue_tx_buffer(int id)
{
	if (id < CAN0_TX_BUFFERS_NUM)
	{
		CAN0->TXBAR.reg = (1 << id);
	}
}

bool drv_can_check_rx_buffer(int id)
{
	if (id < CAN0_RX_BUFFERS_NUM)
	{
		if (id < 32)
		{
			return (CAN0->NDAT1.reg >> id) & 1;
		}
		else
		{
			return (CAN0->NDAT2.reg >> (id - 32)) & 1;
		}
	}
	else
	{
		return false;
	}
}