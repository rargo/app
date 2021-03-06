#ifndef IRQ_H
#define IRQ_H

#define S3C64XX_IRQ_VIC0(n) (n)
#define S3C64XX_IRQ_VIC1(n) (n + 32)

/* VIC0 */
#define IRQ_EINT0_3		S3C64XX_IRQ_VIC0(0)
#define IRQ_EINT4_11		S3C64XX_IRQ_VIC0(1)
#define IRQ_RTC_TIC		S3C64XX_IRQ_VIC0(2)
#define IRQ_CAMIF_C		S3C64XX_IRQ_VIC0(3)
#define IRQ_CAMIF_P		S3C64XX_IRQ_VIC0(4)
#define IRQ_CAMIF_MC		S3C64XX_IRQ_VIC0(5)
#define IRQ_S3C6410_IIC1	S3C64XX_IRQ_VIC0(5)
#define IRQ_S3C6410_IIS		S3C64XX_IRQ_VIC0(6)
#define IRQ_S3C6400_CAMIF_MP	S3C64XX_IRQ_VIC0(6)
#define IRQ_CAMIF_WE_C		S3C64XX_IRQ_VIC0(7)
#define IRQ_S3C6410_G3D		S3C64XX_IRQ_VIC0(8)
#define IRQ_S3C6400_CAMIF_WE_P	S3C64XX_IRQ_VIC0(8)
#define IRQ_POST0		S3C64XX_IRQ_VIC0(9)
#define IRQ_ROTATOR		S3C64XX_IRQ_VIC0(10)
#define IRQ_2D			S3C64XX_IRQ_VIC0(11)
#define IRQ_TVENC		S3C64XX_IRQ_VIC0(12)
#define IRQ_SCALER		S3C64XX_IRQ_VIC0(13)
#define IRQ_BATF		S3C64XX_IRQ_VIC0(14)
#define IRQ_JPEG		S3C64XX_IRQ_VIC0(15)
#define IRQ_MFC			S3C64XX_IRQ_VIC0(16)
#define IRQ_SDMA0		S3C64XX_IRQ_VIC0(17)
#define IRQ_SDMA1		S3C64XX_IRQ_VIC0(18)
#define IRQ_ARM_DMAERR		S3C64XX_IRQ_VIC0(19)
#define IRQ_ARM_DMA		S3C64XX_IRQ_VIC0(20)
#define IRQ_ARM_DMAS		S3C64XX_IRQ_VIC0(21)
#define IRQ_KEYPAD		S3C64XX_IRQ_VIC0(22)
#define IRQ_TIMER0_VIC		S3C64XX_IRQ_VIC0(23)
#define IRQ_TIMER1_VIC		S3C64XX_IRQ_VIC0(24)
#define IRQ_TIMER2_VIC		S3C64XX_IRQ_VIC0(25)
#define IRQ_WDT			S3C64XX_IRQ_VIC0(26)
#define IRQ_TIMER3_VIC		S3C64XX_IRQ_VIC0(27)
#define IRQ_TIMER4_VIC		S3C64XX_IRQ_VIC0(28)
#define IRQ_LCD_FIFO		S3C64XX_IRQ_VIC0(29)
#define IRQ_LCD_VSYNC		S3C64XX_IRQ_VIC0(30)
#define IRQ_LCD_SYSTEM		S3C64XX_IRQ_VIC0(31)

/* VIC1 */
#define IRQ_EINT12_19		S3C64XX_IRQ_VIC1(0)
#define IRQ_EINT20_27		S3C64XX_IRQ_VIC1(1)
#define IRQ_PCM0		S3C64XX_IRQ_VIC1(2)
#define IRQ_PCM1		S3C64XX_IRQ_VIC1(3)
#define IRQ_AC97		S3C64XX_IRQ_VIC1(4)
#define IRQ_UART0		S3C64XX_IRQ_VIC1(5)
#define IRQ_UART1		S3C64XX_IRQ_VIC1(6)
#define IRQ_UART2		S3C64XX_IRQ_VIC1(7)
#define IRQ_UART3		S3C64XX_IRQ_VIC1(8)
#define IRQ_DMA0		S3C64XX_IRQ_VIC1(9)
#define IRQ_DMA1		S3C64XX_IRQ_VIC1(10)
#define IRQ_ONENAND0		S3C64XX_IRQ_VIC1(11)
#define IRQ_ONENAND1		S3C64XX_IRQ_VIC1(12)
#define IRQ_NFC			S3C64XX_IRQ_VIC1(13)
#define IRQ_CFCON		S3C64XX_IRQ_VIC1(14)
#define IRQ_USBH		S3C64XX_IRQ_VIC1(15)
#define IRQ_SPI0		S3C64XX_IRQ_VIC1(16)
#define IRQ_SPI1		S3C64XX_IRQ_VIC1(17)
#define IRQ_IIC			S3C64XX_IRQ_VIC1(18)
#define IRQ_HSItx		S3C64XX_IRQ_VIC1(19)
#define IRQ_HSIrx		S3C64XX_IRQ_VIC1(20)
#define IRQ_RESERVED		S3C64XX_IRQ_VIC1(21)
#define IRQ_MSM			S3C64XX_IRQ_VIC1(22)
#define IRQ_HOSTIF		S3C64XX_IRQ_VIC1(23)
#define IRQ_HSMMC0		S3C64XX_IRQ_VIC1(24)
#define IRQ_HSMMC1		S3C64XX_IRQ_VIC1(25)
#define IRQ_HSMMC2		IRQ_SPI1	/* shared with SPI1 */
#define IRQ_OTG			S3C64XX_IRQ_VIC1(26)
#define IRQ_IRDA		S3C64XX_IRQ_VIC1(27)
#define IRQ_RTC_ALARM		S3C64XX_IRQ_VIC1(28)
#define IRQ_SEC			S3C64XX_IRQ_VIC1(29)
#define IRQ_PENDN		S3C64XX_IRQ_VIC1(30)
#define IRQ_TC			IRQ_PENDN
#define IRQ_ADC			S3C64XX_IRQ_VIC1(31)

#define IRQ_MAX			S3C64XX_IRQ_VIC1(32)

void irq_c_handler(unsigned int irq);
void irq_init(void);
int irq_register_handler(unsigned int intr, unsigned int prio, void (*handler)(void));
void irq_triger_soft(unsigned int intr);
void irq_mask(unsigned int intr);
void irq_unmask(unsigned int intr);

#endif
