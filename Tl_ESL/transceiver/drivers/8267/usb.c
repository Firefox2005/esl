#include "bsp.h"
#include "usb_def.h"
#include "usb.h"
//#include "../../common/types.h"
typedef unsigned char u8 ;
typedef signed char s8;

typedef unsigned short u16;
typedef signed short s16;

typedef int s32;
typedef unsigned int u32;

typedef long long s64;
typedef unsigned long long u64;

#define		NULL    0
////////////////////////////////////////////////////////////////////////
#define	  	USB_PRINTER
#define		USB_KEYBOARD
#define		USB_MOUSE
#define		USB_SPEAKER
#define		USB_MICROPHONE



////////////////////////////////////////////////////////////////////////

unsigned char irq_edp0_setup = 0, irq_edp0_data = 0, irq_edp0_status = 0,
              irq_edps = 0;
unsigned char bm_request = 0, usb_request = 0, usb_valuel = 0, usb_valueh = 0,
              usb_indexl = 0, usb_indexh = 0;

unsigned short   usb_length = 0;
unsigned short   g_length = 0;
const unsigned char * g_table = 0;
unsigned char   g_stall = 0;
unsigned char   g_protocol = 0;		//default 1 for report protocol
unsigned char   g_rate = 0;		//default 0 for all report
unsigned char   aodat = 0;

#define IF_ID_PRINTER           0x00
#define IF_ID_AUDIO_CONTROL     0x01
#define IF_ID_MIC               0x02
#define IF_ID_SPEAKER           0x03
#define IF_ID_MOUSE             0x04
#define IF_ID_KEYBOARD          0x05

#define EDP_ID_PRINTER_IN       0x08
#define EDP_ID_PRINTER_OUT      0x07
#define EDP_ID_MIC              0x07
#define EDP_ID_SPEAKER          0x06
#define EDP_ID_MOUSE            0x01
#define EDP_ID_KEYBOARD_IN      0x02
#define EDP_ID_KEYBOARD_OUT     0x03


//------------------------------------------------------------------------------------
const unsigned char DeviceDescriptor[] = 
{
  //Device:USB1.10,Vid=0x0123,Pid=0x4567,bNumConfigurations = 0x01,
  0x12,      //Length
  0x01,      //DescriptorType
  0x10,0x01, //bcdUSB
  0x00,      //DeviceClass
  0x00,      //DeviceSubClass
  0x00,      //DeviceProtocol
  0x08,      //bMaxPacketSize 8
  0x8a,0x24, //idVendor........
  0x67,0x45, //idProduct........
  0x01,0x00, //bcdDevice
  1,         //iManufacturer
  2,         //iProduct
  3,         //iSerialNumber
  0x01       //bNumConfigurations
};

#define CONFIG_DESCRIPTOR  0x09, /* Length */ \
0x02,      /* DescriptorType : ConfigDescriptor */    \
0xfa,0x00, /* TotalLength: variable */  \
0x06,      /* NumInterfaces: variable */  \
0x01,      /* ConfigurationValue */  \
0x00,      /* Configuration String */  \
0x80,      /* Attributes:Bus Power */  \
0xfa       /* MaxPower = 0xfa*2ma */

#define PRINTER_STD_IF   0x09, /* Length */ \
0x04,  /* bDescriptorType */ \
0x00, /*IF_ID_PRINTER,*/  /* bInterfaceNumber */ \
0x00,  /* bAlternateSetting */ \
0x02,  /* bNumEndpoints */ \
0x07,  /* bInterfaceclass ->Printer */ \
0x01,  /* bInterfaceSubClass -> Control */ \
0x02,  /* bInterfaceProtocol maybe 0x03 */ \
0x00   /* iInterface */

#define PRINTER_IN_ENDPOINT 0x07, /* length */ \
0x05, /* bDescriptorType */ \
0x88, /* 0x80 | EDP_ID_PRINTER_IN,*/ /* bEndpointAddress -> Direction: IN - EndpointID: 8 */ \
0x02, /* bmAttributes  -> Transfer Type: Bulk */ \
0x40, 0x00, /* wMaxPacketSize */ \
0x00  /* bInterval */

#define PRINTER_OUT_ENDPOINT 0x07, /* length */ \
0x05, /* bDescriptorType */ \
0x07, /* EDP_ID_PRINTER_OUT,*/ /* bEndpointAddress -> Direction: OUT - EndpointID: 7 */ \
0x02, /* bmAttributes  -> Bulk  Transfer Typ */ \
0x40, 0x00, /* wMaxPacketSize */ \
0x00 /* bInterval */

/*Audio Control Interface Descriptors*/
/*Standard Audio Control Interface Descriptors*/
#define AUDIO_CONTROL_STD_IF 0x09, /* Length */ \
0x04, /* DescriptorType:Interface */ \
IF_ID_AUDIO_CONTROL, /* InterfaceNum:1 */ \
0x00, /* AlternateSetting:0 */ \
0x00, /* NumEndpoint:0 */ \
0x01, /* InterfaceClass:audio */ \
0x01, /* InterfaceSubClass:audio control */ \
0x00, /* InterfaceProtocol */ \
0x00  /* Interface String */

/*Class-specific Audio Control Interface Descriptors*/
/*1. Header*/
#if defined (USB_MICROPHONE) && defined (USB_SPEAKER)
#define  AUDIO_CONTROL_HEADER  0x0a, /* Length */ \
0x24, /* DescriptorType:audio interface descriptor */ \
0x01, /* DescriptorSubType:audio control header */ \
0x00,0x01, /* bcdADC:audio Device Class v1.00 */ \
0x34,0x00, /* TotalLength: variable */ \
0x02, /* InCollection:2 AudioStreaming interface */ \
0x02, /* Microphone AS id: 2 */ \
0x03  /* Speaker AS id: 3 */
#elif defined (USB_MICROPHONE)
#define  AUDIO_CONTROL_HEADER  0x09, /* Length */ \
0x24, /* DescriptorType:audio interface descriptor */ \
0x01, /* DescriptorSubType:audio control header */ \
0x00,0x01, /* bcdADC:audio Device Class v1.00 */ \
0x1e,0x00, /* TotalLength: variable */ \
0x01, /* InCollection:1 AudioStreaming interface */ \
0x02  /* Microphone AS id: 2 */ 
#elif defined (USB_SPEAKER)
#define  AUDIO_CONTROL_HEADER  0x09, /* Length */ \
0x24, /* DescriptorType:audio interface descriptor */ \
0x01, /* DescriptorSubType:audio control header */ \
0x00,0x01, /* bcdADC:audio Device Class v1.00 */ \
0x1e,0x00, /* TotalLength: variable */ \
0x01, /* InCollection:1 AudioStreaming interface */ \
0x03  /* Speaker AS id: 3 */
#endif

/*2. Terminal or Unit*/
#define MIC_INPUT_TERMINAL 0x0c, /* Length */ \
0x24,  /* DescriptorType:audio interface descriptor */ \
0x02,  /* DescriptorSubType:Input Terminal */ \
0x01,  /* TerminalID:0x01 */ \
0x01,0x02, /* TerminalType:USB Microphone */ \
0x00,  /* AssocTerminal */ \
0x01,  /* NrChannels:mono 1 channel */ \
0x00,0x00, /* ChannelConfig:Left Front,Right Front */ \
0x00,  /* ChannelName String */ \
0x00   /* Terminal String */

//USB Streaming OT:audio interface descriptor,audio control output terminal(0x03),terminal id 0x03,
//USB Streaming(0x0101),Output Terminal(0x03),SourceId 0x02,
#define MIC_OUTPUT_TERMINAL 0x09, /* Length */ \
0x24,  /* DescriptorType:audio interface descriptor */ \
0x03,  /* DescriptorSubTYpe:Output Terminal */ \
0x03,  /* TerminalID:0x03 */ \
0x01,0x01, /* TerminalType:USB Streaming */ \
0x00,  /* AssocTerminal:ID 0 */ \
0x01,  /* SourceID:1 from input terminal */ \
0x00   /* Terminal String */

#define SPEAKER_INPUT_TERMINAL 0x0c, /* Length */ \
0x24,  /* DescriptorType:audio interface descriptor */ \
0x02,  /* DescriptorSubType:Input Terminal */ \
0x04,  /* TerminalID:0x04 */ \
0x01,0x01, /* TerminalType:USB Streaming */ \
0x00,  /* AssocTerminal */ \
0x01,  /* NrChannels:2 channel */ \
0x00,0x00, /* ChannelConfig:Left Front,Right Front */ \
0x00,  /* ChannelName String */ \
0x00   /* Terminal String */

#define SPEAKER_OUTPUT_TERMINAL 0x09, /* Length */ \
0x24,  /* DescriptorType:audio interface descriptor */ \
0x03,  /* DescriptorSubTYpe:Output Terminal */ \
0x06,  /* TerminalID:0x06 */ \
0x01,0x03, /* TerminalType:Speaker */ \
0x00,  /* AssocTerminal: 0 */ \
0x04,  /* SourceID:4  SPEAKER_INPUT_TERMINAL */ \
0x00   /* Terminal String */

/*Audio Control Endpoint Descriptors*/
/*Standard Audio Control Endpoint Descriptors*/
/*Endpoint 0 is taken as Standard Audio Control Endpoint*/

/*Class-specific Audio Control Endpoint Descriptors*/
/*None*/

/*Microphone AudioStreaming Interface Descriptors*/
/*Standard AS Interface Descriptors*/
/*1. Zero-bandwidth Alternate Setting 0*/
#define MIC_STD_IF_SET0 0x09, /* Length */ \
0x04,  /* DescriptorType:Interface */ \
IF_ID_MIC,  /* InterfaceNum(id): 2 */ \
0x00,  /* AlternateSetting:0 */ \
0x00,  /* NumEndpoint:0 */ \
0x01,  /* InterfaceClass:audio */ \
0x02,  /* InterfaceSubClass:audio streaming */ \
0x00,  /* InterfaceProtocol */ \
0x00   /* Interface String */

/*2. Operational Alternate Setting 1*/
#define MIC_STD_IF_SET1 0x09, /* Length */ \
0x04, /* DescriptorType:Interface */ \
IF_ID_MIC, /* InterfaceNum(id): 2 */ \
0x01, /* AlternateSetting:1 */ \
0x01, /* NumEndpoint:1 */ \
0x01, /* InterfaceClass:audio */ \
0x02, /* InterfaceSubClass:audio streaming */ \
0x00, /* InterfaceProtocol */ \
0x00  /* Interface String */

/*Class-specific AS Interface Descriptors*/
/*1. AS_General Interface Descriptors*/
#define MIC_AS_GENERAL 0x07,  /* Length */ \
0x24,   /* DescriptorType:audio interface descriptor */ \
0x01,   /* DescriptorSubType:AS_GENERAL */ \
0x03,   /* TerminalLink:#3USB USB Streaming OT */ \
0x01,   /* Delay:1 */ \
0x01,0x00   /* FormatTag:PCM */

/*2. Format_Type Interface Descriptors*/
#define MIC_AS_FORMAT_TYPE 0x0b,  /* Length */ \
0x24,  /* DescriptorType:audio interface descriptor */ \
0x02,  /* DescriptorSubType:Format_type */ \
0x01,  /* FormatType:Format type 1 */ \
0x01,  /* NumberOfChanne:1 */ \
0x02,  /* SubframeSize:2byte */ \
0x10,  /* BitsResolution:16bit */ \
0x01,  /* SampleFreqType:One sampling frequency. */ \
/* 0x40,0x1f,0x00  //8k */ \
/* 0x80,0x3e,0x00  //16k */ \
/* 0x22,0x56,0x00  //22.05k */ \
/* 0x00,0x7d,0x00  // 32k */ \
/* 0x44,0xac,0x00  //44k */ \
0x80,0x3e,0x00//16K// 0x80,0xbb,0x00   // 48k 

/*Standard AS Isochronous Audio Data Endpoint Descriptors*/
#define MIC_ENDPOINT  0x07, /* Length */ \
0x05,  /* DescriptorType:endpoint descriptor */ \
0x80 | EDP_ID_MIC,  /* EndpointAddress:Input endpoint 7 */ \
0x05,  /* Attributes:0x05,Isochronous,Synchronization Type(Asynchronous).. */ \
0x00,0x01, /* MaxPacketSize: 256 */ \
0x01   /* Interval */

/*Class-specific AS Isochronous Audio Data Endpoint Descriptors*/
#define MIC_AUDIO_ENDPOINT  0x07,  /* Length */ \
0x25,  /* DescriptorType:audio endpoint descriptor */ \
0x01,  /* DescriptorSubType:audio endpiont general */ \
0x00,  /* Attributes:0x00........ */ \
0x00,  /* LockDelayUnits */ \
0x00,0x00  /* LockDelay */

/*Speaker AudioStreaming Interface Descriptors*/
/*Standard AS Interface Descriptors*/
/*1. Zero-bandwidth Alternate Setting 0*/
#define SPEAKRE_STD_IF_SET0  0x09, /* Length */ \
0x04,  /* DescriptorType:Interface */ \
IF_ID_SPEAKER,  /* InterfaceNum(id): 3 */ \
0x00,  /* AlternateSetting:0 */ \
0x00,  /* NumEndpoint:0 */ \
0x01,  /* InterfaceClass:audio */ \
0x02,  /* InterfaceSubClass:audio streaming */ \
0x00,  /* InterfaceProtocol */ \
0x00   /* Interface String */

/*2. Operational Alternate Setting 1*/
#define SPEAKER_STD_IF_SET1  0x09, /* Length */ \
0x04,  /* DescriptorType:Interface */ \
IF_ID_SPEAKER,  /* InterfaceNum(id): 3 */ \
0x01,  /* AlternateSetting:1 */ \
0x01,  /* NumEndpoint:1 */ \
0x01,  /* InterfaceClass:audio */ \
0x02,  /* InterfaceSubClass:audio streaming */ \
0x00,  /* InterfaceProtocol */ \
0x00   /* Interface String */

/*Class-specific AS Interface Descriptors*/
/*1. AS_General Interface Descriptors*/
#define SPEAKER_AS_GENERAL  0x07,  /* Length */ \
0x24,  /* DescriptorType:audio interface descriptor */ \
0x01,  /* DescriptorSubType:AS_GENERAL */ \
0x04,  /* TerminalLink:#4 USB Streaming IT */ \
0x01,  /* Delay:1 */ \
0x01,0x00  /* FormatTag:PCM */

/*2. Format_Type Interface Descriptors*/
#define SPEAKER_AS_FORMAT_TYPE    0x0b,  /* Length */ \
0x24,  /* DescriptorType:audio interface descriptor */ \
0x02,  /* DescriptorSubType:Format_type */ \
0x01,  /* FormatType:Format type 1 */ \
0x01,  /* NumberOfChanne:1 */ \
0x02,  /* SubframeSize:2byte */ \
0x10,  /* BitsResolution:16bit */ \
0x01,  /* SampleFreqType:One sampling frequency. */ \
/* 0x40,0x1f,0x00  //8k */ \
/* 0x80,0x3e,0x00  //16k */ \
/* 0x22,0x56,0x00, //22.05k */ \
/* 0x00,0x7d,0x00  // 32k */ \
/* 0x44,0xac,0x00, //44.1k */ \
0x80,0x3e,0x00//16K //0x80,0xbb,0x00   //48k 

/*Standard AS Isochronous Audio Data Endpoint Descriptors*/
#define SPEAKER_ENDPOINT    0x07,   /* Length */ \
0x05,  /* DescriptorType:endpoint descriptor */ \
EDP_ID_SPEAKER,  /* EndpointAddress */ \
0x09,  /* Attributes:0x05,Isochronous,Synchronization Type(Asynchronous).... */ \
0x00,0x01,  /* MaxPacketSize: 256 */ \
0x01   /* Interval */

/*Class-specific AS Isochronous Audio Data Endpoint Descriptors*/
#define SPEAKER_AUDIO_ENDPOINT    0x07,   /* Length */ \
0x25,  /* DescriptorType:audio endpoint descriptor */ \
0x01,  /* DescriptorSubType:audio endpiont general */ \
0x00,  /* Attributes:0x00............. */ \
0x00,  /* LockDelayUnits */ \
0x00,0x00  /* LockDelay */

#define MOUSE_STD_IF 0x09, /* length */ \
0x04, /* bDescriptorType�ֶΡ��ӿ��������ı��Ϊ0x04 */ \
IF_ID_MOUSE, /* bInterfaceNumber(id): 4 */ \
0x00, /* bAlternateSetting�ֶΡ��ýӿڵı��ñ�ţ�Ϊ0 */ \
0x01, /* bNumEndpoints�ֶΡ���0�˵����Ŀ������USB���ֻ��Ҫһ���ж�����˵㣬��˸�ֵΪ1 */ \
0x03, /* bInterfaceClass�ֶΡ��ýӿ���ʹ�õ��ࡣUSB�����HID�࣬HID��ı���Ϊ0x03 */ \
0x01, /* bInterfaceSubClass�ֶΡ��ýӿ���ʹ�õ����ࡣUSB���̡�������ڸ����࣬�������Ϊ0x01��*/ \
0x02, /* bInterfaceProtocol�ֶΡ����̴���Ϊ0x01��������Ϊ0x02��*/ \
0x00  /* iConfiguration�ֶΡ��ýӿڵ��ַ�������ֵ������û�У�Ϊ0 */

#define MOUSE_HID  0x09, /* length */ \
0x21, /* bDescriptorType�ֶΡ�HID�������ı��Ϊ0x21��*/ \
0x10, 0x01, /* bcdHID�ֶΡ���Э��ʹ�õ�HID1.1Э�顣ע����ֽ����� */ \
0x21, /* bCountyCode�ֶΡ��豸���õĹ��Ҵ��룬����ѡ��Ϊ����������0x21 */ \
0x01, /* bNumDescriptors�ֶΡ��¼�����������Ŀ������ֻ��һ������������ */ \
0x22, /* bDescriptorType�ֶΡ��¼������������ͣ�Ϊ���������������Ϊ0x22 */ \
0x34, 0x00 /* bDescriptorLength�ֶΡ��¼��������ĳ��ȡ��¼�������Ϊ���������� */

#define MOUSE_ENDPOINT  0x07, /* length */ \
0x05, /* bDescriptorType�ֶΡ��˵����������Ϊ0x05 */ \
0x80 | EDP_ID_MOUSE, /* bEndpointAddress�ֶΡ��˵�ĵ�ַ��D7λ��ʾ���ݷ���D3-D0��ʾ�˵�� */ \
0x03, /* bmAttributes�ֶΡ�D1~D0Ϊ�˵㴫������ѡ�񡣸ö˵�Ϊ�ж϶˵㡣�ж϶˵�ı��Ϊ3������λ����Ϊ0��*/ \
0x10, 0x00, /* wMaxPacketSize�ֶΡ��ö˵�����������˵�1��������Ϊ16�ֽڡ�ע����ֽ����ȡ�*/ \
0x0A  /* bInterval�ֶΡ��˵��ѯ��ʱ�䣬��������Ϊ10��֡ʱ�䣬��10ms */

#define KEYBOARD_STD_IF  0x09, /* length */ \
0x04,  /* bDescriptorType�ֶΡ��ӿ��������ı��Ϊ0x04 */ \
IF_ID_KEYBOARD,  /* bInterfaceNumber(id): 5 */ \
0x00,  /* bAlternateSetting�ֶΡ��ýӿڵı��ñ�ţ�Ϊ0��*/ \
0x02,  /* bNumEndpoints�ֶΡ���0�˵����Ŀ������USB������Ҫ����, �ж϶˵㣨һ������һ�����������˸�ֵΪ2 */ \
0x03,  /* bInterfaceClass�ֶΡ��ýӿ���ʹ�õ��ࡣUSB������HID�࣬HID��ı���Ϊ0x03��*/ \
0x01,  /* bInterfaceSubClass�ֶΡ�ֻ�涨��һ�����ࣺ֧��BIOS�������������ࡣUSB���̡�������ڸ����࣬�������Ϊ0x01��*/ \
0x01,  /* bInterfaceProtocol�ֶΡ����̴���Ϊ0x01��������Ϊ0x02��*/ \
0x00   /* iConfiguration�ֶΡ��ýӿڵ��ַ�������ֵ������û�У�Ϊ0��*/

#define KEYBOARD_HID  0x09,  /* length */ \
0x21,  /* bDescriptorType�ֶΡ�HID�������ı��Ϊ0x21 */ \
0x10, 0x01,  /* bcdHID�ֶΡ���Э��ʹ�õ�HID1.1Э�顣ע����ֽ����ȡ�*/ \
0x21,  /* bCountyCode�ֶΡ��豸���õĹ��Ҵ��룬����ѡ��Ϊ����������0x21��*/ \
0x01,  /* bNumDescriptors�ֶΡ��¼�����������Ŀ������ֻ��һ��������������*/ \
0x22,  /* bDescriptorType�ֶΡ��¼������������ͣ�Ϊ���������������Ϊ0x22��*/ \
0x41, 0x00 /* bDescriptorLength�ֶΡ��¼��������ĳ��ȡ��¼�������Ϊ���̱�����������*/

#define KEYBOARD_ENDPOINT_IN  0x07, /* length */ \
0x05,  /* bDescriptorType�ֶΡ��˵����������Ϊ0x05��*/ \
0x80 | EDP_ID_KEYBOARD_IN,  /* bEndpointAddress�ֶΡ��˵�ĵ�ַ��D7λ��ʾ���ݷ���D3-D0��ʾ�˵��*/ \
0x03,  /* bmAttributes�ֶΡ�D1~D0Ϊ�˵㴫������ѡ�񡣸ö˵�Ϊ�ж϶˵㡣�ж϶˵�ı��Ϊ3������λ����Ϊ0��*/ \
0x10, 0x00,  /* wMaxPacketSize�ֶΡ��ö˵�����������˵�2��������Ϊ16�ֽڡ�ע����ֽ����ȡ�*/ \
0x0A   /* bInterval�ֶΡ��˵��ѯ��ʱ�䣬��������Ϊ10��֡ʱ�䣬��10ms��*/

#define KEYBOARD_ENDPOINT_OUT  0x07,  /* length */ \
0x05,  /* bDescriptorType�ֶΡ��˵����������Ϊ0x05��*/ \
EDP_ID_KEYBOARD_OUT,  /* bEndpointAddress�ֶΡ��˵�ĵ�ַ��D7λ��ʾ���ݷ���D3-D0��ʾ�˵��*/ \
0x03,  /* bmAttributes�ֶΡ�D1~D0Ϊ�˵㴫������ѡ�񡣸ö˵�Ϊ�ж϶˵㡣�ж϶˵�ı��Ϊ3������λ����Ϊ0��*/ \
0x10, 0x00,  /* wMaxPacketSize�ֶΡ��ö˵�����������˵�1��������Ϊ16�ֽڡ�ע����ֽ����ȡ�*/ \
0x0A   /* bInterval�ֶΡ��˵��ѯ��ʱ�䣬��������Ϊ10��֡ʱ�䣬��10ms��*/


/* USB Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
unsigned char ConfigDescriptor[] = 
{
  CONFIG_DESCRIPTOR,
#ifdef USB_PRINTER
  PRINTER_STD_IF,
  PRINTER_IN_ENDPOINT,
  PRINTER_OUT_ENDPOINT,
#endif
#if defined (USB_MICROPHONE) || defined (USB_SPEAKER)
  AUDIO_CONTROL_STD_IF,
  AUDIO_CONTROL_HEADER,
#endif
#ifdef USB_MICROPHONE 
  MIC_INPUT_TERMINAL,
  MIC_OUTPUT_TERMINAL,
#endif
#ifdef USB_SPEAKER
  SPEAKER_INPUT_TERMINAL,
  SPEAKER_OUTPUT_TERMINAL,
#endif
#ifdef USB_MICROPHONE
  MIC_STD_IF_SET0,
  MIC_STD_IF_SET1,
  MIC_AS_GENERAL,
  MIC_AS_FORMAT_TYPE,
  MIC_ENDPOINT,
  MIC_AUDIO_ENDPOINT,
#endif
#ifdef USB_SPEAKER
  SPEAKRE_STD_IF_SET0,
  SPEAKER_STD_IF_SET1,
  SPEAKER_AS_GENERAL,
  SPEAKER_AS_FORMAT_TYPE,
  SPEAKER_ENDPOINT,
  SPEAKER_AUDIO_ENDPOINT,
#endif
#ifdef USB_MOUSE
  MOUSE_STD_IF,
  MOUSE_HID,
  MOUSE_ENDPOINT,
#endif
#ifdef USB_KEYBOARD
  KEYBOARD_STD_IF,
  KEYBOARD_HID,
  KEYBOARD_ENDPOINT_IN,
  KEYBOARD_ENDPOINT_OUT,
#endif
};

/* USB String Descriptor (optional) */
const unsigned char StringLangID[] = 
{
  0x04,
  0x03,
  0x09,
  0x04
};

const unsigned char StringVendor[] = 
{
  0x26, //Length
  0x03, //DescriptorType
  'D', 0, 'e', 0, 'm', 0, 'o', 0, '-', 0, 's', 0, 'p', 0, 'e', 0,
  'r', 0, 'k', 0, 'e', 0, 'r', 0, 'p', 0, 'h', 0, 'o', 0, 'n', 0,
  'e', 0, '1', 0
};

const unsigned char StringProduct[] = 
{
  0x1c,  //Length
  0x03,  //DescriptorType
  'T', 0, 'e', 0, 'l', 0, 'i', 0, 'n', 0, 'k', 0,
  'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0, '1', 0, '6', 0
};


const unsigned char StringSerial[] = 
{
  0x1c,  //Length
  0x03,  //DescriptorType
  'T', 0, 'e', 0, 'l', 0, 'i', 0, 'n', 0, 'k', 0,
  'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0, '1', 0, '6', 0
};

#ifdef USB_MOUSE
//����HID����������MOUSE_HID������ݱ���һ��
const unsigned char MouseHidDesc[] = 
{
	0x09, 0x10, 0x01, 0x21, 0x01, 0x22, 0x34, 0x00
};

//USB�����������Ķ���
const unsigned char MouseReportDesc[] = 
{
 //ÿ�п�ʼ�ĵ�һ�ֽ�Ϊ����Ŀ��ǰ׺��ǰ׺�ĸ�ʽΪ��
 //D7~D4��bTag��D3~D2��bType��D1~D0��bSize�����·ֱ��ÿ����Ŀע�͡�
 
 //����һ��ȫ�֣�bTypeΪ1����Ŀ��ѡ����;ҳΪ��ͨ����Generic Desktop Page(0x01)
 //�����һ�ֽ����ݣ�bSizeΪ1����������ֽ����Ͳ�ע���ˣ�
 //�Լ�����bSize���жϡ�
 0x05, 0x01, // USAGE_PAGE (Generic Desktop)
 
 //����һ���ֲ���bTypeΪ2����Ŀ��˵����������Ӧ�ü�����;�������
 0x09, 0x02, // USAGE (Mouse)
 
 //����һ������Ŀ��bTypeΪ0����Ŀ�������ϣ������������0x01��ʾ
 //�ü�����һ��Ӧ�ü��ϡ�����������ǰ������;ҳ����;����Ϊ
 //��ͨ�����õ���ꡣ
 0xa1, 0x01, // COLLECTION (Application)
 
 //����һ���ֲ���Ŀ��˵����;Ϊָ�뼯��
 0x09, 0x01, //   USAGE (Pointer)
 
 //����һ������Ŀ�������ϣ������������0x00��ʾ�ü�����һ��
 //�����ϣ���;��ǰ��ľֲ���Ŀ����Ϊָ�뼯�ϡ�
 0xa1, 0x00, //   COLLECTION (Physical)
 
 //����һ��ȫ����Ŀ��ѡ����;ҳΪ������Button Page(0x09)��
 0x05, 0x09, //     USAGE_PAGE (Button)
 
 //����һ���ֲ���Ŀ��˵����;����СֵΪ1��ʵ��������������
 0x19, 0x01, //     USAGE_MINIMUM (Button 1)
 
 //����һ���ֲ���Ŀ��˵����;�����ֵΪ3��ʵ����������м���
 0x29, 0x03, //     USAGE_MAXIMUM (Button 3)
 
 //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼�ֵ���������Ƿ��ص��������ֵ����
 //��СΪ0����Ϊ����������Bit����ʾһ�������������СΪ0�����Ϊ1��
 0x15, 0x00, //     LOGICAL_MINIMUM (0)
 
 //����һ��ȫ����Ŀ��˵���߼�ֵ���Ϊ1��
 0x25, 0x01, //     LOGICAL_MAXIMUM (1)
 
 //����һ��ȫ����Ŀ��˵�������������Ϊ������
 0x95, 0x03, //     REPORT_COUNT (3)
 
 //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ1��bit��
 0x75, 0x01, //     REPORT_SIZE (1)
 
 //����һ������Ŀ��˵����3������Ϊ1bit�������������ͳ���
 //��ǰ�������ȫ����Ŀ�����壩������Ϊ���룬
 //����Ϊ��Data,Var,Abs��Data��ʾ��Щ���ݿ��Ա䶯��Var��ʾ
 //��Щ�������Ƕ����ģ�ÿ�����ʾһ����˼��Abs��ʾ����ֵ��
 //��������Ľ�����ǣ���һ��������bit0��ʾ����1��������Ƿ��£�
 //�ڶ���������bit1��ʾ����2���Ҽ����Ƿ��£�������������bit2��ʾ
 //����3���м����Ƿ��¡�
 0x81, 0x02, //     INPUT (Data,Var,Abs)
 
 //����һ��ȫ����Ŀ��˵������������Ϊ1��
 0x95, 0x01, //     REPORT_COUNT (1)
 
 //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ5bit��
 0x75, 0x05, //     REPORT_SIZE (5)
 
 //����һ������Ŀ�������ã���ǰ������ȫ����Ŀ��֪������Ϊ5bit��
 //����Ϊ1������������Ϊ�����������ص�����һֱ��0����
 //���ֻ��Ϊ�˴���һ���ֽڣ�ǰ������3��bit��������һЩ����
 //���ѣ���������û��ʵ����;�ġ�
 0x81, 0x03, //     INPUT (Cnst,Var,Abs)
 
 //����һ��ȫ����Ŀ��ѡ����;ҳΪ��ͨ����Generic Desktop Page(0x01)
 0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
 
 //����һ���ֲ���Ŀ��˵����;ΪX��
 0x09, 0x30, //     USAGE (X)
 
 //����һ���ֲ���Ŀ��˵����;ΪY��
 0x09, 0x31, //     USAGE (Y)
 
 //����һ���ֲ���Ŀ��˵����;Ϊ����
 0x09, 0x38, //     USAGE (Wheel)
 
 //��������Ϊȫ����Ŀ��˵�����ص��߼���С�����ֵ��
 //��Ϊ���ָ���ƶ�ʱ��ͨ���������ֵ����ʾ�ģ�
 //���ֵ����˼���ǣ���ָ���ƶ�ʱ��ֻ�����ƶ�����
 //�����ƶ�ʱ��XֵΪ���������ƶ�ʱ��YֵΪ����
 //���ڹ��֣����������Ϲ�ʱ��ֵΪ����
 0x15, 0x81, //     LOGICAL_MINIMUM (-127)
 0x25, 0x7f, //     LOGICAL_MAXIMUM (127)
 
 //����һ��ȫ����Ŀ��˵��������ĳ���Ϊ8bit��
 0x75, 0x08, //     REPORT_SIZE (8)
 
 //����һ��ȫ����Ŀ��˵��������ĸ���Ϊ3����
 0x95, 0x03, //     REPORT_COUNT (3)
 
 //����һ������Ŀ����˵��������8bit���������������õģ�
 //����Ϊ��Data,Var,Rel��Data˵�������ǿ��Ա�ģ�Var˵��
 //��Щ�������Ƕ����ģ�����һ��8bit��ʾX�ᣬ�ڶ���8bit��ʾ
 //Y�ᣬ������8bit��ʾ���֡�Rel��ʾ��Щֵ�����ֵ��
 0x81, 0x06, //     INPUT (Data,Var,Rel)
 
 //��������������Ŀ�����ر�ǰ��ļ����á�
 //���ǿ����������ϣ�����Ҫ�����Ρ�bSizeΪ0�����Ժ���û���ݡ�
 0xc0,       //   END_COLLECTION
 0xc0        // END_COLLECTION
};
#endif 

#ifdef USB_KEYBOARD
//���̵�HID����������KEYBOARD_HID������ݱ���һ��
const unsigned char KeyboardHidDesc[] = 
{
	0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, 0x41, 0x00 
};


//USB���̱����������Ķ���
const unsigned char KeyboardReportDesc[] = 
{
 //ÿ�п�ʼ�ĵ�һ�ֽ�Ϊ����Ŀ��ǰ׺��ǰ׺�ĸ�ʽΪ��
 //D7~D4��bTag��D3~D2��bType��D1~D0��bSize�����·ֱ��ÿ����Ŀע�͡�
 
 //����һ��ȫ�֣�bTypeΪ1����Ŀ������;ҳѡ��Ϊ��ͨ����Generic Desktop Page(0x01)
 //�����һ�ֽ����ݣ�bSizeΪ1����������ֽ����Ͳ�ע���ˣ�
 //�Լ�����bSize���жϡ�
 0x05, 0x01, // USAGE_PAGE (Generic Desktop)
 
 //����һ���ֲ���bTypeΪ2����Ŀ��˵���������ļ�����;���ڼ���
 0x09, 0x06, // USAGE (Keyboard)
 
 //����һ������Ŀ��bTypeΪ0����Ŀ�������ϣ������������0x01��ʾ
 //�ü�����һ��Ӧ�ü��ϡ�����������ǰ������;ҳ����;����Ϊ
 //��ͨ�����õļ��̡�
 0xa1, 0x01, // COLLECTION (Application)
 
 //����ID�����ﶨ����̱����IDΪ1������ID 0�Ǳ����ģ�
 //Ϊ�˼��ٳ�����޸�����������Ȼ����һ������ID��
 //0x85, 0x01, //Report ID (1)
 
 //����һ��ȫ����Ŀ��ѡ����;ҳΪ���̣�Keyboard/Keypad(0x07)��
 0x05, 0x07, //     USAGE_PAGE (Keyboard/Keypad)

 //����һ���ֲ���Ŀ��˵����;����СֵΪ0xe0��ʵ�����Ǽ�����Ctrl����
 //�������;ֵ����HID��;���в鿴��
 0x19, 0xe0, //     USAGE_MINIMUM (Keyboard LeftControl)
 
 //����һ���ֲ���Ŀ��˵����;�����ֵΪ0xe7��ʵ�����Ǽ�����GUI����
 0x29, 0xe7, //     USAGE_MAXIMUM (Keyboard Right GUI)
 
 //����һ��ȫ����Ŀ��˵�����ص����ݵ��߼�ֵ���������Ƿ��ص��������ֵ��
 //��СΪ0����Ϊ����������Bit����ʾһ�������������СΪ0�����Ϊ1��
 0x15, 0x00, //     LOGICAL_MINIMUM (0)
 
 //����һ��ȫ����Ŀ��˵���߼�ֵ���Ϊ1��
 0x25, 0x01, //     LOGICAL_MAXIMUM (1)
 
 //����һ��ȫ����Ŀ��˵�������������Ϊ�˸���
 0x95, 0x08, //     REPORT_COUNT (8)
 
 //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ1��bit��
 0x75, 0x01, //     REPORT_SIZE (1)
 
 //����һ������Ŀ��˵����8������Ϊ1bit�������������ͳ���
 //��ǰ�������ȫ����Ŀ�����壩������Ϊ���룬
 //����Ϊ��Data,Var,Abs��Data��ʾ��Щ���ݿ��Ա䶯��Var��ʾ
 //��Щ�������Ƕ����ģ�ÿ�����ʾһ����˼��Abs��ʾ����ֵ��
 //��������Ľ�����ǣ���ĳ�����ֵΪ1ʱ���ͱ�ʾ��Ӧ�ļ����¡�
 //bit0�Ͷ�Ӧ����;��Сֵ0xe0��bit7��Ӧ����;���ֵ0xe7��
 0x81, 0x02, //     INPUT (Data,Var,Abs)
 
 //����һ��ȫ����Ŀ��˵������������Ϊ1��
 0x95, 0x01, //     REPORT_COUNT (1)
 
 //����һ��ȫ����Ŀ��˵��ÿ��������ĳ���Ϊ8bit��
 0x75, 0x08, //     REPORT_SIZE (8)
 
 //����һ������Ŀ�������ã���ǰ������ȫ����Ŀ��֪������Ϊ8bit��
 //����Ϊ1������������Ϊ�����������ص�����һֱ��0����
 //���ֽ��Ǳ����ֽڣ�������OEMʹ�ã���
 0x81, 0x03, //     INPUT (Cnst,Var,Abs)
 
 //����һ��ȫ����Ŀ������λ������Ϊ6����
 0x95, 0x06, //   REPORT_COUNT (6)
 
 //����һ��ȫ����Ŀ������ÿ��λ�򳤶�Ϊ8bit��
 //��ʵ���������Ŀ��ҪҲ�ǿ��Եģ���Ϊ��ǰ���Ѿ���һ������
 //����Ϊ8bit��ȫ����Ŀ�ˡ�
 0x75, 0x08, //   REPORT_SIZE (8)
 
 //����һ��ȫ����Ŀ�������߼���СֵΪ0��
 //ͬ�ϣ��������ȫ����ĿҲ�ǿ��Բ�Ҫ�ģ���Ϊǰ���Ѿ���һ��
 //�����߼���СֵΪ0��ȫ����Ŀ�ˡ�
 0x15, 0x00, //   LOGICAL_MINIMUM (0)
 
 //����һ��ȫ����Ŀ�������߼����ֵΪ255��
 0x25, 0xFF, //   LOGICAL_MAXIMUM (255)
 
 //����һ��ȫ����Ŀ��ѡ����;ҳΪ���̡�
 //ǰ���Ѿ�ѡ�����;ҳΪ�����ˣ����Ը���Ŀ��ҪҲ���ԡ�
 0x05, 0x07, //   USAGE_PAGE (Keyboard/Keypad)
 
 //����һ���ֲ���Ŀ��������;��СֵΪ0��0��ʾû�м����£�
 0x19, 0x00, //   USAGE_MINIMUM (Reserved (no event indicated))
 
 //����һ���ֲ���Ŀ��������;���ֵΪ0x65
 0x29, 0x65, //   USAGE_MAXIMUM (Keyboard Application)
 
 //����һ������Ŀ����˵��������8bit���������������õģ�
 //����Ϊ��Data,Ary,Abs��Data˵�������ǿ��Ա�ģ�Ary˵��
 //��Щ��������һ�����飬��ÿ��8bit�����Ա�ʾĳ����ֵ��
 //������µļ�̫�ࣨ���糬�����ﶨ��ĳ��Ȼ��߼��̱����޷�
 //ɨ����������ʱ��������Щ���ݷ���ȫ1�������ƣ�����ʾ������Ч��
 //Abs��ʾ��Щֵ�Ǿ���ֵ��
 0x81, 0x00, //     INPUT (Data,Ary,Abs)

 //����Ϊ������������
 //�߼���Сֵǰ���Ѿ��ж���Ϊ0�ˣ��������ʡ�ԡ� 
 //����һ��ȫ����Ŀ��˵���߼�ֵ���Ϊ1��
 0x25, 0x01, //     LOGICAL_MAXIMUM (1)
 
 //����һ��ȫ����Ŀ��˵������������Ϊ5���� 
 0x95, 0x05, //   REPORT_COUNT (5)
 
 //����һ��ȫ����Ŀ��˵��������ĳ���Ϊ1bit��
 0x75, 0x01, //   REPORT_SIZE (1)
 
 //����һ��ȫ����Ŀ��˵��ʹ�õ���;ҳΪָʾ�ƣ�LED��
 0x05, 0x08, //   USAGE_PAGE (LEDs)
 
 //����һ���ֲ���Ŀ��˵����;��СֵΪ���ּ��̵ơ�
 0x19, 0x01, //   USAGE_MINIMUM (Num Lock)
 
 //����һ���ֲ���Ŀ��˵����;���ֵΪKana�ơ�
 0x29, 0x05, //   USAGE_MAXIMUM (Kana)
 
 //����һ������Ŀ������������ݣ���ǰ�涨���5��LED��
 0x91, 0x02, //   OUTPUT (Data,Var,Abs)
 
 //����һ��ȫ����Ŀ������λ������Ϊ1����
 0x95, 0x01, //   REPORT_COUNT (1)
 
 //����һ��ȫ����Ŀ������λ�򳤶�Ϊ3bit��
 0x75, 0x03, //   REPORT_SIZE (3)
 
 //����һ������Ŀ���������������ǰ������5bit������������Ҫ
 //3��bit���ճ�һ�ֽڡ�
 0x91, 0x03, //   OUTPUT (Cnst,Var,Abs)
 
 //�����������Ŀ�����ر�ǰ��ļ��ϡ�bSizeΪ0�����Ժ���û���ݡ�
 0xc0,        // END_COLLECTION
//����ע�Ͳ�������һ�ֽڱ���ID��
//ͨ������ı����������Ķ��壬����֪�����ص����뱨�����8�ֽڡ�
//��һ�ֽڵ�8��bit������ʾ������Ƿ��£�����Shift��Alt�ȼ�����
//�ڶ��ֽ�Ϊ����ֵ��ֵΪ����0���������ڰ��ֽ���һ����ͨ����ֵ��
//���飬��û�м�����ʱ��ȫ��6���ֽ�ֵ��Ϊ0����ֻ��һ����ͨ������ʱ��
//�������ֽ��еĵ�һ�ֽ�ֵ��Ϊ�ð����ļ�ֵ������ļ�ֵ�뿴HID��
//��;���ĵ��������ж����ͨ��ͬʱ����ʱ����ͬʱ������Щ���ļ�ֵ��
//������µļ�̫�࣬���������ֽڶ�Ϊ0xFF�����ܷ���0x00����������
//����ϵͳ��Ϊ���м����Ѿ��ͷţ������ڼ�ֵ�������е��Ⱥ�˳����
//����ν�ģ�����ϵͳ�Ḻ�����Ƿ����¼����¡�����Ӧ�����ж϶˵�1
//�а�������ĸ�ʽ����ʵ�ʵļ������ݡ����⣬�����л�������һ���ֽ�
//��������棬����������LED����ġ�ֻʹ���˵�7λ����1λ�Ǳ���ֵ0��
//��ĳλ��ֵΪ1ʱ�����ʾ��Ӧ��LEDҪ����������ϵͳ�Ḻ��ͬ������
//����֮���LED����������������̣�һ������ּ��̵���ʱ����һ��
//Ҳ������������̱�����Ҫ�жϸ���LEDӦ�ú�ʱ������ֻ�ǵȴ�����
//���ͱ��������Ȼ����ݱ���ֵ��������Ӧ��LED�������ڶ˵�1����ж�
//�ж�����1�ֽڵ�������棬Ȼ�����ȡ������Ϊѧϰ���ϵ�LED�ǵ͵�ƽʱ
//������ֱ�ӷ��͵�LED�ϡ�����main�����а�������LED�Ĵ���Ͳ���Ҫ�ˡ�
};
#endif

void UsbInitDescriptor()
{
	u16 wConfigLen = 0;
	u16 *pConfigLen = NULL;
	u8 byIntfNum = 0;
	
	u16 wACDescOffset = 0x09;  //sizeof CONFIG_DESCRIPTOR
	u16 wAudioCtrlLen = 0;
	u16 *pAudioCtrlLen = NULL;

	wConfigLen = sizeof(ConfigDescriptor);
	pConfigLen = (u16*)(ConfigDescriptor + 2);
	*pConfigLen = wConfigLen;
	
#ifdef USB_PRINTER
	byIntfNum += 1;
	wACDescOffset += 0x09 + 0x07 + 0x07;  //sizeof printer interface and endpoint descriptor
#endif

#if defined (USB_MICROPHONE) || defined (USB_SPEAKER)
	byIntfNum += 1;
	wACDescOffset += 0x09;  //sizeof AUDIO_CONTROL_STD_IF
#endif

#if defined (USB_MICROPHONE) && defined (USB_SPEAKER)
	wAudioCtrlLen += 0x0a;
#elif defined (USB_MICROPHONE) || defined (USB_SPEAKER)
	wAudioCtrlLen += 0x09;
#endif

#ifdef USB_MICROPHONE
	byIntfNum += 1;
	wAudioCtrlLen += 0x0c + 0x09; //size of microphone input and output descriptor
#endif

#ifdef USB_SPEAKER
	byIntfNum += 1;
	wAudioCtrlLen += 0x0c + 0x09; //size of speaker input and output descriptor
#endif

#ifdef USB_MOUSE
	byIntfNum += 1;
#endif

#ifdef USB_KEYBOARD
	byIntfNum += 1;
#endif
	
	*(ConfigDescriptor + 4) = byIntfNum;

#if defined (USB_MICROPHONE) || defined (USB_SPEAKER)
	pAudioCtrlLen = (u16*)(ConfigDescriptor + wACDescOffset + 5);
	*pAudioCtrlLen = wAudioCtrlLen;
#endif
	return;
}

///------------------------------------------------------------------------
void SendDescriptorData(void) 
{
  short i = 0;
  short n = 0;
  if(g_length < 8)
  {
	  n = g_length;
  }
  else
  {
	  n = 8;
  }

  g_length = g_length - n;
  WRITE_REG8(EDP0_PTR, 0);
  for (i=0; i<n; i++) 
  {
	  WRITE_REG8(EDP0_DAT, *g_table);
	  g_table++;
  }
}

void HandleGetDescriptor(void)
{
	short i, n;
	if(g_length < 8)
	{
		n = g_length;
	}
	else
	{
		n = 8;
	}
	g_length = g_length - n;
	WRITE_REG8(EDP0_PTR, 0);
	for (i=0; i<n; i++) 
	{
		WRITE_REG8(EDP0_DAT, *g_table);
	    g_table++;
	}
}

void ProcStdDevRequest()
{
	if (usb_valueh==1) 
	{
        g_table = DeviceDescriptor;
        g_length = sizeof (DeviceDescriptor);
    }
    else if (usb_valueh==2) 
	{
        g_table = ConfigDescriptor;
        g_length = sizeof (ConfigDescriptor);
    }
    else if (usb_valueh==3 && usb_valuel==0) 
	{
        g_table = StringLangID;
        g_length = sizeof (StringLangID);
    }
    else if (usb_valueh==3 && usb_valuel==0x1) 
	{
        g_table = StringVendor;
        g_length = sizeof (StringVendor);
    }
    else if (usb_valueh==3 && usb_valuel==0x2) 
	{
        g_table = StringProduct;
        g_length = sizeof (StringProduct);
    }
    else if (usb_valueh==3 && usb_valuel==0x3) 
	{
        g_table = StringSerial;
        g_length = sizeof (StringSerial);
    }
    else
	{
		g_stall = 1;
	}

    if (usb_length < g_length)
	{
		g_length = usb_length;
	}
	return;
}

void ProcStdIntfRequest()
{
	switch(usb_valueh)
	{
		// HID Descriptor
	case 0x21:
		if(usb_indexl == 4)          //usb_indexl is the interface number
		{//mouse
#ifdef USB_MOUSE
			g_table = MouseHidDesc;
			g_length = sizeof (MouseHidDesc);
#endif
		}
		else if(usb_indexl == 5)
		{//keyboard
#ifdef USB_KEYBOARD
			g_table = KeyboardHidDesc;
			g_length = sizeof(KeyboardHidDesc);
#endif
		}
		break;
		
		//Report Descriptor
	case 0x22:
		if(usb_indexl == 4)
		{//mouse
#ifdef USB_MOUSE
			g_table = MouseReportDesc;
			g_length = sizeof (MouseReportDesc);
			//printf("Send mouse report descriptor. \n");
#endif
		}
		else if(usb_indexl == 5)
		{//keyboard
#ifdef USB_KEYBOARD
			g_table = KeyboardReportDesc;
			g_length = sizeof(KeyboardReportDesc);
			//printf("Send keyboard report descriptor. \n");
#endif
		}
		break;			
		// Phisical Descriptor
	case 0x23:
		// TODO
		break;			
		// other condition
	default:
		break;
	}
	if (usb_length < g_length)
	{
		g_length = usb_length;
	}
	return;
}

void ProcOutClassIntfRequest(int nDataStage)
{
	switch(usb_request)
	{
		// set_report
	case 0x09:
		switch(usb_valueh)
		{
			// report_type_input
		case 0x01:
			// TODO
			break;
			
			// report_type_output
		case 0x02:
			// usb_hid_set_report_ouput();
			break;
			
			// report_type_feature
		case 0x03:
			// TODO
			break;
			
		default:
			break;
		}
		break;
		
		// set_idle (used for idle test)              //??
		case 0x0a:
			if(nDataStage)
			{
				WRITE_REG8(EDP0_PTR, 0);
				g_rate = READ_REG8(EDP0_DAT); //??
			}
			g_rate = usb_valueh; //??
			break;
			
			// set_protocol                              //??
		case 0x0b:
			if(nDataStage)
			{
				WRITE_REG8(EDP0_PTR, 0);
				g_protocol = READ_REG8(EDP0_DAT);
			}
			g_protocol = usb_valuel;
			break;
	}
}

void ProcInClassIntfRequest()
{
	switch(usb_request)
	{
		// get_hid_input
	case 0x01:
		WRITE_REG8(EDP0_PTR, 0);
		WRITE_REG8(EDP0_DAT, 0x81);
		WRITE_REG8(EDP0_DAT, 0x02);
		WRITE_REG8(EDP0_DAT, 0x55);
		WRITE_REG8(EDP0_DAT, 0x55);
		break;
		
		// get_idle (used for idle test)
	case 0x02:
		WRITE_REG8(EDP0_PTR, 0);
		WRITE_REG8(EDP0_DAT, g_rate);
		break;
		
		// get_protocol
	case 0x03:
		WRITE_REG8(EDP0_PTR, 0);
		WRITE_REG8(EDP0_DAT, g_protocol);
		break;
		
	default:
		break;
	}
	return;
}

void ProcSetInterface()
{
	WRITE_REG8(EDP0_PTR,0);   //address
	WRITE_REG8 (EDP0_CTL, BIT0);	//Ack Data
	return;
}

void PerformRequest(int nDataStage) 
{
	if ( (bm_request==0x80) && (usb_request==0x06) ) {
		if (nDataStage == 0) 
		{
			ProcStdDevRequest();
		}
		HandleGetDescriptor();
	}
	else if ( (bm_request==0x81) && (usb_request==0x06))
	{
		if(nDataStage == 0)
		{
			ProcStdIntfRequest();
		}
		HandleGetDescriptor ();
	}
	else if (bm_request==0x21)  
	{
		ProcOutClassIntfRequest(nDataStage);
	}
	else if (bm_request==0xa1)  
	{
		ProcInClassIntfRequest();
	}
	else if( (bm_request == 0x01) && (usb_request == 0x0b) )
	{
		ProcSetInterface();
	}
	else
	{
		g_stall = 1;
	}
}

void Edp0Setup() 
{
	WRITE_REG8(EDP0_PTR,0);   //address
	bm_request	= READ_REG8(EDP0_DAT);
	usb_request	= READ_REG8(EDP0_DAT);
	usb_valuel	= READ_REG8(EDP0_DAT);
	usb_valueh	= READ_REG8(EDP0_DAT);
	usb_indexl	= READ_REG8(EDP0_DAT);
	usb_indexh	= READ_REG8(EDP0_DAT);
	usb_length	= READ_REG8(EDP0_DAT);
	usb_length	+= READ_REG8(EDP0_DAT) << 8;
	g_stall = 0;
	PerformRequest(0);
	if ( g_stall )
		WRITE_REG8(EDP0_CTL, BIT1);	//Stall Data
	else
		WRITE_REG8(EDP0_CTL, BIT0);	//Ack Data
}

void Edp0Data(void) 
{
	PerformRequest(1);
	if ( g_stall )
		WRITE_REG8(EDP0_CTL, BIT1);	//Stall Data
	else
		WRITE_REG8(EDP0_CTL, BIT0);	//Ack Data
}

void Edp0Status() 
{
	if ( g_stall )
		WRITE_REG8(EDP0_CTL, BIT3);	//Stall status
	else
		WRITE_REG8(EDP0_CTL, BIT2);	//Ack status
}

#if 0
void SendTestData()
{
	volatile static int s_nTimer = 0;
	volatile static char s_chInput = 0x59;
	int nDataIndx = 0;
	char achInput[8] = {0};

	s_nTimer = ( s_nTimer + 1 ) % 1000000;
	if(s_nTimer == 1000000 - 1)
	{
//		printf("Send keyboard data 0x%0x \n", s_chInput);
#if 0
		achInput[2] = s_chInput++;
		if(s_chInput > 0x61)
		{
			s_chInput = 0x59;
		}

		write_reg8(EDP2_PTR, 0);
		for(nDataIndx = 0; nDataIndx < 8; nDataIndx++)
		{
			write_reg8(EDP2_DAT, achInput[nDataIndx]);
		}
		write_reg16(EDP2_CTL, 0x01);
#endif
	}
	else if(s_nTimer == 500000)
	{
//		printf("Send mouse data. \n");
#if 1
		achInput[1] = 10;
		write_reg8(EDP1_PTR, 0);
		for(nDataIndx = 0; nDataIndx < 4; nDataIndx++)
		{
			write_reg8(EDP1_DAT, achInput[nDataIndx]);
		}
		write_reg16(EDP1_CTL, 0x01);
#endif
	}
	return;
}
#endif
void UsbTask()
{	  
    unsigned int irq_src = IRQSRC; //get interrupt source
  
    if (irq_src & IRQEDP0SETUP) {
    	WRITE_REG8(EDP0_STA, BIT4);    //clear setup irq
        Edp0Setup();
    }
    if (irq_src & IRQEDP0DATA) {
    	WRITE_REG8(EDP0_STA, BIT5);    //clear data irq
        Edp0Data( );
    }
    if (irq_src & IRQEDP0STATUS) {
    	WRITE_REG8(EDP0_STA, BIT6);    //clear status irq
        Edp0Status();
    }

}

void UsbIrqHandle() 
{
    unsigned int irq_src = IRQSRC; //get interrupt source
    if (irq_src & IRQEDPS) {
		    irq_edps = READ_REG8(EDPS_IRQ);
		    WRITE_REG8(EDPS_IRQ, irq_edps);
    }
	return;
}

void UsbInitInterrupt()
{
	WRITE_REG8(0x13c, 0x40);  //set Minimum threshold  to ACK endpoint 8 transfer
	
	WRITE_REG8(USB_MODE, 0xff & ~BIT7 & ~BIT5);// enable manuual mode of standard and get descriptor request
	USBMASK |= BIT7|BIT6; //enable endpoint7/endpoint6's interrupt(endpoint7/endpoint6 is taken as Audio input/output)
	IRQMSK |= IRQEDPS; //enable IRQEDPS interrupt
	return;
}

void UsbInit() 
{
	UsbInitInterrupt();

	/****** set buf address and size of endpoint7******/
	EP6BUFADDR = 0x80;
	EP7BUFADDR  = 0xC0;
	EPBUFSIZE = (256 >> 2);

	UsbInitDescriptor();

	return;
}
#define   N   7

#if 0
void WriteEndPoint(int nEnpNum, unsigned char * pData, int nDataLen)
{
	int nIndx = 0;
	write_reg8(EDPS_PTR + nEnpNum, 0);
	for(nIndx = 0; nIndx < nDataLen; nIndx++)
	{
		write_reg8(EDPS_DAT + nEnpNum, pData[nIndx]);
	}
	write_reg8(EDPS_CTL + nEnpNum, BIT0);
}
#endif

