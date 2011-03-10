/*
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
#include <malloc.h>


#if defined(CONFIG_CMD_NAND)

#include <nand.h>
#include <asm/stm-nand.h>
#include <asm/ecc.h>
#include <asm/errno.h>
#include <asm/st40reg.h>
#include <asm/io.h>
#include <asm/socregs.h>


#define isprint(x)	( ((x)>=0x20u) && ((x)<0x7fu) )


#define DEBUG_FLEX		0	/* Enable additional debugging of the FLEX controller */
#define VERBOSE_ECC		0	/* Enable for verbose ECC information  */


/*
 * Define the bad/good block scan pattern which are used while scanning
 * the NAND device for any factory marked good/bad blocks.
 *
 * For small page devices, factory bad block markers are in byte 5.
 * For large page devices, factory bad block markers are in bytes 0 & 1.
 * Any bit in those bytes being a zero, implies the entire block is bad.
 *
 * Using "SCANALLPAGES" takes a significant performance hit - tens of seconds!
 * So, we override the default structures (which is to scan *all* pages),
 * and instead, we only scan the first & 2nd pages in each block.
 * However, we do also check the entire data area in those 2 pages,
 * to see if they are properly erased. Improperly erased pages
 * result in the entire bock also being treated as bad.
 */
static uint8_t scan_pattern[] = { 0xffu, 0xffu };

struct nand_bbt_descr stm_nand_badblock_pattern_16 = {
	.pattern = scan_pattern,
	.options = NAND_BBT_SCANEMPTY /* | NAND_BBT_SCANALLPAGES */ | NAND_BBT_SCAN2NDPAGE,
	.offs = 5,	/* Byte 5 */
	.len = 1
};

struct nand_bbt_descr stm_nand_badblock_pattern_64 = {
	.pattern = scan_pattern,
	.options = NAND_BBT_SCANEMPTY /* | NAND_BBT_SCANALLPAGES */ | NAND_BBT_SCAN2NDPAGE,
	.offs = 0,	/* Bytes 0-1 */
	.len = 2
};


#ifdef CFG_NAND_ECC_HW3_128	/* for STM "boot-mode" */

	/* for SMALL-page devices */
static struct nand_oobinfo stm_nand_oobinfo_16 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 12,
	.eccpos = {
		 0,  1,  2,	/* ECC for 1st 128-byte record */
		 4,  5,  6,	/* ECC for 2nd 128-byte record */
		 8,  9, 10,	/* ECC for 3rd 128-byte record */
		12, 13, 14},	/* ECC for 4th 128-byte record */
	.oobfree = { {3, 1}, {7, 1}, {11, 1}, {15, 1} }
};

	/* for LARGE-page devices */
static struct nand_oobinfo stm_nand_oobinfo_64 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 48,
	.eccpos = {
		 0,  1,  2,	/* ECC for  1st 128-byte record */
		 4,  5,  6,	/* ECC for  2nd 128-byte record */
		 8,  9, 10,	/* ECC for  3rd 128-byte record */
		12, 13, 14,	/* ECC for  4th 128-byte record */
		16, 17, 18,	/* ECC for  5th 128-byte record */
		20, 21, 22,	/* ECC for  6th 128-byte record */
		24, 25, 26,	/* ECC for  7th 128-byte record */
		28, 29, 30,	/* ECC for  8th 128-byte record */
		32, 33, 34,	/* ECC for  9th 128-byte record */
		36, 37, 38,	/* ECC for 10th 128-byte record */
		40, 41, 42,	/* ECC for 11th 128-byte record */
		44, 45, 46,	/* ECC for 12th 128-byte record */
		48, 49, 50,	/* ECC for 13th 128-byte record */
		52, 53, 54,	/* ECC for 14th 128-byte record */
		56, 57, 58,	/* ECC for 15th 128-byte record */
		60, 61, 62},	/* ECC for 16th 128-byte record */
	.oobfree = {
		{ 3, 1}, { 7, 1}, {11, 1}, {15, 1},
		{19, 1}, {23, 1}, {27, 1}, {31, 1},
		{35, 1}, {39, 1}, {43, 1}, {47, 1},
		{51, 1}, {55, 1}, {59, 1}, {63, 1} },
};


/*
 * define a structure to hold the fields in the "struct mtd_info"
 * and "struct nand_chip" structures that we need to over-write
 * to allow 2 (incompatible) ECC configurations to exist on
 * the same physical NAND flash device at the same time!
 *
 * This is required as some of the STi7xxx devices, have a property
 * that when in NAND "boot-mode" (i.e. boot-from-NAND) that an ECC
 * configuration of 3 ECC bytes per 128 byte record *must* be used.
 * However, when not in "boot-mode", less aggressive ECC configuration
 * of either 3 ECC bytes per 256 or 512 bytes may be used.
 *
 * When using this "hybrid" of 2 different ECC configurations on the
 * same physical device, we use 2 instances of this structure:
 *	1) based on the normal (default) configuration (e.g. S/W ECC)
 *	2) the "boot-mode" 'special' with 3/128 H/W compatibility.
 *
 * Various functions are intercepted to ensure that the mtd_info
 * and nand_chip structures always have the correct "view" of the ECC
 * configurations, then the default functions can be safely used as is.
 */
struct stm_mtd_nand_ecc
{
	struct	/* holds differences in "struct nand_chip" */
	{
		int			eccmode;
		int			eccsize;
		int			eccsteps;
		struct nand_oobinfo	*autooob;
		int (*calculate_ecc)(struct mtd_info *, const u_char *, u_char *);
		int (*correct_data)(struct mtd_info *, u_char *, u_char *, u_char *);
	}	nand;
	struct	/* holds differences in "struct mtd_info" */
	{
		u_int32_t		oobavail;
		u_int32_t		eccsize;
		struct nand_oobinfo	oobinfo;
	}	mtd;
};


static struct stm_mtd_nand_ecc default_ecc;	/* ECC diffs for the normal SW case */
static struct stm_mtd_nand_ecc special_ecc;	/* ECC diffs for the "special" hybrid case */
static int done_ecc_info = 0;			/* are the 2 ECC structures initialized ? */


#endif	/* CFG_NAND_ECC_HW3_128 */


#ifdef CFG_NAND_FLEX_MODE	/* for STM "flex-mode" (c.f. "bit-banging") */

/* Flex-Mode Data {Read,Write} Config Registers & Flex-Mode {Command,Address} Registers */
#define FLEX_WAIT_RBn			( 1u << 27 )	/* wait for RBn to be asserted (i.e. ready) */
#define FLEX_BEAT_COUNT_1		( 1u << 28 )	/* One Beat */
#define FLEX_BEAT_COUNT_2		( 2u << 28 )	/* Two Beats */
#define FLEX_BEAT_COUNT_3		( 3u << 28 )	/* Three Beats */
#define FLEX_BEAT_COUNT_4		( 0u << 28 )	/* Four Beats */
#define FLEX_CSn_STATUS			( 1u << 31 )	/* Deasserts CSn after current operation completes */

/* Flex-Mode Data-{Read,Write} Config Registers */
#define FLEX_1_BYTE_PER_BEAT		( 0u << 30 )	/* One Byte per Beat */
#define FLEX_2_BYTES_PER_BEAT		( 1u << 30 )	/* Two Bytes per Beat */

/* Flex-Mode Configuration Register */
#define FLEX_CFG_ENABLE_FLEX_MODE	( 1u <<  0 )	/* Enable Flex-Mode operations */
#define FLEX_CFG_ENABLE_AFM		( 2u <<  0 )	/* Enable Advanced-Flex-Mode operations */
#define FLEX_CFG_SW_RESET		( 1u <<  3 )	/* Enable Software Reset */
#define FLEX_CFG_CSn_STATUS		( 1u <<  4 )	/* Deasserts CSn in current Flex bank */


enum stm_nand_flex_mode {
	flex_quiecent,		/* next byte_write is *UNEXPECTED* */
	flex_command,		/* next byte_write is a COMMAND */
	flex_address		/* next byte_write is a ADDRESS */
};


/*
 * NAND device connected to STM NAND Controller operating in FLEX mode.
 * There may be several NAND device connected to the NAND controller.
 */
struct stm_nand_flex_device {
	int			csn;
	struct nand_chip	*chip;
	struct mtd_info		*mtd;
	struct nand_timing_data *timing_data;
};


/*
 * STM NAND Controller operating in FLEX mode.
 * There is only a single one of these.
 */
static struct stm_nand_flex_controller {
	int			initialized;	/* is the FLEX controller initialized ? */
	int			current_csn;	/* Currently Selected Device (CSn) */
	int			next_csn;	/* First free NAND Device (CSn) */
	enum stm_nand_flex_mode mode;
	struct stm_nand_flex_device device[CFG_MAX_NAND_DEVICE];
	uint8_t			*buf;		/* Bounce buffer for non-aligned xfers */
} flex;


#endif /* CFG_NAND_FLEX_MODE */


extern int stm_nand_default_bbt (struct mtd_info *mtd)
{
	struct nand_chip * const this = (struct nand_chip *)(mtd->priv);

	/* over-write the default "badblock_pattern", with our one */
	/* choose the correct pattern struct, depending on the OOB size */
	if (mtd->oobsize > 16)
		this->badblock_pattern = &stm_nand_badblock_pattern_64;	/* LARGE-page */
	else
		this->badblock_pattern = &stm_nand_badblock_pattern_16;	/* SMALL-page */

	/* now call the generic BBT function */
	return nand_default_bbt (mtd);
}


#ifdef CFG_NAND_ECC_HW3_128	/* for STM "boot-mode" */


extern int stm_nand_calculate_ecc (
	struct mtd_info * const mtd,
	const u_char * const dat,
	u_char * const ecc_code)
{
	const struct nand_chip const * this = mtd->priv;

	if (this->eccmode!=NAND_ECC_HW3_128)
	{
		printf("ERROR: Can not calculate ECC: Internal Error (eccmode=%u)\n",
			this->eccmode);
		BUG();
		return -1;	/* Note: caller ignores this value! */
	}
	else if ((((unsigned long)dat)%4)!=0)	/* data *must* be 4-bytes aligned */
	{
		/* QQQ: change this case to use a properly aligned bounce buffer */
		printf("ERROR: Can not calculate ECC: data (%08lx) must be 4-byte aligned!\n",
			(unsigned long)dat);
		ecc_code[0] = 'B';
		ecc_code[1] = 'A';
		ecc_code[2] = 'D';
		return -1;	/* Note: caller ignores this value! */
	}
	else
	{	/* calculate 3 ECC bytes per 128 bytes of data */
		const ecc_t computed_ecc = ecc_gen(dat, ECC_128);
		/* poke them into the right place */
		ecc_code[0] = computed_ecc.byte[0];
		ecc_code[1] = computed_ecc.byte[1];
		ecc_code[2] = computed_ecc.byte[2];
	}

	return 0;
}


extern int stm_nand_correct_data (
	struct mtd_info *mtd,
	u_char *dat,
	u_char *read_ecc,
	u_char *calc_ecc)
{
	ecc_t read, calc;
	enum ecc_check result;
	const struct nand_chip const * this = mtd->priv;

	if (this->eccmode!=NAND_ECC_HW3_128)
	{
		printf("ERROR: Can not correct ECC: Internal Error (eccmode=%u)\n",
			this->eccmode);
		BUG();
		return -1;
	}

	/* do we need to try and correct anything ? */
	if (    (read_ecc[0] == calc_ecc[0]) &&
		(read_ecc[1] == calc_ecc[1]) &&
		(read_ecc[2] == calc_ecc[2])    )
	{
		return 0;		/* ECCs agree, nothing to do */
	}

#if VERBOSE_ECC
	printf("warning: ECC error detected!  "
		"read_ecc %02x:%02x:%02x (%c%c%c) != "
		"calc_ecc %02x:%02x:%02x (%c%c%c)\n",
		(unsigned)read_ecc[0],
		(unsigned)read_ecc[1],
		(unsigned)read_ecc[2],
		isprint(read_ecc[0]) ? read_ecc[0] : '.',
		isprint(read_ecc[1]) ? read_ecc[1] : '.',
		isprint(read_ecc[2]) ? read_ecc[2] : '.',
		(unsigned)calc_ecc[0],
		(unsigned)calc_ecc[1],
		(unsigned)calc_ecc[2],
		isprint(calc_ecc[0]) ? calc_ecc[0] : '.',
		isprint(calc_ecc[1]) ? calc_ecc[1] : '.',
		isprint(calc_ecc[2]) ? calc_ecc[2] : '.');
#endif	/* VERBOSE_ECC */

	/* put ECC bytes into required structure */
	read.byte[0] = read_ecc[0];
	read.byte[1] = read_ecc[1];
	read.byte[2] = read_ecc[2];
	calc.byte[0] = calc_ecc[0];
	calc.byte[1] = calc_ecc[1];
	calc.byte[2] = calc_ecc[2];

	/* correct a 1-bit error (if we can) */
	result = ecc_correct(dat, read, calc, ECC_128);

	/* let the user know if we were able to recover it or not! */
	switch (result)
	{
		case E_D1_CHK:
			printf("info: 1-bit error in data was corrected\n");
			break;
		case E_C1_CHK:
			printf("info: 1-bit error in ECC ignored (data was okay)\n");
			break;
		default:
#if VERBOSE_ECC
			/* QQQ: filter out genuinely ERASED pages - TO DO */
			printf("ERROR: uncorrectable ECC error not corrected!\n");
#endif	/* VERBOSE_ECC */
			break;
	}

	/* return zero if all okay, and -1 if we have an uncorrectable issue */
	if ((result==E_D1_CHK)||(result==E_C1_CHK))
	{
		return 0;	/* okay (correctable) */
	}
	else
	{
		return -1;	/* uncorrectable */
	}
}


/*
 * fill in the "default_ecc" and "special_ecc" structures.
 */
static void initialize_ecc_diffs (
	const struct mtd_info * const mtd)
{
	const struct nand_chip * const this = (struct nand_chip *)(mtd->priv);
	struct nand_oobinfo * autooob;

	/* choose the correct OOB info struct, depending on the OOB size */
	if (mtd->oobsize > 16)
		autooob = &stm_nand_oobinfo_64;	/* LARGE-page */
	else
		autooob = &stm_nand_oobinfo_16;	/* SMALL-page */

	/* fill in "default_ecc" from the current "live" (default) structures */
	default_ecc.nand.eccmode	= this->eccmode;
	default_ecc.nand.eccsize	= this->eccsize;
	default_ecc.nand.eccsteps	= this->eccsteps;
	default_ecc.nand.autooob	= this->autooob;
	default_ecc.nand.calculate_ecc	= this->calculate_ecc;
	default_ecc.nand.correct_data	= this->correct_data;
	default_ecc.mtd.oobavail	= mtd->oobavail;
	default_ecc.mtd.eccsize		= mtd->eccsize;
	memcpy(&default_ecc.mtd.oobinfo, &mtd->oobinfo, sizeof(struct nand_oobinfo));

	/* fill in "special_ecc" for our special "hybrid" ECC paradigm */
	special_ecc.nand.eccmode	= NAND_ECC_HW3_128;
	special_ecc.nand.eccsize	= 128;
	special_ecc.nand.eccsteps	= mtd->oobblock / special_ecc.nand.eccsize;
	special_ecc.nand.autooob	= autooob;
	special_ecc.nand.calculate_ecc	= stm_nand_calculate_ecc;
	special_ecc.nand.correct_data	= stm_nand_correct_data;
	if (this->options & NAND_BUSWIDTH_16) {
		special_ecc.mtd.oobavail= mtd->oobsize - (autooob->eccbytes + 2);
		special_ecc.mtd.oobavail= special_ecc.mtd.oobavail & ~0x01;
	} else {
		special_ecc.mtd.oobavail= mtd->oobsize - (autooob->eccbytes + 1);
	}
	special_ecc.mtd.eccsize		= special_ecc.nand.eccsize;
	memcpy(&special_ecc.mtd.oobinfo, autooob, sizeof(struct nand_oobinfo));
}


/*
 * Make the "live" MTD structures use the ECC configuration
 * as described in the passed "diffs" structure.
 */
static void set_ecc_diffs (
	struct mtd_info * const mtd,
	const struct stm_mtd_nand_ecc * const diffs)
{
	struct nand_chip * const this = (struct nand_chip *)(mtd->priv);

	this->eccmode		= diffs->nand.eccmode;
	this->eccsize		= diffs->nand.eccsize;
	this->eccsteps		= diffs->nand.eccsteps;
	this->autooob		= diffs->nand.autooob;
	this->calculate_ecc	= diffs->nand.calculate_ecc;
	this->correct_data	= diffs->nand.correct_data;

	mtd->oobavail		= diffs->mtd.oobavail;
	mtd->eccsize		= diffs->mtd.eccsize;
	memcpy(&mtd->oobinfo, &diffs->mtd.oobinfo, sizeof(struct nand_oobinfo));

	/* also, we need to reinitialize oob_buf */
	this->oobdirty		= 1;

#if VERBOSE_ECC
	printf("info: switching to NAND \"%s\" ECC (%u/%u)\n",
		(diffs==&special_ecc) ? "BOOT-mode" : "NON-boot-mode",
		this->eccbytes,
		this->eccsize);
#endif	/* VERBOSE_ECC */
}


static int set_ecc_mode (
	struct mtd_info * const mtd,
	const loff_t addr,
	const size_t len)
{
	struct nand_chip * const this = (struct nand_chip *)(mtd->priv);

	if (!done_ecc_info)		/* first time ? */
	{
		initialize_ecc_diffs (mtd);
		done_ecc_info = 1;	/* do not do this again */
	}

	/* do we need to switch ECC mode ? */
	if ( addr >= CFG_NAND_STM_BOOT_MODE_BOUNDARY )
	{	/* entire range is *not* in "boot-mode" (i.e. default ECC) */
		if (this->eccmode == NAND_ECC_HW3_128)
		{	/* we are in the wrong ECC mode, so change */
			set_ecc_diffs (mtd, &default_ecc);
		}
	}
	else if ( addr + len <= CFG_NAND_STM_BOOT_MODE_BOUNDARY )
	{	/* entire range is in "boot-mode" (i.e. 3 bytes of ECC per 128 record */
		if (this->eccmode != NAND_ECC_HW3_128)
		{	/* we are in the wrong ECC mode, so change */
			set_ecc_diffs (mtd, &special_ecc);
		}
	}
	else
	{	/* the range is split over *both* "boot" and "non-boot" modes! */
		printf("ERROR: NAND range crosses \"boot-mode\" boundary (0x%08x)\n",
			CFG_NAND_STM_BOOT_MODE_BOUNDARY);
		return -EINVAL;
	}

	return 0;	/* success */
}


extern void stm_nand_enable_hwecc (
	struct mtd_info *mtd,
	int mode)
{
	/* do nothing - we are only emulating HW in SW */
}


extern int stm_nand_read (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf)
{
	int result;

	result = set_ecc_mode (mtd, from, len);
	if (result != 0)
	{
		*retlen = 0;
	}
	else
	{
		result = nand_read_ecc (mtd, from, len, retlen, buf, NULL, NULL);
	}

	return result;
}


extern int stm_nand_read_ecc (struct mtd_info *mtd, loff_t from, size_t len,
	size_t * retlen, u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel)
{
	int result;

	result = set_ecc_mode (mtd, from, len);
	if (result != 0)
	{
		*retlen = 0;
	}
	else
	{
		result = nand_read_ecc (mtd, from, len, retlen, buf, eccbuf, oobsel);
	}

	return result;
}


extern int stm_nand_read_oob (struct mtd_info *mtd, loff_t from, size_t len, size_t * retlen, u_char * buf)
{
	int result;

	result = set_ecc_mode (mtd, from, len);
	if (result != 0)
	{
		*retlen = 0;
	}
	else
	{
		result = nand_read_oob (mtd, from, len, retlen, buf);
	}

	return result;
}


extern int stm_nand_write (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char * buf)
{
	int result;

	result = set_ecc_mode (mtd, to, len);
	if (result != 0)
	{
		*retlen = 0;
	}
	else
	{
		result = nand_write_ecc (mtd, to, len, retlen, buf, NULL, NULL);
	}

	return result;
}


extern int stm_nand_write_ecc (struct mtd_info *mtd, loff_t to, size_t len,
	size_t * retlen, const u_char * buf, u_char * eccbuf, struct nand_oobinfo *oobsel)
{
	int result;

	result = set_ecc_mode (mtd, to, len);
	if (result != 0)
	{
		*retlen = 0;
	}
	else
	{
		result = nand_write_ecc (mtd, to, len, retlen, buf, eccbuf, oobsel);
	}

	return result;
}


extern int stm_nand_write_oob (struct mtd_info *mtd, loff_t to, size_t len, size_t * retlen, const u_char *buf)
{
	int result;

	result = set_ecc_mode (mtd, to, len);
	if (result != 0)
	{
		*retlen = 0;
	}
	else
	{
		result = nand_write_oob (mtd, to, len, retlen, buf);
	}

	return result;
}


#endif	/* CFG_NAND_ECC_HW3_128 */


#ifdef CFG_NAND_FLEX_MODE	/* for STM "flex-mode" (c.f. "bit-banging") */


/* Configure NAND controller timing registers */
/* QQQ: to write & use this function (for performance reasons!) */
#ifdef QQQ
static void flex_set_timings(struct nand_timing_data * const tm)
{
	uint32_t n;
	uint32_t reg;
	uint32_t emi_clk;
	uint32_t emi_t_ns;

	/* Timings are set in units of EMI clock cycles */
	emi_clk = clk_get_rate(clk_get(NULL, "emi_master"));
	emi_t_ns = 1000000000UL / emi_clk;

	/* CONTROL_TIMING */
	n = (tm->sig_setup + emi_t_ns - 1u)/emi_t_ns;
	reg = (n & 0xffu) << 0;

	n = (tm->sig_hold + emi_t_ns - 1u)/emi_t_ns;
	reg |= (n & 0xffu) << 8;

	n = (tm->CE_deassert + emi_t_ns - 1u)/emi_t_ns;
	reg |= (n & 0xffu) << 16;

	n = (tm->WE_to_RBn + emi_t_ns - 1u)/emi_t_ns;
	reg |= (n & 0xffu) << 24;

#if DEBUG_FLEX
	printf("info: CONTROL_TIMING = 0x%08x\n", reg);
#endif
	*ST40_EMI_NAND_CTL_TIMING = reg;

	/* WEN_TIMING */
	n = (tm->wr_on + emi_t_ns - 1u)/emi_t_ns;
	reg = (n & 0xffu) << 0;

	n = (tm->wr_off + emi_t_ns - 1u)/emi_t_ns;
	reg |= (n & 0xffu) << 8;

#if DEBUG_FLEX
	printf("info: WEN_TIMING = 0x%08x\n", reg);
#endif
	*ST40_EMI_NAND_WEN_TIMING = reg;

	/* REN_TIMING */
	n = (tm->rd_on + emi_t_ns - 1u)/emi_t_ns;
	reg = (n & 0xffu) << 0;

	n = (tm->rd_off + emi_t_ns - 1u)/emi_t_ns;
	reg |= (n & 0xffu) << 8;

#if DEBUG_FLEX
	printf("info: REN_TIMING = 0x%08x\n", reg);
#endif
	*ST40_EMI_NAND_REN_TIMING = reg;
}
#endif


/*
 * hardware specific access to the Ready/not_Busy signal.
 * Signal is routed through the EMI NAND Controller block.
 */
extern int stm_flex_device_ready(struct mtd_info * const mtd)
{
	/* Apply a small delay before sampling the RBn signal */
#if 1
	ndelay(500);	/* QQQ: do we really need this ??? */
#endif
	/* extract bit 2: status of RBn pin on the FLEX bank */
	return ((*ST40_EMI_NAND_RBN_STA) & (1ul<<2)) ? 1 : 0;
}


static void init_flex_mode(void)
{
	u_int32_t reg;

	/* Disable the BOOT-mode controller */
	*ST40_EMI_NAND_BOOTBANK_CFG = 0;

	/* Perform a S/W reset the FLEX-mode controller */
	/* need to assert it for at least one (EMI) clock cycle. */
	*ST40_EMI_NAND_FLEXMODE_CFG = FLEX_CFG_SW_RESET;
	udelay(1);	/* QQQ: can we do something shorter ??? */
	*ST40_EMI_NAND_FLEXMODE_CFG = 0;

	/* Disable all interrupts in FLEX mode */
	*ST40_EMI_NAND_INT_EN = 0;

	/* Set FLEX-mode controller to enable FLEX-mode */
	*ST40_EMI_NAND_FLEXMODE_CFG = FLEX_CFG_ENABLE_FLEX_MODE;

	/*
	 * Configure (pervading) FLEX_DATA to write 4-bytes at a time.
	 * DATA is only written by write_buf(), not write_byte().
	 * Hence, we only need to configure this once (ever)!
	 * As we may be copying directly from NOR flash to NAND flash,
	 * we need to deassert the CSn after *each* access, as we
	 * can not guarantee the buffer is in RAM (or not in the EMI).
	 * Note: we could run memcpy() in write_buf() instead.
	 */
	reg = FLEX_BEAT_COUNT_4 | FLEX_1_BYTE_PER_BEAT;
	reg |= FLEX_CSn_STATUS;		/* deassert CSn after each flex data write */
#if 0
	reg |= FLEX_WAIT_RBn;		/* QQQ: do we want this ??? */
#endif
	*ST40_EMI_NAND_FLEX_DATAWRT_CFG = reg;
}


/* FLEX mode chip select: For now we only support 1 chip per
 * 'stm_nand_flex_device' so chipnr will be 0 for select, -1 for deselect.
 *
 * So, if we change device:
 *   - Set bank in mux_control_reg to data->csn
 *   - Update read/write timings (to do)
 */
extern void stm_flex_select_chip(
	struct mtd_info * const mtd,
	const int chipnr)
{
	struct nand_chip * const chip = mtd->priv;
	struct stm_nand_flex_device * data = chip->priv;

#if DEBUG_FLEX
	printf("\t\t\t\t---- SELECT = %2d ----\n", chipnr);
#endif

	if (!flex.initialized)		/* is the H/W yet to be initialized ? */
	{
		/* initialize the FLEX mode controller H/W */
		init_flex_mode();
		/* initialize the "flex" software structure */
		flex.mode          = flex_quiecent;	/* nothing pending */
		flex.next_csn      = 0;			/* start with first EMI CSn */
		flex.current_csn   = -1;		/* no NAND device selected */
							/* allocate a bounce buffer */
		flex.buf = malloc(NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);
		if (flex.buf==NULL)
		{
			printf("ERROR: Unable to allocate memory for a bounce buffer\n");
			BUG();
		}
		flex.initialized   = 1;			/* initialization done */
	}

	if (data == NULL)		/* device not yet scanned ? */
	{
#ifdef CFG_NAND_FLEX_CSn_MAP
		const int csn_map[CFG_MAX_NAND_DEVICE] = CFG_NAND_FLEX_CSn_MAP;
#endif	/* CFG_NAND_FLEX_CSn_MAP */
		int csn            = flex.next_csn++;		/* first free CSn */
		chip->priv = data  = &(flex.device[csn]);	/* first free "private" structure */
		if (csn >= CFG_MAX_NAND_DEVICE) BUG();
#ifdef CFG_NAND_FLEX_CSn_MAP
		csn                = csn_map[csn];		/* Re-map to different CSn if needed */
#endif	/* CFG_NAND_FLEX_CSn_MAP */
#if DEBUG_FLEX
		printf("info: stm_nand_flex_device.csn = %u\n", csn);
#endif

		data->csn          = csn;			/* fill in the private structure ... */
		data->mtd          = mtd;
		data->chip         = chip;
		data->timing_data  = NULL;			/* QQQ: to do */
#ifdef CFG_NAND_ECC_HW3_128
		mtd->read          = stm_nand_read;
		mtd->write         = stm_nand_write;
		mtd->read_ecc      = stm_nand_read_ecc;
		mtd->write_ecc     = stm_nand_write_ecc;
		mtd->read_oob      = stm_nand_read_oob;
		mtd->write_oob     = stm_nand_write_oob;
		chip->enable_hwecc = stm_nand_enable_hwecc;
#endif /* CFG_NAND_ECC_HW3_128 */
	}

	/* Deselect, do nothing */
	if (chipnr == -1) {
		return;

	} else if (chipnr == 0) {
		/* If same chip as last time, no need to change anything */
		if (data->csn == flex.current_csn)
			return;

		/* Set correct EMI Chip Select (CSn) on FLEX controller */
		flex.current_csn = data->csn;
		*ST40_EMI_NAND_FLEX_MUXCTRL = 1ul << data->csn;

		/* Set up timing parameters */
#if 0
		/* The default times will work for 200MHz (or slower) */
		/* QQQ: to do - BUT this is also the WRONG place to do this! */
		flex_set_timings(data->timing_data);
#endif

	} else {
		printf("ERROR: In %s() attempted to select chipnr = %d\n",
			__FUNCTION__,
			chipnr);
	}
}


extern void stm_flex_hwcontrol (
	struct mtd_info * const mtd,
	int control)
{
	switch(control) {

	case NAND_CTL_SETCLE:
#if DEBUG_FLEX
		printf("\t\t\t\t\t\t----START COMMAND----\n");
		if (flex.mode != flex_quiecent) BUG();
#endif
		flex.mode = flex_command;
		break;

#if DEBUG_FLEX
	case NAND_CTL_CLRCLE:
		printf("\t\t\t\t\t\t---- end  command----\n");
		if (flex.mode != flex_command) BUG();
		flex.mode = flex_quiecent;
		break;
#endif

	case NAND_CTL_SETALE:
#if DEBUG_FLEX
		printf("\t\t\t\t\t\t----START ADDRESS----\n");
		if (flex.mode != flex_quiecent) BUG();
#endif
		flex.mode = flex_address;
		break;

#if DEBUG_FLEX
	case NAND_CTL_CLRALE:
		printf("\t\t\t\t\t\t---- end  address----\n");
		if (flex.mode != flex_address) BUG();
		flex.mode = flex_quiecent;
		break;
#endif

#if DEBUG_FLEX
	default:
		printf("ERROR: Unexpected parameter (control=0x%x) in %s()\n",
			control,
			__FUNCTION__);
		BUG();
#endif
	}
}


/**
 * nand_read_byte - [DEFAULT] read one byte from the chip
 * @mtd:	MTD device structure
 */
extern u_char stm_flex_read_byte(
	struct mtd_info * const mtd)
{
	u_char byte;
	u_int32_t reg;

	/* read 1-byte at a time */
	reg = FLEX_BEAT_COUNT_1 | FLEX_1_BYTE_PER_BEAT;
	reg |= FLEX_CSn_STATUS;		/* deassert CSn after each flex data read */
#if 0
	reg |= FLEX_WAIT_RBn;		/* QQQ: do we want this ??? */
#endif
	*ST40_EMI_NAND_FLEX_DATA_RD_CFG = reg;

	/* read it */
	byte = (u_char)*ST40_EMI_NAND_FLEX_DATA;

#if DEBUG_FLEX
	printf("\t\t\t\t\t\t\t\t\t READ = 0x%02x\n", byte);
#endif

	/* return it */
	return byte;
}


/**
 * nand_write_byte - [DEFAULT] write one byte to the chip
 * @mtd:	MTD device structure
 * @byte:	pointer to data byte to write
 */
extern void stm_flex_write_byte(
	struct mtd_info * const mtd,
	u_char byte)
{
	u_int32_t reg;

#if DEBUG_FLEX
	printf("\t\t\t\t\t\t\t\t\tWRITE = 0x%02x\t%s\n", byte,
		(flex.mode==flex_command) ? "command" :
		((flex.mode==flex_address) ? "address" : "*UNKNOWN*"));
#endif

	switch (flex.mode)
	{
		case flex_command:
			reg = byte | FLEX_BEAT_COUNT_1;
			reg |= FLEX_CSn_STATUS;	/* deassert CSn after each flex command write */
#if 0
			reg |= FLEX_WAIT_RBn;		/* QQQ: do we want this ??? */
#endif
			*ST40_EMI_NAND_FLEX_CMD = reg;
			break;

		case flex_address:
			reg = byte | FLEX_BEAT_COUNT_1;
			reg |= FLEX_CSn_STATUS;	/* deassert CSn after each flex address write */
#if 0
			reg |= FLEX_WAIT_RBn;		/* QQQ: do we want this ??? */
#endif
			*ST40_EMI_NAND_FLEX_ADD_REG = reg;
#if 0			/* QQQ: do we need this - I think not! */
			while (!nand_device_ready()) ;	/* wait till NAND is ready */
#endif
			break;

		default:
			BUG();
	}
}


/**
 * nand_read_buf - [DEFAULT] read chip data into buffer
 * @mtd:	MTD device structure
 * @buf:	buffer to store data
 * @len:	number of bytes to read
 */
extern void stm_flex_read_buf(
	struct mtd_info * const mtd,
	u_char * const buf,
	const int len)
{
	int i;
	uint32_t *p;
	u_int32_t reg;

	/* our buffer needs to be 4-byte aligned, for the FLEX controller */
	p = ((uint32_t)buf & 0x3) ? (void*)flex.buf : (void*)buf;

#if DEBUG_FLEX
	printf("info: stm_flex_read_buf( buf=%p, len=0x%x )\t\tp=%p%s\n",
		buf, len, p,
		((uint32_t)buf & 0x3) ? "\t\t**** UN-ALIGNED *****" : "");
#endif

	/* configure to read 4-bytes at a time */
	reg = FLEX_BEAT_COUNT_4 | FLEX_1_BYTE_PER_BEAT;
	reg |= FLEX_CSn_STATUS;		/* deassert CSn after each flex data read */
#if 0
	reg |= FLEX_WAIT_RBn;		/* QQQ: do we want this ??? */
#endif
	*ST40_EMI_NAND_FLEX_DATA_RD_CFG = reg;

	/* copy the data (from NAND) as 4-byte words ... */
	for(i=0; i<len/4; i++)
	{
		p[i] = *ST40_EMI_NAND_FLEX_DATA;
	}

	/* copy back into user-supplied buffer, if it was unaligned */
	if ((void*)p != (void*)buf)
		memcpy(buf, p, len);

#if DEBUG_FLEX
	printf("READ BUF\t\t\t\t");
	for (i=0; i<16; i++)
		printf("%02x ", buf[i]);
	printf("...\n");
#endif
}


/**
 * nand_write_buf - [DEFAULT] write buffer to chip
 * @mtd:	MTD device structure
 * @buf:	data buffer
 * @len:	number of bytes to write
 */
extern void stm_flex_write_buf(
	struct mtd_info * const mtd,
	const u_char * const buf,
	const int len)
{
	int i;
	uint32_t *p;

#if DEBUG_FLEX
	printf("WRITE BUF\t\t");
	for (i=0; i<16; i++)
		printf("%02x ", buf[i]);
	printf("...\n");
#endif

	/* our buffer needs to be 4-byte aligned, for the FLEX controller */
	p = ((uint32_t)buf & 0x3) ? (void*)flex.buf : (void*)buf;

	/* copy from user-supplied buffer, if it is unaligned */
	if ((void*)p != (void*)buf)
		memcpy(p, buf, len);

	/* configured to write 4-bytes at a time */
	/* copy the data (to NAND) as 32-bit words ... */
	for(i=0; i<len/4; i++)
	{
		*ST40_EMI_NAND_FLEX_DATA = p[i];
	}
}


#endif /* CFG_NAND_FLEX_MODE */


#endif	/* CONFIG_CMD_NAND */
