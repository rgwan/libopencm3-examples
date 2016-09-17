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
    0x05, 0x01, /* Usage Page (Generic Desktop)             */
    0x09, 0x06, /*		Usage (Keyboard)                    */
    0xA1, 0x01, /*		Collection (Application)            */
    0x85, 0x01,  /*   Report ID 1  */
    0x05, 0x07, /*  	Usage (Key codes)                   */
    0x19, 0xE0, /*      Usage Minimum (224)                 */
    0x29, 0xE7, /*      Usage Maximum (231)                 */
    0x15, 0x00, /*      Logical Minimum (0)                 */
    0x25, 0x01, /*      Logical Maximum (1)                 */
    0x75, 0x01, /*      Report Size (1)                     */
    0x95, 0x08, /*      Report Count (8)                    */
    0x81, 0x02, /*      Input (Data, Variable, Absolute)    */
    0x95, 0x01, /*      Report Count (1)                    */
    0x75, 0x08, /*      Report Size (8)                     */
    0x81, 0x01, /*      Input (Constant)    ;5 bit padding  */
    0x95, 0x05, /*      Report Count (5)                    */
    0x75, 0x01, /*      Report Size (1)                     */
    0x05, 0x08, /*      Usage Page (Page# for LEDs)         */
    0x19, 0x01, /*      Usage Minimum (01)                  */
    0x29, 0x05, /*      Usage Maximum (05)                  */
    0x91, 0x02, /*      Output (Data, Variable, Absolute)   */
    0x95, 0x01, /*      Report Count (1)                    */
    0x75, 0x03, /*      Report Size (3)                     */
    0x91, 0x01, /*      Output (Constant)                   */
    0x95, 0x06, /*      Report Count (1)                    */
    0x75, 0x08, /*      Report Size (3)                     */
    0x15, 0x00, /*      Logical Minimum (0)                 */
    0x25, 0x65, /*      Logical Maximum (101)               */
    0x05, 0x07, /*  	Usage (Key codes)                   */
    0x19, 0x00, /*      Usage Minimum (00)                  */
    0x29, 0x65, /*      Usage Maximum (101)                 */
    0x81, 0x00, /*      Input (Data, Array)                 */
    0xC0        /* 		End Collection,End Collection       */
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
	int reset = 0;
	
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

	for (i = 0; i < 0x10000; i++)
		__asm__("nop");	
	
	//ps2_tx(0xaa);/* POST successfully */
	
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
	gpio_set(GPIOA, GPIO8);
	int j;
	i = 0;
	while(ps2_tx(0xaa))
	{
		for (j = 0; j < 1000; j++)
			__asm__("nop");	
		i++;
		if(i > 8)
			break;
	}
	printf("Now entry loop\r\n");
	
	/*i = 512;
	while(i--)
	{
		ps2_poll();
		if(ps2_cmd)
		{
			cli();
			printf("Recieved 0x%02x from host\r\n", ps2_cmd);
			if(ps2_cmd == 0xff)
				ps2_tx(0xaa);
			else
				ps2_tx(0xfa);
			if(ps2_cmd == 0xf2)
				ps2_tx(0xab);
			ps2_cmd = 0;
			sei();
		}
	}*/
	while (1)
	{
		ps2_poll();
		if(ps2_cmd)
		{
			cli();
			printf("Recieved 0x%02x from host\r\n", ps2_cmd);
			if(ps2_cmd != 0xfe && ps2_cmd != 0xee)
				ps2_tx(0xfa);
			if(ps2_cmd == 0xee)
				ps2_tx(0xee);
			if(ps2_cmd == 0xff && reset < 8)
			{
				reset++;
				i = 0;
				while(ps2_tx(0xaa))
				{
					for (j = 0; j < 1000; j++)
						__asm__("nop");	
					i++;
					if(i > 8)
						break;
				}
			}
			if(ps2_cmd == 0xf2)
			{
				ps2_tx(0xab);
				ps2_tx(0x83);
			}
			if(ps2_cmd == 0xed)
				printf("LED change\r\n");
			ps2_cmd = 0;
			sei();
		}
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
							ps2_tx(0xe0);
							ps2_tx(PS2_RELEASED);
							ps2_tx(PS2_EX_SCRN & 0xff);
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
								ps2_tx(0xe0);
							ps2_tx(PS2_RELEASED);
							ps2_tx(key1);
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
							ps2_tx(PS2_RELEASED);
							ps2_tx(key2);
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
							ps2_tx(PS2_L_CTRL);
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
								ps2_tx(0xe0);
							ps2_tx(key1);
							//exti_reset_request(EXTI13);
							//sei();
							printf("tx key 1 down %d\r\n", key1);
						}
						if(key2)
						{
							//cli();
							ps2_tx(key2);
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
			ps2_tx(PS2_RELEASED);
			ps2_tx(PS2_0);
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
			ps2_tx(PS2_0);
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
		ps2_tx(PS2_RELEASED);
		ps2_tx(PS2_L_CTRL);
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
		ps2_tx(0xe0);
		ps2_tx(PS2_EX_SCRN & 0xff);
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
		//ps2_tx(0x1c);
		//printf("Still alive\r\n"); 
		gpio_toggle(GPIOB, GPIO11);
		cnt = 0;
	}
}

