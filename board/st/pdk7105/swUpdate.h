#ifndef _SW_UPDATE_T_
#define _SW_UPDATE_T_

#define SUCCESS_RET     0
#define FAILURE_RET     -1
#define ABNORMITY_RET   -2

#define RIGHT_EXT2_FS             0
#define NOT_EXT2_FS               1
#define WRONG_EXT2_FS             2
#define UBOOT_UPDATE_TIME_SEC     3 
#define EXTUSB_WARING_TIME_SEC    10
#define INTUSB_WARING_TIME_SEC    60
#define ALL_WARING_TIME_SEC       100

#define INI_VECTORLo					0x00
#define INI_VECTORHi					0x00
#define POLYNOMIALHi					0x80
#define POLYNOMIALLo					0x05

/* led */
#define PIO0_BASSADDRESS  0xFD020000
#define PIO0_CLEAR_C0  *(volatile unsigned char *)(PIO0_BASSADDRESS+0x28)
#define PIO0_CLEAR_C1 *(volatile unsigned char *)(PIO0_BASSADDRESS+0x38)
#define PIO0_CLEAR_C2   *(volatile unsigned char *)(PIO0_BASSADDRESS+0x48)
#define PIO0_SET_C0  *(volatile unsigned char *)(PIO0_BASSADDRESS+0x24)
#define PIO0_SET_C1 *(volatile unsigned char *)(PIO0_BASSADDRESS+0x34)
#define PIO0_SET_C2   *(volatile unsigned char *)(PIO0_BASSADDRESS+0x44)


typedef enum USB_OPER {
	USB_START = 1,
	USB_STOP
} usb_operation_et;

typedef enum USB_PARTITION
{
    EXT_USB_PARTITION=1,
	INT_USB_PARTITION_BACKUP=1,
	INT_USB_PARTITION_RUNNING
}usb_partion_et;

typedef enum UPDATE_KEY 
{    
    KEY_DOWN ,   //升级键按下    
    KEY_UP      //未按升级键
}update_key_et;

typedef enum LED_GB620 
{    
    GB620_RED_LED ,  
    GB620_GREEN_LED      
}led_gb620_et;

typedef enum LED_FLASH_TYPE 
{    
    GB620_LED_FLASH_ALL,  
    GB620_LED_FLASH_SEC       
}led_flash_et;

typedef enum IPTV_LOADER 
{
	NORMAL_START = 1,           //正常启动
	ONEKEY_RECOVER,       		//一键还原模式，包括外部非ext2-U盘和内部升级
	NET_UPGRADE_OSP,      		//网络升级OSP    
	NET_UPGRADE_RUNNING,   		//网络升级RUNNING
	EXT_EXT2_USB=255         	//外部ext2 U盘
}iptv_loader_et;

/* uboot update */
typedef struct _ImageFileHeader
{
	unsigned int  UpdateUbootFlag;/*update uboot flag*/	
	unsigned int  UBOOT_Ver;/*u-boot version: */
	unsigned int  SW_Ver;
	unsigned char HW_Ver[16];
	unsigned int  FactoryID;
	unsigned int  DeviceID;/*620*/
	unsigned int  UbootSize;/*uboot size*/
	unsigned char Date[10];/*eg:20100522*/
	unsigned char SHA1_Uboot[20];/*uboot section sha1*/		
	unsigned short CRC_Header; /*header crc value*/
}ImageFileHeader_T;

typedef struct _SPISoftWareInfo
{
	unsigned int   SW_Ver;
	unsigned int   HW_Ver;
	unsigned int   UBOOT_Ver;/*u-boot version: */
	unsigned int   FactoryID;
	unsigned int   DeviceID;/*620*/
	unsigned short Reserved;
	unsigned char  Date[10];/*20100522*/
}SPISoftWareInfo_T;

typedef struct Loader_Info_s
{
    unsigned char   aLoaderVer[8];
    unsigned char   aAppVer[8];
}Loader_Info_t;

static void SHA1ValueAdd(unsigned char *Sum, unsigned char *AddValue);
static int CompareSHA1Value(unsigned char *buffer1, unsigned char *buffer2);

#endif
