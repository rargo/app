#ifndef EINT_H
#define EINT_H

/* s3c6410 every two interrupt share the same control type */
typedef enum {
	EINT_CON0_1,
	EINT_CON2_3,
	EINT_CON4_5,
	EINT_CON6_7,
	EINT_CON8_9,
	EINT_CON10_11,
	EINT_CON12_13,
	EINT_CON14_15,
	EINT_CON16_17,
	EINT_CON18_19,
	EINT_CON20_21,
	EINT_CON22_23,
	EINT_CON24_25,
	EINT_CON26_27,
}eint_n;

/* filter type */
typedef enum {
	EINT_FILTER_DISABLE, /*filter disable */
	EINT_FILTER_DELAY,
	EINT_FILTER_DIGITAL, /* digital count:0~31 */
}eint_filter;

/* trigger type */
typedef enum {
	EINT_TRIGGER_LOW,
	EINT_TRIGGER_HIGH,
	EINT_TRIGGER_FALLING_EDGE,
	EINT_TRIGGER_RISING_EDGE = 0x04,
	EINT_TRIGGER_BOTH_EDGE = 0x06,
}eint_type;

void eint_set_type(eint_n n, eint_type type, eint_filter filter, \
		int digital_filter_count);

void eint_init(int eint0_3_prio,int eint4_11_prio, int eint12_19_prio, \
		int eint20_27_prio);
unsigned int eint_single_mask(int n);
unsigned int eint_single_unmask(int n);
void eint_set_handler(int n, void (*handler)(void));
/* pull_up_down:
 * 0: disable
 * 1: pull down
 * 2: pull up
 */
void eint_set_gpio(int n, int pull_up_down);

#endif
