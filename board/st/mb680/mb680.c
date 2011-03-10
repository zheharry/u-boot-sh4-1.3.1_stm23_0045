/*
 * (C) Copyright 2008-2009 STMicroelectronics.
 *
 * Sean McGoogan <Sean.McGoogan@st.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <asm/soc.h>
#include <asm/stx7105reg.h>
#include <asm/io.h>
#include <asm/pio.h>


#define PIO_BASE  0xfd020000	/* Base of PIO block in COMMs block */


/* following are the offsets in the EMI functions EPLD (IC21),
 * in the STB Peripheral board (MB705)*/
#define EPLD_IDENT		0x00	/* EPLD Identifier Register */
#define EPLD_TEST		0x02	/* EPLD Test Register */
#define EPLD_SWITCH		0x04	/* EPLD Switch Register */
#define EPLD_MISC		0x0a	/* Miscellaneous Control Register */

#ifdef CONFIG_SH_SE_MODE
#define EPLD_BASE		0xb7000000	/* Phys 0x07000000 */
#else
#define EPLD_BASE		0xa7000000	/* EMI Bank E */
#endif	/* CONFIG_SH_SE_MODE */


static inline void epld_write(unsigned long value, unsigned long offset)
{
	/* 16-bit write to EPLD registers */
	writew(value, EPLD_BASE + offset);
}

static inline unsigned long epld_read(unsigned long offset)
{
	/* 16-bit read from EPLD registers */
	return readw(EPLD_BASE + offset);
}

void flashWriteEnable(void)
{
	unsigned short epld_reg;

	/* Enable Vpp for writing to flash */
	epld_reg = epld_read(EPLD_MISC);
	epld_reg |= 1u << 3;	/* NandFlashWP = MISC[3] = 1 */
	epld_reg |= 1u << 2;	/* NorFlashVpp = MISC[2] = 1 */
	epld_write(epld_reg, EPLD_MISC);
}

void flashWriteDisable(void)
{
	unsigned short epld_reg;

	/* Disable Vpp for writing to flash */
	epld_reg = epld_read(EPLD_MISC);
	epld_reg &= ~(1u << 3);	/* NandFlashWP = MISC[3] = 0 */
	epld_reg &= ~(1u << 2);	/* NorFlashVpp = MISC[2] = 0 */
	epld_write(epld_reg, EPLD_MISC);
}

static int mb680_init_epld(void)
{
	const unsigned short test_value = 0x1234u;
	unsigned short epld_reg;
	unsigned short epld_version, board_version;

	/* write (anything) to the test register */
	epld_write(test_value, EPLD_TEST);
	/* verify we got back an inverted result */
	epld_reg = epld_read(EPLD_TEST);
	if (epld_reg != (test_value ^ 0xffffu)) {
		printf("Failed EPLD test (offset=%02x, result=%04x)\n",
			EPLD_TEST, epld_reg);
		return 1;
		}

	/* Assume we can trust the version register */
	epld_reg = epld_read(EPLD_IDENT);
	board_version = epld_reg >> 4 & 0xfu;
	epld_version = epld_reg & 0xfu;

	/* display the board revision, and EPLD version */
	printf("MB705: revision %c, EPLD version %02d\n",
		board_version + 'A' - 1,
		epld_version);

	/* return a "success" result */
	return 0;
}

#ifdef CONFIG_STMAC_LAN8700
static void phy_reset(void)
{
	/* Reset the SMSC LAN8700 PHY */
	STPIO_SET_PIN(PIO_PORT(5), 5, 1);
	STPIO_SET_PIN(PIO_PORT(11), 2, 1);
	udelay(1);
	STPIO_SET_PIN(PIO_PORT(5), 5, 0);
	udelay(100);
	STPIO_SET_PIN(PIO_PORT(5), 5, 1);
	udelay(1);
	STPIO_SET_PIN(PIO_PORT(11), 2, 0);
}
#endif	/* CONFIG_STMAC_LAN8700 */

static void configPIO(void)
{
	unsigned long sysconf;

	/* Setup PIO of ASC device */
	SET_PIO_ASC(PIO_PORT(4), 0, 1, 2, 3);  /* UART2 - AS0 */
	SET_PIO_ASC(PIO_PORT(5), 0, 1, 3, 2);  /* UART3 - AS1 */

	/* Select UART2 via PIO4 */
	sysconf = *STX7105_SYSCONF_SYS_CFG07;
	/* CFG07[1] = UART2_RXD_SRC_SELECT = 0 */
	/* CFG07[2] = UART2_CTS_SRC_SELECT = 0 */
	sysconf &= ~(1ul<<2 | 1ul<<1);
	*STX7105_SYSCONF_SYS_CFG07 = sysconf;

	/* Route UART2 via PIO4 for TX, RX, CTS & RTS */
	sysconf = *STX7105_SYSCONF_SYS_CFG34;
	/* PIO4[0] CFG34[8,0]   AltFunction = 3 */
	/* PIO4[1] CFG34[9,1]   AltFunction = 3 */
	/* PIO4[2] CFG34[10,2]  AltFunction = 3 */
	/* PIO4[3] CFG34[11,3]  AltFunction = 3 */
	sysconf &= ~0x0f0ful;	/* 3,3,3,3 */
	sysconf |=  0x0f00ul;	/* 2,2,2,2 */
	*STX7105_SYSCONF_SYS_CFG34 = sysconf;

	/* Route UART3 via PIO5 for TX, RX, CTS & RTS */
	sysconf = *STX7105_SYSCONF_SYS_CFG35;
	/* PIO5[0] CFG35[8,0]   AltFunction = 3 */
	/* PIO5[1] CFG35[9,1]   AltFunction = 3 */
	/* PIO5[2] CFG35[10,2]  AltFunction = 3 */
	/* PIO5[3] CFG35[11,3]  AltFunction = 3 */
	sysconf &= ~0x0f0ful;	/* 3,3,3,3 */
	sysconf |=  0x000ful;	/* 1,1,1,1 */
	*STX7105_SYSCONF_SYS_CFG35 = sysconf;

#ifdef CONFIG_STMAC_LAN8700
	/* Configure SMSC LAN8700 PHY Reset signals */
	SET_PIO_PIN(PIO_PORT(5), 5, STPIO_OUT);
	SET_PIO_PIN(PIO_PORT(11), 2, STPIO_OUT);
#endif	/* CONFIG_STMAC_LAN8700 */
}

extern int board_init(void)
{
	configPIO();

	/* Reset the PHY */
#ifdef CONFIG_STMAC_LAN8700
	phy_reset();
#endif	/* CONFIG_STMAC_LAN8700 */

#if defined(CONFIG_SH_STM_SATA)
	stx7105_configure_sata ();
#endif	/* CONFIG_SH_STM_SATA */

	return 0;
}

int checkboard (void)
{
	printf ("\n\nBoard: STx7105-Mboard (MB680)"
#ifdef CONFIG_SH_SE_MODE
		"  [32-bit mode]"
#else
		"  [29-bit mode]"
#endif
		"\n");

	/*
	 * initialize the EPLD.
	 */
	mb680_init_epld();

#if 0	/* QQQ - DELETE */
{
const unsigned long nand_reg = *ST40_EMI_NAND_VERSION_REG;
const unsigned long epld_reg = epld_read(EPLD_SWITCH);
	printf ("*ST40_EMI_NAND_VERSION_REG = %u.%u.%u\n",
		(nand_reg>>8)&0x0ful,
		(nand_reg>>4)&0x0ful,
		(nand_reg>>0)&0x0ful);
	printf("*EPLD_SWITCH = 0x%08x  -->  boot-from-%s\n",
		epld_reg,
		(epld_reg & (1ul<<8)) ? "NAND" : "NOR");
}
#endif	/* QQQ - DELETE */

	return 0;
}
