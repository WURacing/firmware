#include "drv_can.h"
#include "sam.h"

#if ENABLE_CAN0
static struct drv_can_standard_filter can0_standard_filters[CAN0_STANDARD_FILTERS_NUM];
static struct drv_can_extended_filter can0_extended_filters[CAN0_EXTENDED_FILTERS_NUM];
static struct drv_can_rx_fifo_0_element can0_rx_fifo_0[CAN0_RX_FIFO_0_NUM];
static struct drv_can_rx_fifo_1_element can0_rx_fifo_1[CAN0_RX_FIFO_1_NUM];
#endif
#if ENABLE_CAN1
static struct drv_can_standard_filter can1_standard_filters[CAN1_STANDARD_FILTERS_NUM];
static struct drv_can_extended_filter can1_extended_filters[CAN1_EXTENDED_FILTERS_NUM];
static struct drv_can_rx_fifo_0_element can1_rx_fifo_0[CAN1_RX_FIFO_0_NUM];
static struct drv_can_rx_fifo_1_element can1_rx_fifo_1[CAN1_RX_FIFO_1_NUM];
#endif
static struct drv_can_rx_buffer_element can_rx_buffers[DRV_CAN_RX_BUFFER_COUNT];
static struct drv_can_tx_buffer_element can_tx_buffers[DRV_CAN_TX_BUFFER_COUNT];

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
#if ENABLE_CAN0
		GCLK->PCHCTRL[CAN0_GCLK_ID] = pclock;
#endif
#if ENABLE_CAN1
		GCLK->PCHCTRL[CAN1_GCLK_ID] = pclock;
#endif
	}
	
	// set up MCLK AHB for CAN (synchronized peripheral clock)
#if ENABLE_CAN0
	MCLK->AHBMASK.bit.CAN0_ = 1;
#endif
#if ENABLE_CAN1
	MCLK->AHBMASK.bit.CAN1_ = 1;
#endif
	
	// set up pin mux for CAN pins
#if ENABLE_CAN0
	PORT->Group[0].PMUX[CAN0_TX_PIN/2].bit.PMUXE = CAN0_TX_MUX;
	PORT->Group[0].PMUX[CAN0_RX_PIN/2].bit.PMUXO = CAN0_RX_MUX;
	PORT->Group[0].PINCFG[CAN0_TX_PIN].bit.PMUXEN = 1;
	PORT->Group[0].PINCFG[CAN0_RX_PIN].bit.PMUXEN = 1;
#endif
#if ENABLE_CAN1
	PORT->Group[1].PMUX[(CAN1_TX_PIN-32)/2].bit.PMUXE = CAN1_TX_MUX;
	PORT->Group[1].PMUX[(CAN1_RX_PIN-32)/2].bit.PMUXO = CAN1_RX_MUX;
	PORT->Group[1].PINCFG[CAN1_TX_PIN-32].bit.PMUXEN = 1;
	PORT->Group[1].PINCFG[CAN1_RX_PIN-32].bit.PMUXEN = 1;
#endif
	
	// Enable interrupt
#if ENABLE_CAN0
	NVIC_EnableIRQ(CAN0_IRQn);
#endif
#if ENABLE_CAN1
	NVIC_EnableIRQ(CAN1_IRQn);
#endif
		
	// put CAN module into configuration mode
#if ENABLE_CAN0
	CAN0->CCCR.bit.INIT = 1;
	while (CAN0->CCCR.bit.INIT != 1) {}; // wait
	CAN0->CCCR.bit.CCE = 1;
	while (CAN0->CCCR.bit.CCE != 1) {}; // wait
#endif
#if ENABLE_CAN1
	CAN1->CCCR.bit.INIT = 1;
	while (CAN1->CCCR.bit.INIT != 1) {}; // wait
	CAN1->CCCR.bit.CCE = 1;
	while (CAN1->CCCR.bit.CCE != 1) {}; // wait
#endif
		
	// copy TX buffer config
	for (enum drv_can_tx_buffer_table i = (enum drv_can_tx_buffer_table) 0; i < DRV_CAN_TX_BUFFER_COUNT; ++i)
	{
		can_tx_buffers[i].TXBE_0 = drv_can_config.transmit_config[i].TXBE_0;
		can_tx_buffers[i].TXBE_1 = drv_can_config.transmit_config[i].TXBE_1;
	}
	
	// copy standard filters from flash to RAM (CAN peripheral can't access flash)
#if ENABLE_CAN0
	for (int i = 0; i < CAN0_STANDARD_FILTERS_NUM; ++i)
	{
		can0_standard_filters[i].SIDFE_0 = drv_can_config.standard_filters_can0[i].SIDFE_0;
	}

	// copy extended filters from flash to RAM
	for (int i = 0; i < CAN0_EXTENDED_FILTERS_NUM; ++i)
	{
		can0_extended_filters[i].XIDFE_0 = drv_can_config.extended_filters_can0[i].XIDFE_0;
		can0_extended_filters[i].XIDFE_1 = drv_can_config.extended_filters_can0[i].XIDFE_1;
	}
#endif
#if ENABLE_CAN1
	for (int i = 0; i < CAN1_STANDARD_FILTERS_NUM; ++i)
	{
		can1_standard_filters[i].SIDFE_0 = drv_can_config.standard_filters_can1[i].SIDFE_0;
	}

	// copy extended filters from flash to RAM
	for (int i = 0; i < CAN1_EXTENDED_FILTERS_NUM; ++i)
	{
		can1_extended_filters[i].XIDFE_0 = drv_can_config.extended_filters_can1[i].XIDFE_0;
		can1_extended_filters[i].XIDFE_1 = drv_can_config.extended_filters_can1[i].XIDFE_1;
	}
#endif

#if ENABLE_CAN0
	{ // Standard filters storage
		CAN_SIDFC_Type sidfc = {
			.bit = {
				.FLSSA = (uint32_t) can0_standard_filters,
				.LSS = CAN0_STANDARD_FILTERS_NUM,
			}
		};
		CAN0->SIDFC = sidfc;
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

#endif
#if ENABLE_CAN1
	{ // Standard filters storage
		CAN_SIDFC_Type sidfc = {
			.bit = {
				.FLSSA = (uint32_t) can1_standard_filters,
				.LSS = CAN1_STANDARD_FILTERS_NUM,
			}
		};
		CAN1->SIDFC = sidfc;
	}
	{ // Extended filters storage
		CAN_XIDFC_Type xidfc = {
			.bit = {
				.FLESA = (uint32_t) can1_extended_filters,
				.LSE = CAN1_EXTENDED_FILTERS_NUM,
			}
		};
		CAN1->XIDFC.reg = xidfc.reg;
	}
	{ // RX FIFO 0 storage
		CAN_RXF0C_Type rxf0c = {
			.bit = {
				.F0SA = (uint32_t) can1_rx_fifo_0,
				.F0S = CAN1_RX_FIFO_0_NUM,
				.F0OM = CAN1_RX_FIFO_0_OPERATION_MODE,
				.F0WM = CAN1_RX_FIFO_0_HIGH_WATER_INT_LEVEL,
			}
		};
		CAN1->RXF0C.reg = rxf0c.reg;
	}
	{ // RX FIFO 1 storage
		CAN_RXF1C_Type rxf1c = {
			.bit = {
				.F1SA = (uint32_t) can1_rx_fifo_1,
				.F1S = CAN1_RX_FIFO_1_NUM,
				.F1OM = CAN1_RX_FIFO_1_OPERATION_MODE,
				.F1WM = CAN1_RX_FIFO_1_HIGH_WATER_INT_LEVEL,
			}
		};
		CAN1->RXF1C.reg = rxf1c.reg;
	}
#endif
	{ // RX element size (for FD mode) configuration
		CAN_RXESC_Type rxesc = {
			.bit = {
				.F0DS = CAN_RX_FIFO_0_DATA_SIZE,
				.F1DS = CAN_RX_FIFO_1_DATA_SIZE,
				.RBDS = CAN_RX_BUFFERS_DATA_SIZE,
			}
		};
#if ENABLE_CAN0
		CAN0->RXESC.reg = rxesc.reg;
#endif
#if ENABLE_CAN1
		CAN1->RXESC.reg = rxesc.reg;
#endif
	}
	{ // RX buffers storage
		CAN_RXBC_Type rxbc = {
			.bit = {
				.RBSA = (uint32_t) can_rx_buffers,
			}
		};
#if ENABLE_CAN0
CAN0->RXBC.reg = rxbc.reg;
#endif
#if ENABLE_CAN1
CAN1->RXBC.reg = rxbc.reg;
#endif
	}
	{ // TX buffers storage
		CAN_TXBC_Type txbc = {
			.bit = {
				.TFQM = 0, /* fifo mode */
				.TFQS = 0, /* but no tx fifos. requires big change to support */
				.NDTB = DRV_CAN_TX_BUFFER_COUNT,
				.TBSA = (uint32_t) can_tx_buffers,
			}
		};
#if ENABLE_CAN0
		CAN0->TXBC.reg = txbc.reg;
#endif
#if ENABLE_CAN1
		CAN1->TXBC.reg = txbc.reg;
#endif
	}
	{ // TX event FIFO storage. disabled
		CAN_TXEFC_Type txefc = {
			.bit = {
				.EFS = 0,
				.EFSA = 0,
				.EFWM = 0,
			}
		};
#if ENABLE_CAN0
		CAN0->TXEFC.reg = txefc.reg;
#endif
#if ENABLE_CAN1
		CAN1->TXEFC.reg = txefc.reg;
#endif
	}
	{ // TX element size (for FD mode) configuration
		CAN_TXESC_Type txesc = {
			.bit = {
				.TBDS = CAN_TX_DATA_SIZE,
			}
		};
#if ENABLE_CAN0
		CAN0->TXESC.reg = txesc.reg;
#endif
#if ENABLE_CAN1
		CAN1->TXESC.reg = txesc.reg;
#endif
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
#if ENABLE_CAN0
		CAN0->TSCC.reg = tscc.reg;
#endif
#if ENABLE_CAN1
		CAN1->TSCC.reg = tscc.reg;
#endif
	}
	{ // enable interrupts. probably want to 
		CAN_IE_Type ie = {
			.bit = {
				.RF1NE = 1, /* interrupt on new FIFO message */
				.RF0NE = 1, /* interrupt on new FIFO message */
				.DRXE  = 1, /* interrupt on new Rx buffer message */
			}
		};
#if ENABLE_CAN0
		CAN0->IE.reg = ie.reg;
#endif
#if ENABLE_CAN1
		CAN1->IE.reg = ie.reg;
#endif
	}
	{ // enable interrupt line 0. this is the default for all interrupts unless changed
		CAN_ILE_Type ile = {
			.bit = {
				.EINT0 = 1,
			}
		};
#if ENABLE_CAN0
		CAN0->ILE.reg = ile.reg;
#endif
#if ENABLE_CAN1
		CAN1->ILE.reg = ile.reg;
#endif
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
#if ENABLE_CAN0
		CAN0->GFC.reg = gfc.reg;
#endif
#if ENABLE_CAN1
		CAN1->GFC.reg = gfc.reg;
#endif
	}
	
	// drop out of configuration mode and start
#if ENABLE_CAN0
	CAN0->CCCR.bit.INIT = 0;
#endif
#if ENABLE_CAN1
	CAN1->CCCR.bit.INIT = 0;
#endif
}

void CAN0_Handler()
{
	CAN0->IR.reg = 0xFFFFFFFF;
}

void CAN1_Handler()
{
	CAN1->IR.reg = 0xFFFFFFFF;
}

struct drv_can_rx_buffer_element * drv_can_get_rx_buffer(int id)
{
	if (id < DRV_CAN_RX_BUFFER_COUNT)
	{
		return &can_rx_buffers[id];
	}
	else
	{
		return NULL;
	}
}

struct drv_can_tx_buffer_element * drv_can_get_tx_buffer(int id)
{
	if (id < DRV_CAN_TX_BUFFER_COUNT)
	{
		return &can_tx_buffers[id];
	}
	else
	{
		return NULL;
	}
}

void drv_can_queue_tx_buffer(Can * bus, int id)
{
	if (id < DRV_CAN_TX_BUFFER_COUNT)
	{
		bus->TXBAR.reg = (1 << id);
	}
}

bool drv_can_check_rx_buffer(Can * bus, int id)
{
	if (id < DRV_CAN_RX_BUFFER_COUNT)
	{
		if (id < 32)
		{
			return (bus->NDAT1.reg >> id) & 1;
		}
		else
		{
			return (bus->NDAT2.reg >> (id - 32)) & 1;
		}
	}
	else
	{
		return false;
	}
}

void drv_can_clear_rx_buffer(Can * bus, int id)
{
	if (id < DRV_CAN_RX_BUFFER_COUNT)
	{
		if (id < 32)
		{
			bus->NDAT1.reg = (1 << id);
		}
		else
		{
			bus->NDAT2.reg = (1 << (id - 32));
		}
	}
}