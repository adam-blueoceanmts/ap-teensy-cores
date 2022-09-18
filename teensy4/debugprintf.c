#pragma GCC optimize ("-Os")

#include "debug/printf.h"

#ifdef PRINT_DEBUG_STUFF

#include "avr/pgmspace.h"
#include <stdarg.h>
#include "imxrt.h"
 #include "usb_desc.h"

void putchar_debug(char c);
static void puint_debug(unsigned int num);


__attribute__((section(".startup"))) void printf_debug(const char *format, ...)
{
	va_list args;
	unsigned int val;
	int n;

	va_start(args, format);
	for (; *format != 0; format++) { // no-frills stand-alone printf
		if (*format == '%') {
			++format;
			if (*format == '%') goto out;
			if (*format == '-') format++; // ignore size
			while (*format >= '0' && *format <= '9') format++; // ignore size
			if (*format == 'l') format++; // ignore long
			if (*format == '\0') break;
			if (*format == 's') {
				printf_debug((char *)va_arg(args, int));
			} else if (*format == 'd') {
				n = va_arg(args, int);
				if (n < 0) {
					n = -n;
					putchar_debug('-');
				}
				puint_debug(n);
			} else if (*format == 'u') {
				puint_debug(va_arg(args, unsigned int));
			} else if (*format == 'x' || *format == 'X') {
				val = va_arg(args, unsigned int);
				for (n=0; n < 8; n++) {
					unsigned int d = (val >> 28) & 15;
					putchar_debug((d < 10) ? d + '0' : d - 10 + 'A');
					val <<= 4;
				}
			} else if (*format == 'c' ) {
				putchar_debug((char)va_arg(args, int));
			}
		} else {
			out:
			if (*format == '\n') putchar_debug('\r');
			putchar_debug(*format);
		}
	}
	va_end(args);
}

__attribute__((section(".startup"))) static void puint_debug(unsigned int num)
{
	char buf[12];
	unsigned int i = sizeof(buf)-2;

	buf[sizeof(buf)-1] = 0;
	while (1) {
		buf[i] = (num % 10) + '0';
		num /= 10;
		if (num == 0) break;
		i--;
	}
	printf_debug(buf + i);
}

// first is this normal Serial?
#if defined(PRINT_DEBUG_USING_USB) && defined(CDC_STATUS_INTERFACE) && defined(CDC_DATA_INTERFACE)
#include "usb_dev.h"
#include "usb_serial.h"
__attribute__((section(".startup"), weak)) void putchar_debug(char c)
{
	usb_serial_putchar(c);
}

__attribute__((section(".startup"), weak)) void printf_debug_init(void) {}

#elif defined(PRINT_DEBUG_USING_USB) && defined(SEREMU_INTERFACE) && !defined(CDC_STATUS_INTERFACE) && !defined(CDC_DATA_INTERFACE)
#include "usb_dev.h"
#include "usb_seremu.h"
__attribute__((section(".startup"), weak)) void putchar_debug(char c)
{
	usb_seremu_putchar(c);
}

__attribute__((section(".startup"), weak)) void printf_debug_init(void) {}

#else
__attribute__((section(".startup"), weak)) void putchar_debug(char c)
{
	while (!(LPUART3_STAT & LPUART_STAT_TDRE)) ; // wait
	LPUART3_DATA = c;
}

__attribute__((section(".startup"), weak)) void printf_debug_init(void)
{
        CCM_CCGR0 |= CCM_CCGR0_LPUART3(CCM_CCGR_ON); // turn on Serial4
        IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_06 = 2; // Arduino pin 17
#if PRINT_DEBUG_STUFF == 2
        LPUART3_BAUD = LPUART_BAUD_OSR(11) | LPUART_BAUD_SBR(1); // 2 MBaud
#else
        LPUART3_BAUD = LPUART_BAUD_OSR(25) | LPUART_BAUD_SBR(8); // ~115200 baud
#endif
        LPUART3_CTRL = LPUART_CTRL_TE;
}
#endif




#endif // PRINT_DEBUG_STUFF
