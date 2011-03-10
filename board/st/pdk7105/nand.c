/*
 * (C) Copyright 2006 DENX Software Engineering
 * (C) Copyright 2008-2009 STMicroelectronics, Sean McGoogan <Sean.McGoogan@st.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <nand.h>
#include <asm/io.h>
#include <asm/pio.h>
#include <asm/stx7105reg.h>
#include <asm/stm-nand.h>

/*
 * hardware specific access to control-lines for "bit-banging".
 *	CL -> Emi_Addr(17)
 *	AL -> Emi_Addr(18)
 *	nCE is handled by EMI (not s/w controlable)
 */
#ifndef CFG_NAND_FLEX_MODE	/* for "bit-banging" (c.f. STM "flex-mode")  */
static void pdk7105_hwcontrol(struct mtd_info *mtdinfo, int cmd)
{
	struct nand_chip* this = (struct nand_chip *)(mtdinfo->priv);

	switch(cmd) {

	case NAND_CTL_SETCLE:
		this->IO_ADDR_W = (void *)((unsigned int)this->IO_ADDR_W | (1u << 17));
		break;

	case NAND_CTL_CLRCLE:
		this->IO_ADDR_W = (void *)((unsigned int)this->IO_ADDR_W & ~(1u << 17));
		break;

	case NAND_CTL_SETALE:
		this->IO_ADDR_W = (void *)((unsigned int)this->IO_ADDR_W | (1u << 18));
		break;

	case NAND_CTL_CLRALE:
		this->IO_ADDR_W = (void *)((unsigned int)this->IO_ADDR_W & ~(1u << 18));
		break;
	}
}
#endif /* CFG_NAND_FLEX_MODE */


/*
 * hardware specific access to the Ready/not_Busy signal.
 * Signal is routed through the EMI NAND Controller block.
 */
#ifndef CFG_NAND_FLEX_MODE	/* for "bit-banging" (c.f. STM "flex-mode")  */
static int pdk7105_device_ready(struct mtd_info *mtd)
{
	/* extract bit 1: status of RBn pin on boot bank */
	return ((*ST40_EMI_NAND_RBN_STA) & (1ul<<1)) ? 1 : 0;
}
#endif /* CFG_NAND_FLEX_MODE */


/*
 * Board-specific NAND initialization. The following members of the
 * argument are board-specific (per include/linux/mtd/nand.h):
 * - IO_ADDR_R?: address to read the 8 I/O lines of the flash device
 * - IO_ADDR_W?: address to write the 8 I/O lines of the flash device
 * - hwcontrol: hardwarespecific function for accesing control-lines
 * - dev_ready: hardwarespecific function for  accesing device ready/busy line
 * - enable_hwecc?: function to enable (reset)  hardware ecc generator. Must
 *   only be provided if a hardware ECC is available
 * - eccmode: mode of ecc, see defines
 * - chip_delay: chip dependent delay for transfering data from array to
 *   read regs (tR)
 * - options: various chip options. They can partly be set to inform
 *   nand_scan about special functionality. See the defines for further
 *   explanation
 * Members with a "?" were not set in the merged testing-NAND branch,
 * so they are not set here either.
 */
extern int board_nand_init(struct nand_chip *nand)
{
	nand->eccmode       = NAND_ECC_SOFT;
	nand->options       = NAND_NO_AUTOINCR;

#ifdef CFG_NAND_FLEX_MODE	/* for STM "flex-mode" (c.f. "bit-banging") */
	nand->select_chip   = stm_flex_select_chip;
	nand->dev_ready     = stm_flex_device_ready;
	nand->hwcontrol     = stm_flex_hwcontrol;
	nand->read_byte     = stm_flex_read_byte;
	nand->write_byte    = stm_flex_write_byte;
	nand->read_buf      = stm_flex_read_buf;
	nand->write_buf     = stm_flex_write_buf;
#else				/* for "bit-banging" (c.f. STM "flex-mode")  */
	nand->dev_ready     = pdk7105_device_ready;
	nand->hwcontrol     = pdk7105_hwcontrol;
#endif /* CFG_NAND_FLEX_MODE */

#if 1
	/* Enable the following to use a Bad Block Table (BBT) */
	nand->options      |= NAND_USE_FLASH_BBT;
	nand->scan_bbt      = stm_nand_default_bbt;
#endif

	return 0;
}
