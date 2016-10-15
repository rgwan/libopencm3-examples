/* STM32-Keyboard */

#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/stm32/usart.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "key_map.h"
#include <libopencm3/stm32/exti.h>

#define FALLING 0
#define RISING 1

uint16_t exti_direction = FALLING;

/* Define this to include the DFU APP interface. */
//#define INCLUDE_DFU_INTERFACE

#ifdef INCLUDE_DFU_INTERFACE
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/dfu.h>
#endif
 _Bool configured = 0;
 uint8_t ps2_cmd = 0;
int _write(int file, char *ptr, int len);

const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5710,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01, /* Usage Page (Generic Desktop)	     */
    0x09, 0x06, /*		Usage (Keyboard)		    */
    0xA1, 0x01, /*		Collection (Application)	    */
    0x85, 0x01,  /*   Report ID 1  */
    0x05, 0x07, /*  	Usage (Key codes)		   */
    0x19, 0xE0, /*      Usage Minimum (224)		 */
    0x29, 0xE7, /*      Usage Maximum (231)		 */
    0x15, 0x00, /*      Logical Minimum (0)		 */
    0x25, 0x01, /*      Logical Maximum (1)		 */
    0x75, 0x01, /*      Report Size (1)		     */
    0x95, 0x08, /*      Report Count (8)		    */
    0x81, 0x02, /*      Input (Data, Variable, Absolute)    */
    0x95, 0x01, /*      Report Count (1)		    */
    0x75, 0x08, /*      Report Size (8)		     */
    0x81, 0x01, /*      Input (Constant)    ;5 bit padding  */
    0x95, 0x05, /*      Report Count (5)		    */
    0x75, 0x01, /*      Report Size (1)		     */
    0x05, 0x08, /*      Usage Page (Page# for LEDs)	 */
    0x19, 0x01, /*      Usage Minimum (01)		  */
    0x29, 0x05, /*      Usage Maximum (05)		  */
    0x91, 0x02, /*      Output (Data, Variable, Absolute)   */
    0x95, 0x01, /*      Report Count (1)		    */
    0x75, 0x03, /*      Report Size (3)		     */
    0x91, 0x01, /*      Output (Constant)		   */
    0x95, 0x06, /*      Report Count (1)		    */
    0x75, 0x08, /*      Report Size (3)		     */
    0x15, 0x00, /*      Logical Minimum (0)		 */
    0x25, 0x65, /*      Logical Maximum (101)	       */
    0x05, 0x07, /*  	Usage (Key codes)		   */
    0x19, 0x00, /*      Usage Minimum (00)		  */
    0x29, 0x65, /*      Usage Maximum (101)		 */
    0x81, 0x00, /*      Input (Data, Array)		 */
    0xC0	/* 		End Collection,End Collection       */
};

static const struct {
	struct usb_hid_descriptor hid_descriptor;
	struct {
		uint8_t bReportDescriptorType;
		uint16_t wDescriptorLength;
	} __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
	.hid_descriptor = {
		.bLength = sizeof(hid_function),
		.bDescriptorType = USB_DT_HID,
		.bcdHID = 0x0100,
		.bCountryCode = 0,
		.bNumDescriptors = 1,
	},
	.hid_report = {
		.bReportDescriptorType = USB_DT_REPORT,
		.wDescriptorLength = sizeof(hid_report_descriptor),
	},
};

const struct usb_endpoint_descriptor hid_endpoint = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 9,
	.bInterval = 0x01,
};

const struct usb_interface_descriptor hid_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_HID,
	.bInterfaceSubClass = 1, /* boot */
	.bInterfaceProtocol = 1, /* keyboard */
	.iInterface = 0,

	.endpoint = &hid_endpoint,

	.extra = &hid_function,
	.extralen = sizeof(hid_function),
};

#ifdef INCLUDE_DFU_INTERFACE
const struct usb_dfu_descriptor dfu_function = {
	.bLength = sizeof(struct usb_dfu_descriptor),
	.bDescriptorType = DFU_FUNCTIONAL,
	.bmAttributes = USB_DFU_CAN_DOWNLOAD | USB_DFU_WILL_DETACH,
	.wDetachTimeout = 255,
	.wTransferSize = 1024,
	.bcdDFUVersion = 0x011A,
};

const struct usb_interface_descriptor dfu_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = 0xFE,
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 1,
	.iInterface = 0,

	.extra = &dfu_function,
	.extralen = sizeof(dfu_function),
};
#endif

const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &hid_iface,
#ifdef INCLUDE_DFU_INTERFACE
}, {
	.num_altsetting = 1,
	.altsetting = &dfu_iface,
#endif
}};

const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
#ifdef INCLUDE_DFU_INTERFACE
	.bNumInterfaces = 2,
#else
	.bNumInterfaces = 1,
#endif
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0xC0,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Zhiyuan Wan Workshop",
	"USB-Keyboard",
	"For someone",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

usbd_device *usb_dev;

static int hid_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
			void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)usbd_dev;

	if ((req->bmRequestType != 0x81) ||
	   (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
	   (req->wValue != 0x2200))
		return 0;

	/* Handle the HID report descriptor. */
	*buf = (uint8_t *)hid_report_descriptor;
	*len = sizeof(hid_report_descriptor);

	return 1;
}

#ifdef INCLUDE_DFU_INTERFACE
static void dfu_detach_complete(usbd_device *usbd_dev, struct usb_setup_data *req)
{
	(void)req;
	(void)usbd_dev;

	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, 0, GPIO15);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO10);
	gpio_set(GPIOA, GPIO10);
	scb_reset_core();
}

static int dfu_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
			void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)buf;
	(void)len;
	(void)usbd_dev;

	if ((req->bmRequestType != 0x21) || (req->bRequest != DFU_DETACH))
		return 0; /* Only accept class request. */

	*complete = dfu_detach_complete;

	return 1;
}
#endif

static void hid_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;
	(void)usbd_dev;

	usbd_ep_setup(usbd_dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 4, NULL);

	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				hid_control_request);
#ifdef INCLUDE_DFU_INTERFACE
	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				dfu_control_request);
#endif
	configured = 1;
	printf("USB device configured\r\n");
}

static void gpio_init(void)
{
	gpio_clear(GPIOA, GPIO8);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO11);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_OPENDRAIN, 0x3ff);
	gpio_set(GPIOB, 0x3ff);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
			GPIO_CNF_INPUT_PULL_UPDOWN, 0xff);
	gpio_set(GPIOA, 0xff);
	gpio_set(GPIOB, GPIO13 | GPIO15);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
			GPIO_CNF_OUTPUT_OPENDRAIN, GPIO13 | GPIO15);	//PS2
	AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON ;
}

static void vector_init(void)
{
	/* If you 're using STM32 's bootloader via go function, it has configured the vector address to system flash */
	/* So you have to configure it back */
	SCB_VTOR = 0x08000000;
}

static void usart_init(void)
{
	/* Setup GPIO pin GPIO_USART1_RE_TX on GPIO port A for transmit. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

	/* Setup UART parameters. */
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX);

	/* Finally enable the USART. */
	usart_enable(USART1);
}

int _write(int file, char *ptr, int len)
{
	int i;

	if (file == 1) {
		for (i = 0; i < len; i++)
			usart_send_blocking(USART1, ptr[i]);
		return i;
	}

	errno = EIO;
	return -1;
}
void delay_us(int i)
{
	int j;
	while(i--)
		for(j = 0; j < 25; j++)
			__asm__("nop");
}

void AK_to_host()
{
    delay_us(13);
    DAT_OUT_0;
    delay_us(4);
    CLK_OUT_0;
    delay_us(36);
    CLK_OUT_1;
    delay_us(4);
    DAT_OUT_1;
}

/*******************************************************
*-函数名称	：
*-函数作用	：从主机读数据
*-参数		：
*-返回值   	：0 接收错误；
*-备注		： 从主机发送到键盘/鼠标的数据在上升沿 当时钟从低变到高的时候 被读取 。
*******************************************************/
unsigned char PS2_DAT_R()
{
    unsigned char dat = 0, i = 0,j = 0,k = 0,send_flag = 0;	     //j,k 校验用
    int delay_t = 20000;		   //while 循环里递减计数
    cli();
    CLK_OUT_1;
    DAT_OUT_1;
    
    if(!CLK_IN)			   //时钟被主机拉低
    {   
	printf("Read from host:\r\n");
	
	while((!CLK_IN))// && (delay_t > 0))   //等待主机释放时钟
	{
	    
	    if(!DAT_IN) send_flag = 1;
	   // delay_t--;
	}
	
	
	if(!DAT_IN) send_flag = 1;
	   
	if(send_flag == 0)
	{
	    printf("ERROR,PS2_DAT is high while host pull CLK\r\n");
	    dat = 0;
	    goto error;			      //判断数据线如果为高就产生错误，放弃接受数据
	}
	/*if(delay_t == 0)
	{
	    printf("wait for host release PS2_CLK=1 timeout!\r\n");
	    dat = 0;
	    goto error;
	    //return 0;			      //判断数据线如果为高就产生错误，放弃接受数据
	}  */
	
	//读8位数据
	for(i=0;i<8;i++)
	{
	    delay_us(delaytime);
	    CLK_OUT_0;
	    delay_us(delaytime);
	    delay_us(delaytime);
	    CLK_OUT_1;
	    delay_us(delaytime);
	    if(DAT_IN)  
	    {
		dat |= ((unsigned char)1<<i) ;
		j++;
	    }
	}
	
	//读校验位
	delay_us(delaytime);
	CLK_OUT_0;
	delay_us(delaytime);
	delay_us(delaytime);
	CLK_OUT_1;
	delay_us(delaytime);
	
	k = 0;
	if(DAT_IN)
	k = 1;//保存校验
	
	//读停止位
	delay_us(delaytime);
	CLK_OUT_0;
	delay_us(delaytime);
	delay_us(delaytime);
	CLK_OUT_1;
	delay_us(delaytime);
       
	
       // 数据线仍旧为 0 吗
       // 是 保持时钟直到数据 1 然后产生一个错误
       if(!DAT_IN)
       {
	   //持续产生时钟
	    while(!DAT_IN)
	    {
		delay_us(delaytime);
		CLK_OUT_0;
		delay_us(delaytime);
		delay_us(delaytime);
		CLK_OUT_1;
		delay_us(delaytime);
	    }
	    printf("DAT IS LOW AFTER SOTP BIT\r\n");
       }
	//从机应答
	AK_to_host();
       
       //检查校验位
	if(k == (j%2)) 
	{
	    printf("parity bit FAIL\r\n");//校验失败
	    return (0x10 + dat);
	}	    
       
       //延时，给主机时间抑制下次的传送
       //delay_us(45);
       printf("dt=%d\r\n", delay_t);
    }
error:
    sei();
    return dat;
}

/*******************************************************
*-函数名称	：PS2键盘发送函数
*-函数作用	：从键盘/鼠标发送到主机的数据在时钟信号的下降沿 当时钟从高变到低的时候 被读取
	     
*-参数		：
*-返回值	    ：0 发送成功，1发送失败
*-备注		：
*******************************************************/

unsigned char  PS2_DAT_W(unsigned char byte)
{
    unsigned int i = 0,j=0;
    cli();
    CLK_OUT_1;
    DAT_OUT_1;
    //起始位0
     
    while(!CLK_IN)  
    {
	printf("PS2_CLK is unrelease\r\n");
	i++;
	if(i > 1000)
		goto out;
    }
    
    
    //1个起始位 总是为 0   
    DAT_OUT_0;	 
    delay_us(delaytime);

    
    CLK_OUT_0; 
    delay_us(delaytime);
    delay_us(delaytime);
   
    CLK_OUT_1;
    delay_us(delaytime);
    
    //8位数据收发
    for(i=0;i<8;i++)
    {	
	//装载要从键盘发送到主机的数据
	if(((byte>>i)&0x01))
	{
	  DAT_OUT_1;
	    j++;
	}
	else DAT_OUT_0;
	delay_us(delaytime);
	
	//把时钟拉低
	CLK_OUT_0;
	delay_us(delaytime);
	delay_us(delaytime);
	
	//释放时钟
	CLK_OUT_1;
	delay_us(delaytime);	
     }
    
    //校验，如果数据位中包含偶数个 1 校验位就会置 1 如果数据位中包含奇数个 1 校验位就会置 0
    
     
//    while(!(GPIOB->IDR & GPIO_Pin_13))
//    {
//	
//	printf("chushihua2\r\n");
//	return SEND_ERRO;    //如果主机拉低时钟线则返回

//    }   
   if(j%2==0)  DAT_OUT_1;
   else  DAT_OUT_0;
    delay_us(delaytime);
    
    CLK_OUT_0;
    delay_us(delaytime);
    delay_us(delaytime); 
    
    //发送停止位 
    CLK_OUT_1;
    delay_us(delaytime);

    DAT_OUT_1;
    delay_us(delaytime);
    
    CLK_OUT_0;
    delay_us(delaytime); 
    delay_us(delaytime);
    
    CLK_OUT_1; 
    delay_us(delaytime * 2);
out:    
    sei();
    return SEND_OK;		       //0 发送成功，1发送失败
}




/*******************************************************
*-函数名称	：PS2键盘接收发送函数
*-函数作用	：从键盘/鼠标发送到主机的数据在时钟信号的下降沿 当时钟从高变到低的时候 被读取
	     
*-参数		：
*-返回值	    ：0 发送成功，1发送失败
*-备注		：
*******************************************************/

unsigned char  PS2_DAT_RW(unsigned char byte)
{
    //判断从主机接收
     if(!CLK_IN)  
    {
	printf("PS2_CLK is unrelease\r\n");
	delay_us(50);
	if(!CLK_IN)  delay_us(50);
	if(!DAT_IN)  return PS2_DAT_R();// Read_from_host();说明主机要发命令
    }
    
    //等待时钟被释放，往主机发送数据
    while(!CLK_IN);
    if(byte == 0) return 0;
    return PS2_DAT_W(byte);
}


/*******************************************************
*-函数名称	：
*-函数作用	：处理从主机接收到的命令
*-参数		：
*-返回值	：
*-备注		：
*******************************************************/
void PS2_DAT_R_A(unsigned char dat)
{
    static unsigned char Lastword = 0; //Lastword保存PS2最后发出的命令或回应
    static unsigned char reset = 0;
    if(dat != 0) 
    {
       printf("HOST : %x\r\n",dat);
	switch (dat)
	{
	    case 0xFF: 
		printf("PS2 : OK & RESET \r\n");
		//led_on();
	    	if(reset < 3)
	    	{
			Lastword = 0XFA;
			PS2_DAT_W(Lastword); 
			//led_off();
			Lastword = 0xAA;
			PS2_DAT_W(Lastword); 
			reset ++;
		}
		else
		{
			printf("More than 3 resets, break\r\n");
		}
		break;
	    
	    case 0XFE:
		printf("PS2 : Resend\r\n");
		PS2_DAT_W(Lastword); 
		break;
	    
	    case 0XFD:
		printf("PS2 :Set Key Type Make\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;
	    
	    case 0XFC:
		printf("PS2 : Set Key Type Make/Break\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;
	    
	    case 0XFB:
		printf("PS2 :Set Key Type Typematic\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;
	    
	    case 0XFA:
		printf("PS2 :Set All Keys Typematic/Make/Break\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;   
	    
	    case 0XF9:
		printf("PS2 : Set All Keys Make\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;   
	    
	    case 0XF8:
		printf("PS2 :Set All Keys Make/Break\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;   
	    
	    case 0XF7:
		printf("PS2 :Set All Keys Typematic\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;   
	    
	    case 0XF6:
		printf("PS2 :Set Default\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;   
	    
	    case 0XF5:  //键盘停止扫描 载入缺省值 键 Set Default 命令 等待进一步指令
		printf("PS2 : Disable\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;  
	    
	    case 0XF4:
		printf("PS2 : Enable\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break; 
	    
	    case 0XF3:			    //主机在这条命令后会发送一个字节的参数来定义机打速率和延时
		printf("PS2 :Set Typematic Rate/Delay\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;   
	    
	    case 0XF2:
		printf("PS2 :Read ID\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		Lastword = 0XAB;
		PS2_DAT_W(Lastword); 
		Lastword = 0X83;
		PS2_DAT_W(Lastword);		
		break;   
	    
	    //主机在这个命令后发送一个字节的参数 是定键盘使用哪套扫描码集
	    //参数字节可以是 0x01 0x02 或 0x03 分别选择扫描码集第一套 第二套或第三套		
	    case 0XF0:
		printf("PS2 : Set Scan Code Set\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break;   
	    
	    case 0xEE:
		printf("PS2 : Echo\r\n");
		Lastword = 0XEE;
		PS2_DAT_W(Lastword); 
		break; 

	    //主机在本命令后跟随一个参数字节 用于指示键盘上 Num Lock, Caps Lock,
	    //and Scroll Lock LED 的状态 
	    //只是最初可用于 PS/2 键盘
	    case 0XED:
		printf("PS2 : Set/Reset LEDs\r\n");
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 
		break; 
	    
	   
	    default:
		
		Lastword = 0XFA;
		PS2_DAT_W(Lastword); 

		printf("PS2 :default\r\n");
		break;
	}
    }
}



//7 * 10 keyboard
/*
 * Keyboard buffer:
 * buf[1]: MOD
 * buf[2]: reserved
 * buf[3]..buf[8] - keycodes 1..6
 */
volatile uint8_t key_table[2][16];
volatile uint8_t hid_buf[9] = {1,0,0,0,0,0,0,0,0};
volatile uint8_t key_ptr = 0;
volatile int8_t hid_keys = 0;

int main(void)
{
	int i;
	unsigned char dat = 0;
	int dat_c = 0;
	vector_init();
	

	//rcc_clock_setup_in_hsi_out_48mhz();
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_USART1);
	memset(key_table, 0xff, sizeof(key_table));
	gpio_init();
	usart_init();

	printf("Early initialized console\r\n");
	usb_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usb_dev, hid_set_config);
	
	nvic_set_priority(NVIC_EXTI15_10_IRQ, 1);
	nvic_set_priority(NVIC_SYSTICK_IRQ, 2);
	
	/* Enable EXTI13 interrupt. */
	
	/*nvic_enable_irq(NVIC_EXTI15_10_IRQ);
	
	exti_select_source(EXTI13, GPIOB);
	exti_direction = FALLING;
	exti_set_trigger(EXTI13, EXTI_TRIGGER_FALLING);
	exti_enable_request(EXTI13);*/
	
	//PS2_DAT_W(0xaa);/* POST successfully */
	
	gpio_set(GPIOB, GPIO13);
	gpio_set(GPIOB, GPIO15);
	
	printf("Self POST operation done\r\n");	
	
	
	/*for (i = 0; i < 0x10000; i++)
		__asm__("nop");*/
	
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(200000);
	systick_interrupt_enable();
	systick_counter_enable();	
	
	gpio_set(GPIOB, GPIO11);

	PS2_DAT_W(0xaa);
	i = 0;	
	printf("Waiting for PS2 connection...\r\n");
	cli();
	while(1)
	{
		dat = PS2_DAT_R();		  //读主机发送到从机的数据
		i++;
		if(dat != 0) PS2_DAT_R_A(dat);
		if(dat == 0xff)  
		{
			dat_c ++;
			if(dat_c>1)
			{
				break;
			}
		   
		}
		i++;
		if(i > 500)
			break;
	}
	printf("Now entry loop\r\n");
	sei();
	gpio_set(GPIOA, GPIO8);
	while (1)
	{
		dat = PS2_DAT_R();		  //读主机发送到从机的数据
       		if(dat != 0) PS2_DAT_R_A(dat);      //从机应答
		usbd_poll(usb_dev);
	}
}



void sys_tick_handler(void)
{//Keyboard scanning
	static int cnt = 0;
	int i;
	int j;
	static uint8_t zzz_key = 0;
	static uint8_t ctrlprt = 0;

	
	
	key_ptr = !key_ptr;
	
	for(i = 0; i < 10; i++)
	{
		if(i != 0)
			gpio_set(GPIOB, 1 << (i - 1));
		gpio_clear(GPIOB, (1 << i));
		for( j = 0; j < 32; j++)
			asm("nop");
		key_table[key_ptr][i] = GPIOA_IDR;
		for( j = 0; j < 32; j++)
			asm("nop");
	}
	gpio_set(GPIOB, 0x3ff);
	
	for( i = 0; i < 10; i++)
	{
		uint8_t tmp = key_table[key_ptr][i] ^ key_table[(!key_ptr)][i];
		if(tmp)
		{
			
			for(j = 0; j < 7;j ++)
			{
				if(tmp & (1 << j))
				{
					uint8_t mod = 0, keycode = 0;
					uint8_t key1 = 0, key2 = 0, ex = 0;
					int k;
					
					mod = key_map[i][j] >> 8;
					keycode = key_map[i][j] & 0xff;
					
					


					if(key_table[key_ptr][i] & (1 << j))
					{
						printf("key %d, %d up mod = %d keycode = %d kp = %d\r\n", i, j, mod, keycode, key_ptr);
						if(i == 9 && j == 0)
						{
							ctrlprt = 4;
							for(k = 3; k < 9; k++)
							{
								if(hid_buf[k] == KEY_PRINTSCREEN)
								{
									hid_buf[k] = 0;
									break;
								}
							}
							//cli();
							PS2_DAT_W(0xe0);
							PS2_DAT_W(PS2_RELEASED);
							PS2_DAT_W(PS2_EX_SCRN & 0xff);
							//exti_reset_request(EXTI13);
							//sei();
							printf("ctrl+prtsc prtsc up\r\n");
							continue;
						}
						if(keycode)
						{
							if(hid_keys > 0)
								hid_keys --;
							for(k = 3; k < 9; k++)
							{
								if(hid_buf[k] == keycode)
								{
									hid_buf[k] = 0;
									break;
								}
							}							
						}
						hid_buf[1] &= ~mod;
						ex = (PS2_map[i][j] >> 16) & 0xff;
						key1 = 	(PS2_map[i][j] >> 8) & 0xff;
						key2 = PS2_map[i][j] & 0xff;
						if(key1)
						{
							//cli();
							if(ex)
								PS2_DAT_W(0xe0);
							PS2_DAT_W(PS2_RELEASED);
							PS2_DAT_W(key1);
							printf("tx key1 up %d\r\n", key1);
							//exti_reset_request(EXTI13);
							//sei();
						}
						if(key2)
						{
							for( k = 0; k < 5; k++)
								asm("nop");
							//cli();
							printf("tx key2 up %d\r\n", key2);
							PS2_DAT_W(PS2_RELEASED);
							PS2_DAT_W(key2);
							//exti_reset_request(EXTI13);
							//sei();
						}				
						
					}
					else
					{
						printf("key %d, %d down hid_keys = %d mod = %d keycode = %d kp = %d\r\n", i, j, hid_keys, mod, keycode, key_ptr);
						if(i == 8 && j == 5 && (zzz_key == 0 || zzz_key == 7))
						{
							zzz_key = 1;
							continue;
						}
						if(i == 9 && j == 0)
						{
							ctrlprt = 1;
							hid_buf[1] |= MOD_CTRL;
							//cli();
							PS2_DAT_W(PS2_L_CTRL);
							//exti_reset_request(EXTI13);
							//sei();
							printf("ctrl+prtsc ctrl down\r\n");
							continue;
						}
						if(hid_keys < 6 && keycode != 0)
						{
							hid_keys ++;
							for(k = 3; k < 9; k++)
							{
								if(hid_buf[k] == 0)
								{
									hid_buf[k] = keycode;
									break;
								}
							}
						}
						hid_buf[1] |= mod;
						ex = (PS2_map[i][j] >> 16) & 0xff;
						key1 = 	(PS2_map[i][j] >> 8) & 0xff;
						key2 = PS2_map[i][j] & 0xff;
						if(key1)
						{
							//cli();
							if(ex)
								PS2_DAT_W(0xe0);
							PS2_DAT_W(key1);
							//exti_reset_request(EXTI13);
							//sei();
							printf("tx key 1 down %d\r\n", key1);
						}
						if(key2)
						{
							//cli();
							PS2_DAT_W(key2);
							//exti_reset_request(EXTI13);
							//sei();
							printf("tx key 2 down %d\r\n", key2);
						}
					}
				}
			}
		}
	}
	if(zzz_key > 0 && zzz_key < 7)
	{
		if(zzz_key % 2 == 0)
		{
			for(i = 3; i < 9; i++)
			{
				if(hid_buf[i] == KEY_0)
				{
					hid_buf[i] = 0;
					break;
				}
			}
			//cli();
			PS2_DAT_W(PS2_RELEASED);
			PS2_DAT_W(PS2_0);
			//exti_reset_request(EXTI13);
			//sei();
			printf("key up 0");
		}
		else
		{
			for(i = 3; i < 9; i++)
			{
				if(hid_buf[i] == 0)
				{
					hid_buf[i] = KEY_0;
					break;
				}
			}
			//cli();
			PS2_DAT_W(PS2_0);
			//exti_reset_request(EXTI13);
			//sei();
			printf("keydown 0");
		}
		printf(" zzz_key = %d\r\n", zzz_key);
		zzz_key++;		
	}
	if(ctrlprt == 5)
	{
		hid_buf[1] &= ~MOD_CTRL;
		//cli();
		PS2_DAT_W(PS2_RELEASED);
		PS2_DAT_W(PS2_L_CTRL);
		//exti_reset_request(EXTI13);
		//sei();
		ctrlprt = 6;
		printf("ctrl+prtsc ctrl up\r\n");
	}
	if(ctrlprt == 4)
	{
		ctrlprt = 5;
	}
	if(ctrlprt == 2)
	{
		//cli();
		PS2_DAT_W(0xe0);
		PS2_DAT_W(PS2_EX_SCRN & 0xff);
		//exti_reset_request(EXTI13);
		//sei();
		
		for(i = 3; i < 9; i++)
		{
			if(hid_buf[i] == 0)
			{
				hid_buf[i] = KEY_PRINTSCREEN;
				break;
			}
		}
		ctrlprt = 3;
		printf("ctrl+prtsc prtsc down\r\n");
	}
	if(ctrlprt == 1)
	{
		ctrlprt = 2;
	}
	
	if(configured)
		usbd_ep_write_packet(usb_dev, 0x81, (void *)hid_buf, 9);
	cnt++;
	if(cnt == 20)
	{
		//printf("cnt = %d \r\n", cnt);
		//PS2_DAT_W(0x1c);
		//printf("Still alive\r\n"); 
		//gpio_toggle(GPIOB, GPIO11);
		cnt = 0;
	}
}

