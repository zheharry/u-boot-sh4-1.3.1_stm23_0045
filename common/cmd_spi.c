/*
 * (C) Copyright 2002
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 * (C) Copyright 2009 STMicroelectronics.  Sean McGoogan <Sean.McGoogan@st.com>
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

/*
 * SPI Read/Write Utilities
 */

#include <common.h>
#include <command.h>
#include <spi.h>
#include <linux/ctype.h>

/*-----------------------------------------------------------------------
 * Definitions
 */

#ifndef MAX_SPI_BYTES
#   define MAX_SPI_BYTES 32	/* Maximum number of bytes we can handle */
#endif

/*
 * Values from last command.
 */
static int   device;
static int   bitlen;
static uchar dout[MAX_SPI_BYTES];
static uchar din[MAX_SPI_BYTES];

/*
 * SPI read/write
 *
 * Syntax:
 *   spi {dev} {num_bits} {dout}
 *     {dev} is the device number for controlling chip select (see TBD)
 *     {num_bits} is the number of bits to send & receive (base 10)
 *     {dout} is a hexadecimal string of data to send
 * The command prints out the hexadecimal string received via SPI.
 */

int do_spi (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	uchar  *cp = 0;
	uchar tmp;
	int   j;
	int   rcode = 0;

	/*
	 * We use the last specified parameters, unless new ones are
	 * entered.
	 */

	if ((flag & CMD_FLAG_REPEAT) == 0)
	{
		if (argc >= 2)
			device = simple_strtoul(argv[1], NULL, 10);
		if (argc >= 3)
			bitlen = simple_strtoul(argv[2], NULL, 10);
		if (argc >= 4) {
			cp = (uchar*)argv[3];
			for(j = 0; *cp; j++, cp++) {
				tmp = *cp - '0';
				if(tmp > 9)
					tmp -= ('A' - '0') - 10;
				if(tmp > 15)
					tmp -= ('a' - 'A');
				if(tmp > 15) {
					return 1;
				}
				if((j % 2) == 0)
					dout[j / 2] = (tmp << 4);
				else
					dout[j / 2] |= tmp;
			}
		}
	}

	if ((device < 0) || (device >=  spi_chipsel_cnt)) {
		return 1;
	}
	if ((bitlen < 0) || (bitlen >  (MAX_SPI_BYTES * 8))) {
		return 1;
	}


	if(spi_xfer(spi_chipsel[device], bitlen, dout, din) != 0) {
		rcode = 1;
	} else {
		cp = din;
	}

	return rcode;
}

int do_update_spi (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned char StartAddr[10]={0};
	unsigned char *DataBufferAddr=NULL;
	char *str_filesize = NULL;
	unsigned long file_size=0;
	
	DataBufferAddr=(unsigned char *)0x80000000;

	str_filesize = getenv("filesize");
	if (NULL == str_filesize) {
	}

	file_size = simple_strtoul(str_filesize, NULL, 16);
	spi_write(StartAddr, 1, DataBufferAddr , file_size);
	return 0;	
}

extern int do_auto_update_uboot_to_spi (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned char StartAddr[10]={0};
	unsigned char *DataBufferAddr=NULL;
	char *str_filesize = NULL;
	unsigned long file_size=0;
	
	DataBufferAddr=(unsigned char *)(0x80000000+argc);

	file_size =flag;
	spi_write(StartAddr, 1, DataBufferAddr , file_size);
	return 0;
	
}

extern unsigned char ReadSPIFlashDataByChar(unsigned int AddressOfFlag)
{
	unsigned char  CharData;
	unsigned char  StartAddr[3];

	StartAddr[0]=(AddressOfFlag>>16)&0xff;
	StartAddr[1]=(AddressOfFlag>>8)&0xff;
	StartAddr[2]=AddressOfFlag&0xff;
	
	spi_read(StartAddr, 3, &CharData, 1);

	return CharData;
}

extern unsigned char WriteSPIFlashDataByChar(unsigned int AddressOfFlag, unsigned char Data)
{
	unsigned char  CharData;
	unsigned char  StartAddr[3];

	CharData=Data;
	StartAddr[0]=(AddressOfFlag>>16)&0xff;
	StartAddr[1]=(AddressOfFlag>>8)&0xff;
	StartAddr[2]=AddressOfFlag&0xff;
		
	spi_write(StartAddr, 3, &CharData, 1);
	
	return 0;
}


extern unsigned char ReadSPIFlashDataToBuffer(unsigned int AddressOfFlag, unsigned char *buffer, unsigned int length)
{
	unsigned char  CharData;
	unsigned char  StartAddr[3];

	StartAddr[0]=(AddressOfFlag>>16)&0xff;
	StartAddr[1]=(AddressOfFlag>>8)&0xff;
	StartAddr[2]=AddressOfFlag&0xff;
	
	spi_read(StartAddr, 3, buffer, length);

	return CharData;
}

extern unsigned char WriteSPIFlashDataFromBuffer(unsigned int AddressOfFlag, unsigned char *buffer, unsigned int length)
{
	unsigned char  CharData;

	unsigned char  StartAddr[3];
	
	StartAddr[0]=(AddressOfFlag>>16)&0xff;
	StartAddr[1]=(AddressOfFlag>>8)&0xff;
	StartAddr[2]=AddressOfFlag&0xff;
		
	spi_write(StartAddr, 3, buffer, length);
	
	return 0;
}

int do_update_flag (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned int PushUpdateFlag=0xfbcdbcdf;
	unsigned int StartAddr=0x90000;  

	WriteSPIFlashDataFromBuffer(StartAddr, &PushUpdateFlag, 4);
	return 0;
}

extern void EraseSPIDataByAddr(unsigned int Addr, unsigned int len)
{
	unsigned int StartAddr;
	unsigned char Buffer[0x10000];

	memset(Buffer, 0xff, 0x10000);
	StartAddr =Addr ;
	
	WriteSPIFlashDataFromBuffer(StartAddr, Buffer, len);

}

static void  do_EraseSPIDataByAddr(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int nBlockNum=-1;
	unsigned int StartAddr=0x80000;
	unsigned int Len=0;
	unsigned char Buffer[0x10000];

	memset(Buffer, 0xff, 0x10000);
	StartAddr = simple_strtoul(argv[1], NULL, 16);
	Len = simple_strtoul(argv[2], NULL, 16);

	WriteSPIFlashDataFromBuffer(StartAddr, Buffer, Len);	
}
static void  do_ChangeFlag(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

	unsigned int StartAddr=0x90000;
    unsigned int date=0;

	date = simple_strtoul(argv[1], NULL, 10);
    WriteSPIFlashDataByChar(StartAddr, date);	
}
static void  do_ChangeMacAddr(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int nBlockNum=-1;
	unsigned int Len=0;
	unsigned char Buffer[30];
	unsigned int 	StartAddr=0xa0000+19;
	unsigned int   Time=0;
	unsigned int j=0;

	memset(Buffer, 0xff, sizeof(Buffer));

	j=0;
	for(Time=0; Time<12; Time++)
		{
		     for(; j<100;)
			{
				if(isxdigit(argv[1][j]))
					break;
				else
					{
						j++;
						continue;
					}
			}
		     
			Buffer[Time]=argv[1][j];
			j++;
		}

	WriteSPIFlashDataFromBuffer(StartAddr, Buffer, 12);	
}


/***************************************************/

U_BOOT_CMD(
	sspi,	5,	1,	do_spi,
	"sspi    - SPI utility commands\n",
	"<device> <bit_len> <dout> - Send <bit_len> bits from <dout> out the SPI\n"
	"<device>  - Identifies the chip select of the device\n"
	"<bit_len> - Number of bits to send (base 10)\n"
	"<dout>    - Hexadecimal string that gets sent\n"
);

U_BOOT_CMD(
	auto_update_uboot_to_spi,	1,	1,	do_auto_update_uboot_to_spi,
	"auto_update_uboot_to_spi - Update new u-boo.bin to SPI Flash\n",
	NULL
	);

U_BOOT_CMD(
	update_spi_uboot,	1,	1,	do_update_spi,
	"update_spi_uboot - Update uboot in SPI Flash\n",
	NULL
	);

U_BOOT_CMD(
	set_pushupdate_flag,	1,	1,	do_update_flag,
	"set_pushupdate_flag - set update_uboot_flag in SPI Flash\n",
	NULL
	);

U_BOOT_CMD(
	eraseSPIData,	 3,	1,	do_EraseSPIDataByAddr,
	"eraseSPIData  addr len - set ff to SPI Flash\n",
	NULL
	);

U_BOOT_CMD(
	changeMacAddr,	 2,	1,	do_ChangeMacAddr,
	"changeMacAddr  addr  - set mac addr to SPI Flash\n",
	NULL
	);

U_BOOT_CMD(
	changeflag,	 2,	1,	do_ChangeFlag,
	"changeflag  date - change flag to SPI Flash  ep:changeflag 255\n",
	NULL
	);	

