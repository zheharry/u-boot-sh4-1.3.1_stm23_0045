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

#include <nand.h>


extern struct nand_bbt_descr stm_nand_badblock_pattern_16;
extern struct nand_bbt_descr stm_nand_badblock_pattern_64;

extern int stm_nand_default_bbt (
	struct mtd_info *mtd);


#ifdef CFG_NAND_ECC_HW3_128	/* for STM "boot-mode" */


extern int stm_nand_calculate_ecc (
	struct mtd_info * const mtd,
	const u_char * const dat,
	u_char * const ecc_code);

extern int stm_nand_correct_data (
	struct mtd_info *mtd,
	u_char *dat,
	u_char *read_ecc,
	u_char *calc_ecc);

extern void stm_nand_enable_hwecc (
	struct mtd_info *mtd,
	int mode);


extern int stm_nand_read (
	struct mtd_info *mtd,
	loff_t from,
	size_t len,
	size_t * retlen,
	u_char * buf);

extern int stm_nand_read_ecc (
	struct mtd_info *mtd,
	loff_t from,
	size_t len,
	size_t * retlen,
	u_char * buf,
	u_char * eccbuf,
	struct nand_oobinfo *oobsel);

extern int stm_nand_read_oob (
	struct mtd_info *mtd,
	loff_t from,
	size_t len,
	size_t * retlen,
	u_char * buf);


extern int stm_nand_write (
	struct mtd_info *mtd,
	loff_t to,
	size_t len,
	size_t * retlen,
	const u_char * buf);

extern int stm_nand_write_ecc (
	struct mtd_info *mtd,
	loff_t to,
	size_t len,
	size_t * retlen,
	const u_char * buf,
	u_char * eccbuf,
	struct nand_oobinfo *oobsel);

extern int stm_nand_write_oob (
	struct mtd_info *mtd,
	loff_t to,
	size_t len,
	size_t * retlen,
	const u_char *buf);


#endif	/* CFG_NAND_ECC_HW3_128 */


#ifdef CFG_NAND_FLEX_MODE	/* for STM "flex-mode" (c.f. "bit-banging") */


extern int stm_flex_device_ready(
	struct mtd_info * const mtd);

extern void stm_flex_select_chip (
	struct mtd_info * const mtd,
	const int chipnr);

extern void stm_flex_hwcontrol (
	struct mtd_info * const mtd,
	int control);


extern u_char stm_flex_read_byte(
	struct mtd_info * const mtd);

extern void stm_flex_write_byte(
	struct mtd_info * const mtd,
	u_char byte);


extern void stm_flex_read_buf(
	struct mtd_info * const mtd,
	u_char * const buf,
	const int len);

extern void stm_flex_write_buf(
	struct mtd_info * const mtd,
	const u_char *buf,
	const int len);


#endif /* CFG_NAND_FLEX_MODE */


