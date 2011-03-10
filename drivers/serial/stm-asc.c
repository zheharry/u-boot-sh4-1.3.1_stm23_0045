/*
 * drivers/serial/stm-asc.c
 *
 * Support for Serial I/O using STMicroelectronics' on-chip ASC.
 *
 *  Copyright (c) 2004,2008  STMicroelectronics Limited
 *  Sean McGoogan <Sean.McGoogan@st.com>
 *  Copyright (C) 1999  Takeshi Yaegachi & Niibe Yutaka
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License.  See the file "COPYING.LIB" in the main
 * directory of this archive for more details.
 *
 */

#include "common.h"
#include <command.h>

#ifdef CONFIG_STM_ASC_SERIAL

#include "asm/termbits.h"
#include "asm/io.h"
#include "asm/pio.h"
#include "asm/socregs.h"
#include "asm/clk.h"
#define SYS_CODE1						0X00
#define SYS_CODE2						0XFF

#define RMT_PS_KEY_VAL			0x0A

#define STA									0X73
#define KEY									0X6B
#define RMT									0x72
#define KEY_CTN							0X4B
#define RMT_CTN							0X52
#define GMT0								0x48
#define GMT1								0x49
#define GMT2								0x50
#define GMT3								0x51
#define GMT_NULL						0x4e
//----------------------------------
//09.02.27
#define	PS_ON_INFO					0x70
#define	SW_51_VER						0x76


#define OK									0XF3
#define ER									0XF4
#define RJCT								0xF5
#define N_PROC							0xF6

#define START_BYTE					0XE0

#define SYS_CODE_TAG				0X01
#define PS_KEY_VAL_TAG			0X02
#define SYS_DSPL_INFO_TAG		0X03
#define WAKE_TIME_TAG				0X04
#define SYS_TIME_TAG				0X05
//------------------------------
//09.02.27//change
#define HOST_CMD_TAG				0X06
//------------------------------
#define GMT_TIME_TAG				0X07
//------------------------------

#define PS_OFF_CMD					0X55
#define PS_RST_CMD					0XAA
//------------------------------
//09.02.27 add
#define TX_GMT_CMD					0X03
#define PS_HOW_ON_CMD				0X04
//ÒÔÏÂÃüÁîÑ¡ÓÃ...
#define GET_VER_CMD					0X05
#define SHOW_M_H_CMD				0X06
#define SHOW_S_M_CMD				0X07
#define SHOW_L_INFO_CMD			0X08
//09.03.04 add...
#define ON_USB_LED_CMD			0X09
#define OFF_USB_LED_CMD			0X0a
//09.03.06 add...
#define EN_HBT_USB_LED_CMD	0X0B
#define DIS_HBT_USB_LED_CMD 0X0C


#define PS_AC_ON						0X01
#define PS_KB_ON						0X02
#define PS_RMT_ON						0X03
#define	PS_SCH_ON						0X04
#define	PS_RST_ON						0X05
#define PS_TST_ON						0X06

#define FPN_KEY_NULL				0x00
#define FPN_KEY_POWER				0x01
#define FPN_KEY_MENU				0x02
#define FPN_KEY_OK					0x04
#define FPN_KEY_EXIT				0x08
#define FPN_KEY_VOLN				0x10
#define FPN_KEY_VOLP				0x20
#define FPN_KEY_CHN					0x40
#define FPN_KEY_CHP					0x80

#define INI_VECTORLo				0x00
#define INI_VECTORHi				0x00
#define POLYNOMIALHi				0x80
#define POLYNOMIALLo				0x05

#define SET_APP_RUN_FLG_CMD	0X0D
#define CLS_APP_RUN_FLG_CMD	0X0E

#define FPN_ERR_SYMBOL			0xac
#define CS7		0000040
#define CS8		0000060
#define CSIZE		0000060
#define CSTOPB		0000100
#define CREAD		0000200
#define PARENB		0000400
#define PARODD		0001000
#define HUPCL		0002000
#define CLOCAL		0004000

#define BAUDMODE	0x00001000
#define BAUDMODE_PIO5	0x00000000
#define CTSENABLE	0x00000800
#define RXENABLE	0x00000100
#define RUN		0x00000080
#define LOOPBACK	0x00000000
#define STOPBIT		0x00000008
#define MODE		0x00000001
#define MODE_7BIT_PAR	0x0003
#define MODE_8BIT_PAR	0x0007
#define MODE_8BIT	0x0001
#define STOP_1BIT	0x0008
#define PARITYODD	0x0020

#define STA_NKD		0x0400
#define STA_TF		0x0200
#define STA_RHF		0x0100
#define STA_TOI		0x0080
#define STA_TNE		0x0040
#define STA_OE		0x0020
#define STA_FE		0x0010
#define STA_PE		0x0008
#define	STA_THE		0x0004
#define STA_TE		0x0002
#define STA_RBF		0x0001


#define UART_BAUDRATE_OFFSET	0x00
#define UART_TXBUFFER_OFFSET	0x04
#define UART_RXBUFFER_OFFSET	0x08
#define UART_CONTROL_OFFSET	0x0C
#define UART_INTENABLE_OFFSET	0x10
#define UART_STATUS_OFFSET	0x14
#define UART_GUARDTIME_OFFSET	0x18
#define UART_TIMEOUT_OFFSET	0x1C
#define UART_TXRESET_OFFSET	0x20
#define UART_RXRESET_OFFSET	0x24
#define UART_RETRIES_OFFSET	0x28

#define UART_BAUDRATE_REG	(CFG_STM_ASC_BASE + UART_BAUDRATE_OFFSET)
#define UART_TXBUFFER_REG	(CFG_STM_ASC_BASE + UART_TXBUFFER_OFFSET)
#define UART_RXBUFFER_REG	(CFG_STM_ASC_BASE + UART_RXBUFFER_OFFSET)
#define UART_CONTROL_REG	(CFG_STM_ASC_BASE + UART_CONTROL_OFFSET)
#define UART_INTENABLE_REG	(CFG_STM_ASC_BASE + UART_INTENABLE_OFFSET)
#define UART_STATUS_REG		(CFG_STM_ASC_BASE + UART_STATUS_OFFSET)
#define UART_GUARDTIME_REG	(CFG_STM_ASC_BASE + UART_GUARDTIME_OFFSET)
#define UART_TIMEOUT_REG	(CFG_STM_ASC_BASE + UART_TIMEOUT_OFFSET)
#define UART_TXRESET_REG	(CFG_STM_ASC_BASE + UART_TXRESET_OFFSET)
#define UART_RXRESET_REG	(CFG_STM_ASC_BASE + UART_RXRESET_OFFSET)
#define UART_RETRIES_REG	(CFG_STM_ASC_BASE + UART_RETRIES_OFFSET)

#define CFG_STM_ASC_BASE_PIO5  0xFD033000
#define UART_BAUDRATE_REG_PIO5   (CFG_STM_ASC_BASE_PIO5+ UART_BAUDRATE_OFFSET)
#define UART_TXBUFFER_REG_PIO5   (CFG_STM_ASC_BASE_PIO5+ UART_TXBUFFER_OFFSET)
#define UART_RXBUFFER_REG_PIO5   (CFG_STM_ASC_BASE_PIO5+ UART_RXBUFFER_OFFSET)
#define UART_CONTROL_REG_PIO5    (CFG_STM_ASC_BASE_PIO5+ UART_CONTROL_OFFSET)
#define UART_INTENABLE_REG_PIO5  (CFG_STM_ASC_BASE_PIO5+ UART_INTENABLE_OFFSET)
#define UART_STATUS_REG_PIO5     (CFG_STM_ASC_BASE_PIO5+ UART_STATUS_OFFSET)
#define UART_GUARDTIME_REG_PIO5  (CFG_STM_ASC_BASE_PIO5+ UART_GUARDTIME_OFFSET)
#define UART_TIMEOUT_REG_PIO5    (CFG_STM_ASC_BASE_PIO5+ UART_TIMEOUT_OFFSET)
#define UART_TXRESET_REG_PIO5    (CFG_STM_ASC_BASE_PIO5+ UART_TXRESET_OFFSET)
#define UART_RXRESET_REG_PIO5    (CFG_STM_ASC_BASE_PIO5+ UART_RXRESET_OFFSET)
#define UART_RETRIES_REG_PIO5    (CFG_STM_ASC_BASE_PIO5+ UART_RETRIES_OFFSET)

/*---- Values for the BAUDRATE Register -----------------------*/

#if defined(__SH4__)
#define PCLK			(get_peripheral_clk_rate())
#define BAUDRATE_VAL_M0(bps)	(PCLK / (16 * (bps)))
#define BAUDRATE_VAL_M1(bps)	((((bps * (1 << 14))+ (1<<13)) / (PCLK/(1 << 6))))
#else	/* !defined(__SH4__) */
#define PCLK			B_CLOCK_RATE
#define BAUDRATE_VAL_M0(bps)	(PCLK / (16 * (bps)))
#define BAUDRATE_VAL_M1(bps)	(int)((((double)bps * (1 << 20))/ PCLK)+0.5)
#endif	/* defined(__SH4__) */

/*
 * MODE 0
 *                       ICCLK
 * ASCBaudRate =   ----------------
 *                   baudrate * 16
 *
 * MODE 1
 *                   baudrate * 16 * 2^16
 * ASCBaudRate =   ------------------------
 *                          ICCLK
 *
 * NOTE:
 * Mode 1 should be used for baudrates of 19200, and above, as it
 * has a lower deviation error than Mode 0 for higher frequencies.
 * Mode 0 should be used for all baudrates below 19200.
 */


#if defined(CONFIG_STM_ST231)
#define p2_outl(addr,b) writel(b,addr)
#define p2_inl(addr)    readl(addr)
#endif


#ifdef CONFIG_HWFLOW
static int hwflow = 0;		/* turned off by default */
#endif	/* CONFIG_HWFLOW */


/* busy wait until it is safe to send a char */
static inline void TxCharReady (void)
{
	unsigned long status;

	do {
		status = p2_inl (UART_STATUS_REG);
	} while (status & STA_TF);
}
static unsigned int controlReg2=0;
/* initialize the ASC */
extern int serial_init (void)
{
	DECLARE_GLOBAL_DATA_PTR;
	const int cflag = CREAD | HUPCL | CLOCAL | CSTOPB | CS8 | PARODD;
	unsigned long val;
	int baud = gd->baudrate;
	int t, mode=1;

	switch (baud) {
	case 9600:
		t = BAUDRATE_VAL_M0(9600);
		mode = 0;
		break;
	case 19200:
		t = BAUDRATE_VAL_M1(19200);
		break;
	case 38400:
		t = BAUDRATE_VAL_M1(38400);
		break;
	case 57600:
		t = BAUDRATE_VAL_M1(57600);
		break;
	default:
		printf ("ASC: unsupported baud rate: %d, using 115200 instead.\n", baud);
	case 115200:
		t = BAUDRATE_VAL_M1(115200);
		break;
	}

	/* wait for end of current transmission */
	TxCharReady ();

	/* disable the baudrate generator */
	val = p2_inl (UART_CONTROL_REG);
	p2_outl (UART_CONTROL_REG, (val & ~RUN));

	/* set baud generator reload value */
	p2_outl (UART_BAUDRATE_REG, t);

	/* reset the RX & TX buffers */
	p2_outl (UART_TXRESET_REG, 1);
	p2_outl (UART_RXRESET_REG, 1);

	/* build up the value to be written to CONTROL */
	val = RXENABLE | RUN;

	/* set character length */
	if ((cflag & CSIZE) == CS7)
		val |= MODE_7BIT_PAR;
	else {
		if (cflag & PARENB)
			val |= MODE_8BIT_PAR;
		else
			val |= MODE_8BIT;
	}

	/* set stop bit */
	/* it seems no '0 stop bits' option is available: by default
	 * we get 0.5 stop bits */
	if (cflag & CSTOPB)
		val |= STOP_1BIT;

	/* odd parity */
	if (cflag & PARODD)
		val |= PARITYODD;

#ifdef CONFIG_HWFLOW
	/*  set flow control */
	if (hwflow)
		val |= CTSENABLE;
#endif	/* CONFIG_HWFLOW */

	/* set baud generator mode */
	if (mode)
		val |= BAUDMODE;

	/* finally, write value and enable ASC */
	p2_outl (UART_CONTROL_REG, val);
	controlReg2=val;
	return 0;
}

int serial_init_pio5 (void)
#if 1
{
	DECLARE_GLOBAL_DATA_PTR;
	const int cflag = CREAD | HUPCL | CLOCAL | CSTOPB | CS8 | PARODD;
	unsigned long val;
	int baud = gd->baudrate;
	int t, mode=1;


	baud=9600;
    
	switch (baud) {
	case 9600:
		t = BAUDRATE_VAL_M0(9600);
		mode = 0;
		break;
	case 19200:
		t = BAUDRATE_VAL_M1(19200);
		break;
	case 38400:
		t = BAUDRATE_VAL_M1(38400);
		break;
	case 57600:
		t = BAUDRATE_VAL_M1(57600);
		break;
	default:
		printf ("ASC: unsupported baud rate: %d, using 115200 instead.\n", baud);
	case 115200:
		t = BAUDRATE_VAL_M1(115200);
		break;
	}

	/* wait for end of current transmission */
	TxCharReady ();

	/* disable the baudrate generator */
	val = p2_inl (UART_CONTROL_REG_PIO5);
	p2_outl (UART_CONTROL_REG_PIO5, (val & ~RUN));

	/* set baud generator reload value */
	p2_outl (UART_BAUDRATE_REG_PIO5, t);

	/* reset the RX & TX buffers */
	p2_outl (UART_TXRESET_REG_PIO5, 1);
	p2_outl (UART_RXRESET_REG_PIO5, 1);

	/* build up the value to be written to CONTROL */
	val = RXENABLE | RUN;

	/* set character length */
	if ((cflag & CSIZE) == CS7)
		val |= MODE_7BIT_PAR;
	else {
		if (cflag & PARENB)
			val |= MODE_8BIT_PAR;
		else
			val |= MODE_8BIT;
	}

	/* set stop bit */
	/* it seems no '0 stop bits' option is available: by default
	 * we get 0.5 stop bits */
	if (cflag & CSTOPB)
		val |= STOP_1BIT;

	/* odd parity */
	if (cflag & PARODD)
		val |= PARITYODD;

#ifdef CONFIG_HWFLOW
	/*  set flow control */
	if (hwflow)
		val |= CTSENABLE;
#endif	/* CONFIG_HWFLOW */

	/* set baud generator mode */
	if (mode)
		val |= BAUDMODE;

	/* finally, write value and enable ASC */
	p2_outl (UART_CONTROL_REG_PIO5, val);

	return 0;
}
#endif

/* returns TRUE if a char is available, ready to be read */
extern int serial_tstc (void)
{
	unsigned long status;

	status = p2_inl (UART_STATUS_REG);
	return (status & STA_RBF);
}

/* blocking function, that returns next char */
extern int serial_getc (void)
{
	char ch;

	/* polling wait: for a char to be read */
	while (!serial_tstc ());

	/* read char, now that we know we have one */
	ch = p2_inl (UART_RXBUFFER_REG);

	/* return consumed char to the caller */
	return ch;
}

/* write write out a single char */
extern void serial_putc (char ch)
{
	/* Stream-LF to CR+LF conversion */
	if (ch == 10)
		serial_putc ('\r');

	/* wait till safe to write next char */
	TxCharReady ();

	/* finally, write next char */
	p2_outl (UART_TXBUFFER_REG, ch);
}

/* write an entire (NUL-terminated) string */
extern void serial_puts (const char *s)
{
	while (*s) {
		serial_putc (*s++);
	}
}

/* called to adjust baud-rate */
extern void serial_setbrg (void)
{
	/* just re-initialize ASC */
	serial_init ();
}

int serial_tstc_pio5 (void)
{
	unsigned short status;

	status = p2_inl (UART_STATUS_REG_PIO5);
	return (status & STA_RBF);
}

/* This function doesn't correctly work */
int serial_getc_pio5 (void)
{
	char ch;

	while (!serial_tstc_pio5 ());

	ch = p2_inl (UART_RXBUFFER_REG_PIO5);

	return ch;
}

static inline int putDebugCharReady_pio5 (void)
{
	unsigned long status;

	status = p2_inl (UART_STATUS_REG_PIO5);
	return !(status & STA_TF);
}

void serial_putc_pio5 (char ch)
{
	if (ch == 10)
		serial_putc_pio5 ('\r');
	while (!putDebugCharReady_pio5 ());
	p2_outl (UART_TXBUFFER_REG_PIO5, ch);
}

void serial_puts_pio5 (const char *s)
{
	while (*s) {
		serial_putc_pio5 (*s++);
	}
}

static void send_8byte(unsigned char *buff)
{		
	unsigned char i;
	int rc;
	
	for(i = 0; i < 8; i++)
	{
		serial_putc_pio5(buff[i]);
	}
}

unsigned char decode(unsigned char ch)
{
	unsigned char	code;
	
	switch(ch)
	{
		case '0':
		case 'O':
			code = 0xc0;
			break;
		case '1':
		case 'l':
		case 'I':
		case 'i':
			code = 0xf9;
			break;
		case '2':
			code = 0xa4;
			break;
		case '3':
			code = 0xb0;
			break;
		case '4':
			code = 0x99;
			break;
		case '5':
		case 'S':
		case 's':
			code = 0x92;
			break;
		case '6':
			code = 0x82;
			break;
		case '7':
			code = 0xF8;
			break;
		case '8':
			code = 0x80;
			break;
		case '9':
		case 'G':
		case 'g':
			code = 0x90;
			break;
		case 'a':
		case 'A':
			code = 0x88;
			break;
		case 'b':
		case 'B':
			code = 0x83;
			break;
		case 'c':
			code = 0xa7;
			break;
		case 'C':
			code = 0xc6;
			break;
		case 'd':
		case 'D':
			code = 0xa1;
			break;
		case 'E':
		case 'e':
			code = 0x86;
			break;
		case 'F':
		case 'f':
			code = 0x8e;
			break;
		case 'P':
		case 'p':
			code = 0x8c;
			break;
		case 'U':
		case 'V':
			code = 0xC1;
			break;
		case 'u':
		case 'v':
			code = 0xe3;
			break;
		case 'q':
		case 'Q':
			code = 0x98;
			break;
		case 'R':
		case 'r':
			code = 0x8f;
			break;
		case '.':
			code = 0x7f;
			break;
		case ' ':
			code = 0xff;
			break;
		case 'h':
			code = 0x8b;
			break;
		case 'H':
			code = 0x89;
			break;
		case 'J':
		case 'j':
			code = 0xf1;
			break;
		case 'L':
			code = 0xC7;
			break;
		case 'n':
			code = 0xab;
			break;
		case 'N':
			code = 0xC8;
			break;
		case 'o':
			code = 0xa3;
			break;
		case 'y':
		case 'Y':
			code = 0x91;
			break;
		case 't':
		case 'T':
			code = 0x87;
			break;
		case '?':
			code = FPN_ERR_SYMBOL;
			break;

		default:
			code = 0xff;
			printf("The [%c] can not display on fpn!\n\r", (char)ch);
			break;
	}
	return code;
}

/*
Call this function to set the content to be displayed on the fpn.
for example, "f.1.08".
*/
unsigned char  decode_str(unsigned char *fpn_str, unsigned char *decode_buf/*4 bytes*/)
{
	unsigned char rc = 0;
	int	fpn_str_len;
	int	f, l, i;
	char	local_str[9];
	char	local_str2[9];
	
	for(i = 0; i < 9; i ++)
	{
		local_str[i] = ' ';
	}	
	fpn_str_len = strlen(fpn_str); 
	
	for(f = 0, l = 0; f < fpn_str_len && l < 9; )
	{
		if('.' == fpn_str[f])
		{
			if(f == 0)
			{
				local_str[l] = ' ';
				l ++;
				local_str[l] = '.';
				l ++;
			}
			else
			{
				if(fpn_str[f - 1] == '.')
				{
					local_str[l] = ' ';
					l ++;
					local_str[l] = '.';
					l ++;
				}
				else
				{
					local_str[l] = '.';
					l ++;
				}

			}
		}
		else
		{
			if(f == (fpn_str_len - 1))
			{
				local_str[l] = fpn_str[f];
				l ++;
				local_str[l] = ' ';
				l ++;
				break;
			}
			else
			{
				if(fpn_str[f + 1] == '.')
				{
					local_str[l] = fpn_str[f];
					l ++;
				}
				else
				{
					local_str[l] = fpn_str[f];
					l ++;
					local_str[l] = ' ';
					l ++;
				}
			}
		}
		
		f ++;
	}

	local_str[8]=0;
	for(i = 0; i < 4; i ++)
	{
		decode_buf[i] = decode(local_str[2 * i]);
		
		decode_buf[i] |= 0x80;

		if('.' == local_str[2 * i + 1])
		decode_buf[i] &= 0x7f;
		
		if(decode_buf[i] == FPN_ERR_SYMBOL)
		{
			rc ++;//error increase
		}
		local_str2[i] = decode_buf[i];
	}
	
	return rc;
}

unsigned short get_crc16(unsigned char *data_blk_ptr,unsigned char data_blk_size)
{       
	unsigned short crc_return;   
	unsigned char CRC16Lo=INI_VECTORLo;
	unsigned char CRC16Hi=INI_VECTORHi;
	unsigned char SaveHi,SaveLo;
	unsigned char i, j;
	for (i = 0; i < data_blk_size; i++)
	{
		CRC16Lo = CRC16Lo ^ *data_blk_ptr++;
		for ( j = 0;  j < 8;  j++ )
		{
			SaveHi = CRC16Hi;
			SaveLo = CRC16Lo;
			CRC16Hi = CRC16Hi >>1;
			CRC16Lo = CRC16Lo >>1;
			if ((SaveHi & 0x1) == 0x1)
			{ 
				CRC16Lo = CRC16Lo | 0x80;
			}
			if ((SaveLo & 0x1) == 0x1) 
			{ 
				CRC16Hi = CRC16Hi ^ POLYNOMIALHi;
				CRC16Lo = CRC16Lo ^ POLYNOMIALLo;
			}
		}
	}
	crc_return=CRC16Hi * 256 + CRC16Lo;
	return (crc_return); 
}

void send_sys_code(void)
{
	unsigned char out[8], in[2];
	unsigned short crc;
	
	out[0] = START_BYTE;
	out[1] = SYS_CODE_TAG;
	
	out[2] = SYS_CODE1;
	out[3] = SYS_CODE2;
	out[4] = 0xff;
	out[5] = 0xff;

	crc = get_crc16(out, 6);
	
	out[6] = crc & 0xff;
	out[7] = (crc >> 8) & 0xff;

	send_8byte(out);
}

extern void send_dspl_info(unsigned char *str)
{

}

void F450_enable(void)
{
	int	ErrorCode;
	unsigned char 	out[8];
	unsigned short	crc;
	
	out[0] = START_BYTE;
	out[1] = HOST_CMD_TAG;
	
	out[2] = SET_APP_RUN_FLG_CMD;
	out[3] = 0xff;
	out[4] = 0xff;
	out[5] = 0xff;

	crc = get_crc16(out, 6);
	
	out[6] = crc & 0xff;
	out[7] = (crc >> 8) & 0xff;

	send_8byte(out);	
}

void F450_disable(void)
{
	int	ErrorCode;
	unsigned char 	out[8];
	unsigned short	crc;
	
	out[0] = START_BYTE;
	out[1] = HOST_CMD_TAG;
	
	out[2] = CLS_APP_RUN_FLG_CMD;
	out[3] = 0xff;
	out[4] = 0xff;
	out[5] = 0xff;

	crc = get_crc16(out, 6);
	
	out[6] = crc & 0xff;
	out[7] = (crc >> 8) & 0xff;

	send_8byte(out);
}

void do_displayFPN(void)
{
	char TestStr[]="4567";
	send_dspl_info(TestStr);
}

U_BOOT_CMD(
	display_fpn,	1,	1,	do_displayFPN,
	"do_displayFPN - display fpn\n",
	NULL
	);

void serial_setbrg_pio5 (void)
{
  serial_init_pio5();
}
#ifdef CONFIG_HWFLOW
extern int hwflow_onoff (int on)
{
	switch (on) {
	case 0:
	default:
		break;		/* return current */
	case 1:
		hwflow = 1;	/* turn on */
		serial_init ();
		break;
	case -1:
		hwflow = 0;	/* turn off */
		serial_init ();
		break;
	}
	return hwflow;
}
#endif	/* CONFIG_HWFLOW */

#endif	/* CONFIG_STM_ASC_SERIAL */
