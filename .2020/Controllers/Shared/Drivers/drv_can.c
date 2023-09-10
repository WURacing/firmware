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
	// set up CAN supplying clock generator at 8MHz
	GCLK_REGS->GCLK_GENCTRL[8] =
			GCLK_GENCTRL_GENEN(1) | GCLK_GENCTRL_SRC_OSC48M | GCLK_GENCTRL_DIV(6) |
			GCLK_GENCTRL_DIVSEL(0) | GCLK_GENCTRL_RUNSTDBY(1);
	// sync
	while (GCLK_REGS->GCLK_SYNCBUSY) {}
	
#if ENABLE_CAN0
	GCLK_REGS->GCLK_PCHCTRL[26] = GCLK_PCHCTRL_CHEN(1) | GCLK_PCHCTRL_GEN_GCLK8;
#endif
#if ENABLE_CAN1
	GCLK_REGS->GCLK_PCHCTRL[27] = GCLK_PCHCTRL_CHEN(1) | GCLK_PCHCTRL_GEN_GCLK8;
#endif
	
	// set up MCLK AHB for CAN (synchronized peripheral clock)
#if ENABLE_CAN0
	MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_CAN0(1);
#endif
#if ENABLE_CAN1
	MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_CAN1(1);
#endif
	
	// set up pin mux for CAN pins
#if ENABLE_CAN0
	PORT_REGS->GROUP[0].PORT_PMUX[(CAN0_TX_PIN%32)/2] |= PORT_PMUX_PMUXE(CAN0_TX_MUX);
	PORT_REGS->GROUP[0].PORT_PMUX[(CAN0_RX_PIN%32)/2] |= PORT_PMUX_PMUXO(CAN0_RX_MUX);
	PORT_REGS->GROUP[0].PORT_PINCFG[CAN0_TX_PIN%32] |= PORT_PINCFG_PMUXEN(1);
	PORT_REGS->GROUP[0].PORT_PINCFG[CAN0_RX_PIN%32] |= PORT_PINCFG_PMUXEN(1);
#endif
#if ENABLE_CAN1
	PORT_REGS->GROUP[1].PORT_PMUX[(CAN1_TX_PIN-32)/2] |= PORT_PMUX_PMUXE(CAN1_TX_MUX);
	PORT_REGS->GROUP[1].PORT_PMUX[(CAN1_RX_PIN-32)/2] |= PORT_PMUX_PMUXO(CAN1_RX_MUX);
	PORT_REGS->GROUP[1].PORT_PINCFG[CAN1_TX_PIN-32] |= PORT_PINCFG_PMUXEN(1);
	PORT_REGS->GROUP[1].PORT_PINCFG[CAN1_RX_PIN-32] |= PORT_PINCFG_PMUXEN(1);
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
	CAN0_REGS->CAN_CCCR = CAN_CCCR_INIT(1);
	while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) == 0) {} // wait
	CAN0_REGS->CAN_CCCR = CAN_CCCR_INIT(1) | CAN_CCCR_CCE(1);
	while ((CAN0_REGS->CAN_CCCR & CAN_CCCR_CCE_Msk) == 0) {}
#endif
#if ENABLE_CAN1
	CAN1_REGS->CAN_CCCR = CAN_CCCR_INIT(1);
	while ((CAN1_REGS->CAN_CCCR & CAN_CCCR_INIT_Msk) == 0) {} // wait
	CAN1_REGS->CAN_CCCR = CAN_CCCR_INIT(1) | CAN_CCCR_CCE(1);
	while ((CAN1_REGS->CAN_CCCR & CAN_CCCR_CCE_Msk) == 0) {}
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
	// Standard filters storage
	CAN0_REGS->CAN_SIDFC = CAN_SIDFC_FLSSA((uint32_t) can0_standard_filters) | CAN_SIDFC_LSS(CAN0_STANDARD_FILTERS_NUM);
	// Extended filters storage
	CAN0_REGS->CAN_XIDFC = CAN_XIDFC_FLESA((uint32_t) can0_extended_filters) | CAN_XIDFC_LSE(CAN0_EXTENDED_FILTERS_NUM);
	// RX FIFO 0 storage
	CAN0_REGS->CAN_RXF0C = CAN_RXF0C_F0SA((uint32_t) can0_rx_fifo_0) | CAN_RXF0C_F0S(CAN0_RX_FIFO_0_NUM) |
			CAN_RXF0C_F0OM(CAN0_RX_FIFO_0_OPERATION_MODE) | CAN_RXF0C_F0WM(CAN0_RX_FIFO_0_HIGH_WATER_INT_LEVEL);
	// RX FIFO 1 storage
	CAN0_REGS->CAN_RXF1C = CAN_RXF1C_F1SA((uint32_t) can0_rx_fifo_1) | CAN_RXF1C_F1S(CAN0_RX_FIFO_1_NUM) |
			CAN_RXF1C_F1OM(CAN0_RX_FIFO_1_OPERATION_MODE) | CAN_RXF1C_F1WM(CAN0_RX_FIFO_1_HIGH_WATER_INT_LEVEL);
#endif
#if ENABLE_CAN1
	// Standard filters storage
	CAN1_REGS->CAN_SIDFC = CAN_SIDFC_FLSSA((uint32_t) can1_standard_filters) | CAN_SIDFC_LSS(CAN1_STANDARD_FILTERS_NUM);
	// Extended filters storage
	CAN1_REGS->CAN_XIDFC = CAN_XIDFC_FLESA((uint32_t) can1_extended_filters) | CAN_XIDFC_LSE(CAN1_EXTENDED_FILTERS_NUM);
	// RX FIFO 0 storage
	CAN1_REGS->CAN_RXF0C = CAN_RXF0C_F0SA((uint32_t) can1_rx_fifo_0) | CAN_RXF0C_F0S(CAN1_RX_FIFO_0_NUM) |
			CAN_RXF0C_F0OM(CAN1_RX_FIFO_0_OPERATION_MODE) | CAN_RXF0C_F0WM(CAN1_RX_FIFO_0_HIGH_WATER_INT_LEVEL);
	// RX FIFO 1 storage
	CAN1_REGS->CAN_RXF1C = CAN_RXF1C_F1SA((uint32_t) can1_rx_fifo_1) | CAN_RXF1C_F1S(CAN1_RX_FIFO_1_NUM) |
			CAN_RXF1C_F1OM(CAN1_RX_FIFO_1_OPERATION_MODE) | CAN_RXF1C_F1WM(CAN1_RX_FIFO_1_HIGH_WATER_INT_LEVEL);
#endif
#if ENABLE_CAN0
	// RX element size (for FD mode) configuration
	CAN0_REGS->CAN_RXESC = CAN_RXESC_F0DS(CAN_RX_FIFO_0_DATA_SIZE) |
			CAN_RXESC_F1DS(CAN_RX_FIFO_1_DATA_SIZE) | CAN_RXESC_RBDS(CAN_RX_BUFFERS_DATA_SIZE);
#endif
#if ENABLE_CAN1
	// RX element size (for FD mode) configuration
	CAN1_REGS->CAN_RXESC = CAN_RXESC_F0DS(CAN_RX_FIFO_0_DATA_SIZE) |
			CAN_RXESC_F1DS(CAN_RX_FIFO_1_DATA_SIZE) | CAN_RXESC_RBDS(CAN_RX_BUFFERS_DATA_SIZE);
#endif
#if ENABLE_CAN0
	// RX buffers storage
	CAN0_REGS->CAN_RXBC = CAN_RXBC_RBSA((uint32_t) can_rx_buffers);
#endif
#if ENABLE_CAN1
	// RX buffers storage
	CAN1_REGS->CAN_RXBC = CAN_RXBC_RBSA((uint32_t) can_rx_buffers);
#endif
#if ENABLE_CAN0
	// TX buffers storage
	CAN0_REGS->CAN_TXBC = CAN_TXBC_TBSA((uint32_t) can_tx_buffers) | CAN_TXBC_NDTB(DRV_CAN_TX_BUFFER_COUNT);
#endif
#if ENABLE_CAN1
	// TX buffers storage
	CAN1_REGS->CAN_TXBC = CAN_TXBC_TBSA((uint32_t) can_tx_buffers) | CAN_TXBC_NDTB(DRV_CAN_TX_BUFFER_COUNT);
#endif
#if ENABLE_CAN0
	// TX element size (for FD mode) configuration
	CAN0_REGS->CAN_TXESC = CAN_TXESC_TBDS(CAN_TX_DATA_SIZE);
#endif
#if ENABLE_CAN1
	// TX element size (for FD mode) configuration
	CAN1_REGS->CAN_TXESC = CAN_TXESC_TBDS(CAN_TX_DATA_SIZE);
#endif
	// bit rate timing (currently unused)
		// With a GCLK_CAN of 8MHz, the reset value 0x00000A33 configures the CAN for a fast bit rate of 500 kBits/s.
		//CAN0_REGS->CAN_DBTP = ...;
	
	// nominal bit rate timing and prescaler
		// With a CAN clock (GCLK_CAN) of 8MHz, the reset value 0x06000A03 configures the CAN for a bit rate of 500 kBits/s.
		//CAN0_REGS->CAN_NBTP = ...;
#if ENABLE_CAN0
	// enable timestamping. we should reset TSCV every ms so we know correct microsecond timing or something like that
	CAN0_REGS->CAN_TSCC = CAN_TSCC_TSS_INC;
#endif
#if ENABLE_CAN1
	// enable timestamping. we should reset TSCV every ms so we know correct microsecond timing or something like that
	CAN1_REGS->CAN_TSCC = CAN_TSCC_TSS_INC;
#endif
#if ENABLE_CAN0
	// enable interrupts. for now on all received messages
	CAN0_REGS->CAN_IE = CAN_IE_RF1NE(1) | CAN_IE_RF0NE(1) | CAN_IE_DRXE(1);
#endif
#if ENABLE_CAN1
	// enable interrupts. for now on all received messages
	CAN1_REGS->CAN_IE = CAN_IE_RF1NE(1) | CAN_IE_RF0NE(1) | CAN_IE_DRXE(1);
#endif
#if ENABLE_CAN0
	// enable interrupt line 0. this is the default for all interrupts unless changed
	CAN0_REGS->CAN_ILE = CAN_ILE_EINT0(1);
#endif
#if ENABLE_CAN1
	// enable interrupt line 0. this is the default for all interrupts unless changed
	CAN1_REGS->CAN_ILE = CAN_ILE_EINT0(1);
#endif
#if ENABLE_CAN0
	// general filter configuration. accept nonmatching into FIFO 0. reject remote.
	CAN0_REGS->CAN_GFC = CAN_GFC_ANFS_RXF0 | CAN_GFC_ANFE_RXF0 | CAN_GFC_RRFS(1) | CAN_GFC_RRFE(1);
#endif
#if ENABLE_CAN1
	// general filter configuration. accept nonmatching into FIFO 0. reject remote.
	CAN1_REGS->CAN_GFC = CAN_GFC_ANFS_RXF0 | CAN_GFC_ANFE_RXF0 | CAN_GFC_RRFS(1) | CAN_GFC_RRFE(1);
#endif
	
	// drop out of configuration mode and start
#if ENABLE_CAN0
	CAN0_REGS->CAN_CCCR = 0;
#endif
#if ENABLE_CAN1
	CAN1_REGS->CAN_CCCR = 0;
#endif
}

void CAN0_Handler()
{
	// handle no interrupts and just reset the IR
	CAN0_REGS->CAN_IR = 0xFFFFFFFF;
}

void CAN1_Handler()
{
	CAN1_REGS->CAN_IR = 0xFFFFFFFF;
}

struct drv_can_rx_buffer_element * drv_can_get_rx_buffer(enum drv_can_rx_buffer_table id)
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

struct drv_can_tx_buffer_element * drv_can_get_tx_buffer(enum drv_can_tx_buffer_table id)
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

void drv_can_queue_tx_buffer(can_registers_t * bus, enum drv_can_tx_buffer_table id)
{
	if (id < DRV_CAN_TX_BUFFER_COUNT)
	{
		bus->CAN_TXBAR = (1 << id);
	}
}

bool drv_can_check_rx_buffer(can_registers_t * bus, enum drv_can_rx_buffer_table id)
{
	if (id < DRV_CAN_RX_BUFFER_COUNT)
	{
		if (id < 32)
		{
			return (bus->CAN_NDAT1 >> id) & 1;
		}
		else
		{
			return (bus->CAN_NDAT2 >> (id - 32)) & 1;
		}
	}
	else
	{
		return false;
	}
}

void drv_can_clear_rx_buffer(can_registers_t * bus, enum drv_can_rx_buffer_table id)
{
	if (id < DRV_CAN_RX_BUFFER_COUNT)
	{
		if (id < 32)
		{
			bus->CAN_NDAT1 = (1 << id);
		}
		else
		{
			bus->CAN_NDAT2 = (1 << (id - 32));
		}
	}
}