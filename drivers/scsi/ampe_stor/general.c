#include "amcr_stor.h"


/*************************************************************************************************/
u32 am_swap_u32(u32 i)
{
	DWORDB value;
	u8 tmp;

	value.dword = i;

	tmp = value.byte[LL];
	value.byte[LL] = value.byte[HH];
	value.byte[HH] = tmp;

	tmp = value.byte[LH];
	value.byte[LH] = value.byte[HL];
	value.byte[HL] = tmp;

	return value.dword;
}

/*************************************************************************************************/
int card_active_ctrl(struct _DEVICE_EXTENSION *pdx, u8 value)
{

	writeb(value, pdx->ioaddr + CARD_ACTIVE_CTRL);
	pdx->current_card_active = value;

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int card_reset(struct _DEVICE_EXTENSION	*pdx, u8 card_type)
{
	u32	timeout;

	writeb(BUFFER_CTRL_RESET | card_type, pdx->ioaddr + CARD_SOFTWARE_RESET);

	/* Wait max 200 ms */
	timeout = 200;

	/* HW clears the bit when it's done */
	while (readb(pdx->ioaddr + CARD_SOFTWARE_RESET) & card_type) {
		if (timeout == 0) {
			TRACEX(("Reset never completed, XXXXXXXXXXXXXXXXXXXXXXXXX"));
			return ERR_ERROR;
		}
		timeout--;
		sys_delay(1);
	}

	if (pdx->dev_is_6621) {
		writeb(0x01, pdx->ioaddr + CARD_DMA_PAGE_CNT);
	}
	else {
		writeb(0x00, pdx->ioaddr + CARD_DMA_BOUNDARY);
	}	

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int card_power_ctrl(struct _DEVICE_EXTENSION *pdx, u8 card_type, u8 pwr_on)
{
	u8 card_pwr;
	u8 output_enable;

	card_pwr = readb(pdx->ioaddr + CARD_POWER_CONTROL);
	output_enable = readb(pdx->ioaddr + CARD_OUTPUT_ENABLE);

	TRACE1(("card_pwr: %x, output_enable: %x", card_pwr, output_enable));

	if (pwr_on) {
		card_pwr |= card_type;
		output_enable |= card_type;
		writeb(card_pwr, pdx->ioaddr + CARD_POWER_CONTROL);
		sys_delay(20);
		writeb(output_enable, pdx->ioaddr + CARD_OUTPUT_ENABLE);
	}
	else {
		card_pwr &= ~card_type;
		output_enable &= ~card_type;
		writeb(output_enable, pdx->ioaddr + CARD_OUTPUT_ENABLE);
		writeb(card_pwr, pdx->ioaddr + CARD_POWER_CONTROL);
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int card_set_clock(struct _DEVICE_EXTENSION *pdx, u32 clock)
{
	u16 clk_src;
	u8 clk_div;

	if (clock == 0) {
		//writew(0, pdx->ioaddr + CARD_CLK_SELECT);
		writeb(0x00, pdx->ioaddr + SD_DATA_XFER_CTRL);
		return ERR_SUCCESS;
	}

	if (clock <= CARD_MIN_CLOCK) {
		clk_src = CARD_CLK_31_25_MHZ;
		clk_div = 200;
	}

	if ((CARD_MIN_CLOCK < clock) && (clock < 5)) {
		clk_src = CARD_CLK_31_25_MHZ;
		clk_div = 16;
	}

	if ((5 <= clock) && (clock < 10)) {
		clk_src = CARD_CLK_48_MHZ;
		clk_div = 10;
	}

	if ((10 <= clock) && (clock < 20)) {
		clk_src = CARD_CLK_48_MHZ;
		clk_div = 5;
	}

	if ((20 <= clock) && (clock < 25)) {
		clk_src = CARD_CLK_125_MHZ;
		clk_div = 7;
	}

	if ((25 <= clock) && (clock < 40)) {
		clk_src = CARD_CLK_48_MHZ;
		clk_div = 2;
	}

	if ((40 <= clock) && (clock < 50)) {
		clk_src = CARD_CLK_384_MHZ;
		clk_div = 10;
	}

	if ((50 <= clock) && (clock < 60)) {
		clk_src = CARD_CLK_48_MHZ;
		clk_div = 1;
	}

	if ((60 <= clock) && (clock < 80)) {
		clk_src = CARD_CLK_384_MHZ;
		clk_div = 7;
	}

	if ((80 <= clock) && (clock < 100)) {
		clk_src = CARD_CLK_384_MHZ;
		clk_div = 5;
	}

	if ((100 <= clock) && (clock < 130)) {
		clk_src = CARD_CLK_384_MHZ;
		clk_div = 4;
	}

	if ((130 <= clock) && (clock < 194)) {
		clk_src = CARD_CLK_384_MHZ;
		clk_div = 3;
	}

	if ((194 <= clock) && (clock < 208)) {
		clk_src = CARD_CLK_384_MHZ;
		clk_div = 2;
	}

	if (208 <= clock) {
		clk_src = CARD_CLK_384_MHZ | CARD_CLK_OVER_CLK;
		clk_div = 2;
	}


	clk_src |= ((clk_div - 1) << 8);
	clk_src |= CARD_CLK_ENABLE;

	// for AU6621, always enable X2 mode if uExtX2Mode is enabled
	if (pdx->dev_is_6621) {
		if ((pdx->uExtX2Mode) && (clock >= 60)) {
			clk_src |= (CARD_CLK_X2_MODE);
		}
	}


	pdx->current_clk_src = (u8)clk_src;

	writew(clk_src, pdx->ioaddr + CARD_CLK_SELECT);

	TRACEW(("CARD_CLK_SELECT: %04x", readw(pdx->ioaddr + CARD_CLK_SELECT)));

	return ERR_SUCCESS;
}

/*************************************************************************************************/
void card_init_interface_mode_ctrl(struct _DEVICE_EXTENSION *pdx)
{

	pdx->current_inf_ctrl = 0;

	if (pdx->uExtEnInterruptDelay) {
		TRACEW(("Set interrupt delay time, --------------------------"));
		pdx->current_inf_ctrl |= INTERRUPT_DELAY_TIME;
	}
	else {
		TRACEW(("Without interrupt delay time, ......................"));
	}

	if (pdx->uExtSignalCtrl) {
		TRACEW(("Set PCIE interface signal clock req control: %x, --------------------------", pdx->uExtSignalCtrl));
		pdx->current_inf_ctrl |= (pdx->uExtSignalCtrl & CARD_SIGNAL_REQ_CTRL);
	}
	else {
		TRACEW(("Without PCIE interface signal clock req control, ......................."));
	}

	writeb(pdx->current_inf_ctrl, pdx->ioaddr + CARD_INTERFACE_MODE_CTRL);
}
