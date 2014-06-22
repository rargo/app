#include "dm9000x.h"
#include <base.h>
#include <arch/cc.h>
#include <ucos.h>
#include <lwip/tcpip.h>

/* board port */
#if 1
#define CONFIG_DM9000_BASE	0x18000300
#define DM9000_IO CONFIG_DM9000_BASE
#define DM9000_DATA (CONFIG_DM9000_BASE+4)
typedef struct bd_info {
	int			bi_baudrate;	/* serial console baudrate */
	unsigned long	bi_ip_addr;	/* IP Address */
	unsigned char	bi_enetaddr[6]; /* Ethernet adress */
} bd_t;
static bd_t gbd;
static bd_t *bd = &gbd;
#define __le16_to_cpu(x) ((u16_t)(x))
/**
 * is_zero_ether_addr - Determine if give Ethernet address is all zeros.
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is all zeroes.
 */
static inline int is_zero_ether_addr(const u8_t *addr)
{
	return !(addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]);
}

/**
 * is_multicast_ether_addr - Determine if the Ethernet address is a multicast.
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is a multicast address.
 * By definition the broadcast address is also a multicast address.
 */
static inline int is_multicast_ether_addr(const u8_t *addr)
{
	return (0x01 & addr[0]);
}

static void udelay(int n)
{
	volatile int i;
	while(n--)
	{
		for(i=0; i<100; i++)
			;
	}
}

static void mdelay(int n)
{
	while(n--)
	{
		udelay(1000);
	}
}

static unsigned int get_timer(void)
{
	return OSTimeGet() * SYS_TIMER_INTERVAL;
}

#define CFG_HZ SYS_TIMER_HZ

#define DM9000_Tacs     (0x0)   // 0clk         address set-up
#define DM9000_Tcos     (0x4)   // 4clk         chip selection set-up
#define DM9000_Tacc     (0xE)   // 14clk        access cycle
#define DM9000_Tcoh     (0x1)   // 1clk         chip selection hold
#define DM9000_Tah      (0x4)   // 4clk         address holding time
#define DM9000_Tacp     (0x6)   // 6clk         page mode access cycle
#define DM9000_PMC      (0x0)   // normal(1data)page mode configuration

#include <irq.h>
#include <eint.h>
#include <base.h>

void eth_isr(void);
static void dm9000_pre_init(void)
{
	SROM_BW_REG &= ~(0xf << 4);
	SROM_BW_REG |= (1<<7) | (1<<6) | (1<<4);
	SROM_BC1_REG = ((DM9000_Tacs<<28)+(DM9000_Tcos<<24)+(DM9000_Tacc<<16)+(DM9000_Tcoh<<12)+(DM9000_Tah<<8)+(DM9000_Tacp<<4)+(DM9000_PMC));

	eint_set_gpio(EINT(7), 0);
	eint_set_type(EINT_CON6_7, EINT_TRIGGER_RISING_EDGE, \
			EINT_FILTER_DELAY, 0);
	eint_set_handler(EINT(7), eth_isr);
	//eint_single_unmask(EINT(7));
}

static void dm9000_set_int(int enable)
{
	if(enable)
		eint_single_unmask(EINT(7));
	else
		eint_single_mask(EINT(7));
}
#endif

/* Board/System/Debug information/definition ---------------- */

//#define CONFIG_DM9000_DEBUG
#ifdef CONFIG_DM9000_DEBUG
#define DM9000_DMP_PACKET(func,packet,length)  \
	do { \
		int i; 							\
		RDIAG(ETH_DEBUG,func ": length: %d", length);			\
		for (i = 0; i < length; i++) {				\
			if (i % 8 == 0)					\
			RDIAG(ETH_DEBUG,"%s: %02x: ", func, i);	\
			RDIAG(ETH_DEBUG,"%02x ", ((unsigned char *) packet)[i]);	\
		} RDIAG(ETH_DEBUG,"");						\
	} while(0)
#else
#define DM9000_DMP_PACKET(func,packet,length)
#endif

/* Structure/enum declaration ------------------------------- */
typedef struct board_info {
	u32_t runt_length_counter;	/* counter: RX length < 64byte */
	u32_t long_length_counter;	/* counter: RX length > 1514byte */
	u32_t reset_counter;	/* counter: RESET */
	u32_t reset_tx_timeout;	/* RESET caused by TX Timeout */
	u32_t reset_rx_status;	/* RESET caused by RX Statsus wrong */
	u16_t tx_pkt_cnt;
	u16_t queue_start_addr;
	u16_t dbug_cnt;
	u8_t phy_addr;
	u8_t device_wait_reset;	/* device state */
	unsigned char srom[128];
	void (*outblk)(volatile void *data_ptr, int count);
	void (*inblk)(void *data_ptr, int count);
	void (*rx_status)(u16_t *RxStatus, u16_t *RxLen);
} board_info_t;
static board_info_t dm9000_info;

/* function declaration ------------------------------------- */
//int eth_init(bd_t * bd);
int eth_init(struct netif *netif);
int eth_send(volatile void *, int);
int eth_rx(void);
void eth_halt(void);
static int dm9000_probe(void);
static u16_t phy_read(int);
static void phy_write(int, u16_t);
u16_t read_srom_word(int);
static u8_t DM9000_ior(int);
static void DM9000_iow(int reg, u8_t value);
static int eth_send_packet(struct netif *netif, struct pbuf *p);

/* DM9000 network board routine ---------------------------- */

#define DM9000_outb(d,r) ( *(volatile u8_t *)r = d )
#define DM9000_outw(d,r) ( *(volatile u16_t *)r = d )
#define DM9000_outl(d,r) ( *(volatile u32_t *)r = d )
#define DM9000_inb(r) (*(volatile u8_t *)r)
#define DM9000_inw(r) (*(volatile u16_t *)r)
#define DM9000_inl(r) (*(volatile u32_t *)r)

	static void
dump_regs(void)
{
	RDIAG(ETH_DEBUG,"");
	RDIAG(ETH_DEBUG,"NCR   (0x00): %02x", DM9000_ior(0));
	RDIAG(ETH_DEBUG,"NSR   (0x01): %02x", DM9000_ior(1));
	RDIAG(ETH_DEBUG,"TCR   (0x02): %02x", DM9000_ior(2));
	RDIAG(ETH_DEBUG,"TSRI  (0x03): %02x", DM9000_ior(3));
	RDIAG(ETH_DEBUG,"TSRII (0x04): %02x", DM9000_ior(4));
	RDIAG(ETH_DEBUG,"RCR   (0x05): %02x", DM9000_ior(5));
	RDIAG(ETH_DEBUG,"RSR   (0x06): %02x", DM9000_ior(6));
	RDIAG(ETH_DEBUG,"ISR   (0xFE): %02x", DM9000_ior(DM9000_ISR));
	RDIAG(ETH_DEBUG,"");
}

static void dm9000_outblk_8bit(volatile void *data_ptr, int count)
{
	int i;
	for (i = 0; i < count; i++)
		DM9000_outb((((u8_t *) data_ptr)[i] & 0xff), DM9000_DATA);
}

static void dm9000_outblk_16bit(volatile void *data_ptr, int count)
{
	int i;
	u32_t tmplen = (count + 1) / 2;

	for (i = 0; i < tmplen; i++)
		DM9000_outw(((u16_t *) data_ptr)[i], DM9000_DATA);
}
static void dm9000_outblk_32bit(volatile void *data_ptr, int count)
{
	int i;
	u32_t tmplen = (count + 3) / 4;

	for (i = 0; i < tmplen; i++)
		DM9000_outl(((u32_t *) data_ptr)[i], DM9000_DATA);
}

static void dm9000_inblk_8bit(void *data_ptr, int count)
{
	int i;
	for (i = 0; i < count; i++)
		((u8_t *) data_ptr)[i] = DM9000_inb(DM9000_DATA);
}

static void dm9000_inblk_16bit(void *data_ptr, int count)
{
	int i;
	u32_t tmplen = (count + 1) / 2;

	for (i = 0; i < tmplen; i++)
		((u16_t *) data_ptr)[i] = DM9000_inw(DM9000_DATA);
}
static void dm9000_inblk_32bit(void *data_ptr, int count)
{
	int i;
	u32_t tmplen = (count + 3) / 4;

	for (i = 0; i < tmplen; i++)
		((u32_t *) data_ptr)[i] = DM9000_inl(DM9000_DATA);
}

static void dm9000_rx_status_32bit(u16_t *RxStatus, u16_t *RxLen)
{
	u32_t tmpdata;

	DM9000_outb(DM9000_MRCMD, DM9000_IO);

	tmpdata = DM9000_inl(DM9000_DATA);
	*RxStatus = __le16_to_cpu(tmpdata);
	*RxLen = __le16_to_cpu(tmpdata >> 16);
}

static void dm9000_rx_status_16bit(u16_t *RxStatus, u16_t *RxLen)
{
	DM9000_outb(DM9000_MRCMD, DM9000_IO);

	*RxStatus = __le16_to_cpu(DM9000_inw(DM9000_DATA));
	*RxLen = __le16_to_cpu(DM9000_inw(DM9000_DATA));
}

static void dm9000_rx_status_8bit(u16_t *RxStatus, u16_t *RxLen)
{
	DM9000_outb(DM9000_MRCMD, DM9000_IO);

	*RxStatus =
		__le16_to_cpu(DM9000_inb(DM9000_DATA) +
				(DM9000_inb(DM9000_DATA) << 8));
	*RxLen =
		__le16_to_cpu(DM9000_inb(DM9000_DATA) +
				(DM9000_inb(DM9000_DATA) << 8));
}

/*
   Search DM9000 board, allocate space and register it
   */
	int
dm9000_probe(void)
{
	u32_t id_val;
	id_val = DM9000_ior(DM9000_VIDL);
	id_val |= DM9000_ior(DM9000_VIDH) << 8;
	id_val |= DM9000_ior(DM9000_PIDL) << 16;
	id_val |= DM9000_ior(DM9000_PIDH) << 24;
	if (id_val == DM9000_ID) {
		RDIAG(ETH_DEBUG,"dm9000 i/o: 0x%x, id: 0x%x ", CONFIG_DM9000_BASE,
				id_val);
		return 0;
	} else {
		RDIAG(ETH_DEBUG,"dm9000 not found at 0x%08x id: 0x%08x",
				CONFIG_DM9000_BASE, id_val);
		return -1;
	}
}

/* General Purpose dm9000 reset routine */
	static void
dm9000_reset(void)
{
	RDIAG(ETH_DEBUG,"resetting DM9000");

	/* Reset DM9000,
	   see DM9000 Application Notes V1.22 Jun 11, 2004 page 29 */

	/* DEBUG: Make all GPIO0 outputs, all others inputs */
	DM9000_iow(DM9000_GPCR, GPCR_GPIO0_OUT);
	/* Step 1: Power internal PHY by writing 0 to GPIO0 pin */
	DM9000_iow(DM9000_GPR, 0);
	/* Step 2: Software reset */
	DM9000_iow(DM9000_NCR, (NCR_LBK_INT_MAC | NCR_RST));

	do {
		RDIAG(ETH_DEBUG,"resetting the DM9000, 1st reset");
		udelay(25); /* Wait at least 20 us */
	} while (DM9000_ior(DM9000_NCR) & 1);

	DM9000_iow(DM9000_NCR, 0);
	DM9000_iow(DM9000_NCR, (NCR_LBK_INT_MAC | NCR_RST)); /* Issue a second reset */

	do {
		RDIAG(ETH_DEBUG,"resetting the DM9000, 2nd reset");
		udelay(25); /* Wait at least 20 us */
	} while (DM9000_ior(DM9000_NCR) & 1);

	/* Check whether the ethernet controller is present */
	if ((DM9000_ior(DM9000_PIDL) != 0x0) ||
			(DM9000_ior(DM9000_PIDH) != 0x90))
		RDIAG(ETH_DEBUG,"ERROR: resetting DM9000 -> not responding");
}

/* Initilize dm9000 MAC, without reset
*/
	int
eth_set_mac(bd_t * bd)
{
	int i,oft;
	RDIAG(ETH_DEBUG,"MAC: %02x:%02x:%02x:%02x:%02x:%02x", bd->bi_enetaddr[0],
			bd->bi_enetaddr[1], bd->bi_enetaddr[2], bd->bi_enetaddr[3],
			bd->bi_enetaddr[4], bd->bi_enetaddr[5]);

	/* fill device MAC address registers */
	for (i = 0, oft = DM9000_PAR; i < 6; i++, oft++)
		DM9000_iow(oft, bd->bi_enetaddr[i]);
	for (i = 0, oft = 0x16; i < 8; i++, oft++)
		DM9000_iow(oft, 0xff);

	/* read back mac, just to be sure */
	for (i = 0, oft = 0x10; i < 6; i++, oft++)
		RDIAG(ETH_DEBUG,"%02x:", DM9000_ior(oft));
	RDIAG(ETH_DEBUG,"");
	return 0;
}


/* Initilize dm9000 board
*/
//int eth_init(bd_t * bd)
int eth_init(struct netif *netif)
{
	int i, lnk;
	u8_t io_mode;
	struct board_info *db = &dm9000_info;

	RDIAG(ETH_DEBUG,"eth_init()");

	dm9000_pre_init();

	/* RESET device */
	dm9000_reset();

	if (dm9000_probe() < 0)
		return -1;

	/* Auto-detect 8/16/32 bit mode, ISR Bit 6+7 indicate bus width */
	io_mode = DM9000_ior(DM9000_ISR) >> 6;

	switch (io_mode) {
		case 0x0:  /* 16-bit mode */
			RDIAG(ETH_DEBUG,"DM9000: running in 16 bit mode");
			db->outblk    = dm9000_outblk_16bit;
			db->inblk     = dm9000_inblk_16bit;
			db->rx_status = dm9000_rx_status_16bit;
			break;
		case 0x01:  /* 32-bit mode */
			RDIAG(ETH_DEBUG,"DM9000: running in 32 bit mode");
			db->outblk    = dm9000_outblk_32bit;
			db->inblk     = dm9000_inblk_32bit;
			db->rx_status = dm9000_rx_status_32bit;
			break;
		case 0x02: /* 8 bit mode */
			RDIAG(ETH_DEBUG,"DM9000: running in 8 bit mode");
			db->outblk    = dm9000_outblk_8bit;
			db->inblk     = dm9000_inblk_8bit;
			db->rx_status = dm9000_rx_status_8bit;
			break;
		default:
			/* Assume 8 bit mode, will probably not work anyway */
			RDIAG(ETH_DEBUG,"DM9000: Undefined IO-mode:0x%x", io_mode);
			db->outblk    = dm9000_outblk_8bit;
			db->inblk     = dm9000_inblk_8bit;
			db->rx_status = dm9000_rx_status_8bit;
			break;
	}

	/* Program operating register, only internal phy supported */
	DM9000_iow(DM9000_NCR, 0x0);
	/* TX Polling clear. PAD, CRC enable for packet index I,II */
	DM9000_iow(DM9000_TCR, 0);
	/* Less 3Kb, 200us */
	DM9000_iow(DM9000_BPTR, BPTR_BPHW(3) | BPTR_JPT_600US);
	/* Flow Control : High/Low Water */
	DM9000_iow(DM9000_FCTR, FCTR_HWOT(3) | FCTR_LWOT(8));
	/* SH FIXME: This looks strange! Flow Control */
	DM9000_iow(DM9000_FCR, 0x0);
	/* Special Mode */
	DM9000_iow(DM9000_SMCR, 0);
	/* clear TX status */
	DM9000_iow(DM9000_NSR, NSR_WAKEST | NSR_TX2END | NSR_TX1END);
	/* Clear interrupt status */
	DM9000_iow(DM9000_ISR, ISR_ROOS | ISR_ROS | ISR_PTS | ISR_PRS);

	/* Set Node address */
#if 0
#if !defined(CONFIG_AT91SAM9261EK) && !defined(CONFIG_DRIVER_DM9000_NO_EEPROM)
	for (i = 0; i < 6; i++)
		((u16_t *) bd->bi_enetaddr)[i] = read_srom_word(i);
#endif
#endif

	bd->bi_enetaddr[0] = netif->hwaddr[0];
	bd->bi_enetaddr[1] = netif->hwaddr[1]; 
	bd->bi_enetaddr[2] = netif->hwaddr[2]; 
	bd->bi_enetaddr[3] = netif->hwaddr[3];
	bd->bi_enetaddr[4] = netif->hwaddr[4];
	bd->bi_enetaddr[5] = netif->hwaddr[5];
	eth_set_mac(bd);

	/* Activate DM9000 */
	/* RX enable */
	DM9000_iow(DM9000_RCR, RCR_DIS_LONG | RCR_DIS_CRC | RCR_RXEN);
	/* Enable TX/RX interrupt mask */
	DM9000_iow(DM9000_IMR, IMR_PAR);

	i = 0;
	while (!(phy_read(1) & 0x20)) {	/* autonegation complete bit */
		udelay(1000);
		i++;
		if (i == 10000) {
			RDIAG(ETH_DEBUG,"could not establish link");
			return 0;
		}
	}

	/* see what we've got */
	lnk = phy_read(17) >> 12;
	RDIAG(ETH_DEBUG,"operating at ");
	switch (lnk) {
		case 1:
			RDIAG(ETH_DEBUG,"10M half duplex ");
			break;
		case 2:
			RDIAG(ETH_DEBUG,"10M full duplex ");
			break;
		case 4:
			RDIAG(ETH_DEBUG,"100M half duplex ");
			break;
		case 8:
			RDIAG(ETH_DEBUG,"100M full duplex ");
			break;
		default:
			RDIAG(ETH_DEBUG,"unknown: %d ", lnk);
			break;
	}
	RDIAG(ETH_DEBUG,"mode");

	/* clear interrupt status */
	DM9000_iow(DM9000_ISR, ISR_ROOS | ISR_ROS | ISR_PTS | ISR_PRS);
	/* XXX enable receive interrupt */
	//DM9000_iow(DM9000_IMR, 0x81);
	DM9000_iow(DM9000_IMR, IMR_PAR | IMR_ROM | IMR_PTM | IMR_PRM | IMR_LNKCHGI);
	/* enable dm9000 interrupt */
	dm9000_set_int(1);

	return 0;
}

/*
   Hardware start transmission.
   Send a packet to media from the upper layer.
   */
	int
eth_send(volatile void *packet, int length)
{
	int tmo;
	struct board_info *db = &dm9000_info;

	//DM9000_DMP_PACKET("eth_send", packet, length);

	/* XXX wait last packet transmit complete,add time-out? */
	if(DM9000_ior(DM9000_TCR) & 0x01) {
		RERR("LAST PACKET NOT SEND FINISH");
	}

	/* Move data to DM9000 TX RAM */
	DM9000_outb(DM9000_MWCMD, DM9000_IO); /* Prepare for TX-data */

	/* push the data to the TX-fifo */
	(db->outblk)(packet, length);

	/* Set TX length to DM9000 */
	DM9000_iow(DM9000_TXPLL, length & 0xff);
	DM9000_iow(DM9000_TXPLH, (length >> 8) & 0xff);

	/* Issue TX polling command */
	DM9000_iow(DM9000_TCR, TCR_TXREQ); /* Cleared after TX complete */

	RDIAG(ETH_DEBUG,"transmit done");
	return 0;
}

/*
   Stop the interface.
   The interface is stopped when it is brought.
   */
	void
eth_halt(void)
{
	RDIAG(ETH_DEBUG,"eth_halt");

	/* RESET devie */
	phy_write(0, 0x8000);	/* PHY RESET */
	DM9000_iow(DM9000_GPR, 0x01);	/* Power-Down PHY */
	DM9000_iow(DM9000_IMR, 0x80);	/* Disable all interrupt */
	DM9000_iow(DM9000_RCR, 0x00);	/* Disable RX */
}

/*
   Read a word data from SROM
   */
	u16_t
read_srom_word(int offset)
{
	DM9000_iow(DM9000_EPAR, offset);
	DM9000_iow(DM9000_EPCR, 0x4);
	udelay(8000);
	DM9000_iow(DM9000_EPCR, 0x0);
	return (DM9000_ior(DM9000_EPDRL) + (DM9000_ior(DM9000_EPDRH) << 8));
}

	void
write_srom_word(int offset, u16_t val)
{
	DM9000_iow(DM9000_EPAR, offset);
	DM9000_iow(DM9000_EPDRH, ((val >> 8) & 0xff));
	DM9000_iow(DM9000_EPDRL, (val & 0xff));
	DM9000_iow(DM9000_EPCR, 0x12);
	udelay(8000);
	DM9000_iow(DM9000_EPCR, 0);
}


/*
   Read a byte from I/O port
   */
	static u8_t
DM9000_ior(int reg)
{
	DM9000_outb(reg, DM9000_IO);
	return DM9000_inb(DM9000_DATA);
}

/*
   Write a byte to I/O port
   */
	static void
DM9000_iow(int reg, u8_t value)
{
	DM9000_outb(reg, DM9000_IO);
	DM9000_outb(value, DM9000_DATA);
}

/*
   Read a word from phyxcer
   */
	static u16_t
phy_read(int reg)
{
	u16_t val;

	/* Fill the phyxcer register into REG_0C */
	DM9000_iow(DM9000_EPAR, DM9000_PHY | reg);
	DM9000_iow(DM9000_EPCR, 0xc);	/* Issue phyxcer read command */
	udelay(100);			/* Wait read complete */
	DM9000_iow(DM9000_EPCR, 0x0);	/* Clear phyxcer read command */
	val = (DM9000_ior(DM9000_EPDRH) << 8) | DM9000_ior(DM9000_EPDRL);

	/* The read data keeps on REG_0D & REG_0E */
	RDIAG(ETH_DEBUG,"phy_read(0x%x): 0x%x", reg, val);
	return val;
}

/*
   Write a word to phyxcer
   */
	static void
phy_write(int reg, u16_t value)
{

	/* Fill the phyxcer register into REG_0C */
	DM9000_iow(DM9000_EPAR, DM9000_PHY | reg);

	/* Fill the written data into REG_0D & REG_0E */
	DM9000_iow(DM9000_EPDRL, (value & 0xff));
	DM9000_iow(DM9000_EPDRH, ((value >> 8) & 0xff));
	DM9000_iow(DM9000_EPCR, 0xa);	/* Issue phyxcer write command */
	udelay(500);			/* Wait write complete */
	DM9000_iow(DM9000_EPCR, 0x0);	/* Clear phyxcer write command */
	RDIAG(ETH_DEBUG,"phy_write(reg:0x%x, value:0x%x)", reg, value);
}

#if 0
static const char ieee_address_string[] = "08002700c422";
static const unsigned char eth_address[] = {0x08,0x00,0x27,0x00,0xc4,0x22};
static const unsigned char ip_address[] = {192,168,1,25};
static const unsigned char dest_ip_address[] = {192,168,1,26};

typedef struct eth_header {
	unsigned char dest[6];
	unsigned char src[6];
	unsigned short type;
	unsigned char data[0];
} __attribute__((packed)) eth_header_t,*p_eth_header_t ;

typedef struct arp {
	unsigned short hardware_type;
	unsigned short protocol_type;
	unsigned char hardware_length;
	unsigned char protocol_length;
	unsigned short op;
	unsigned char src_eth_addr[6];
	unsigned char src_ip_addr[4];
	unsigned char dest_eth_addr[6];
	unsigned char dest_ip_addr[4];
}__attribute__((packed)) arp_t,*p_arp_t ;

static unsigned short arp_packet[60];//packet must at least 60 bytes

/*reverse memory bytes*/
static void mem_r(unsigned char *p_out,const unsigned char *p_in,unsigned int count)
{
	p_in += count - 1;
	while(count--)
	{
		*p_out++ = *p_in--;
	}
}

/* reverse(change) memory bytes,p is input and output */
static void mem_rc(unsigned char *p,unsigned int count)
{
	unsigned char temp;
	unsigned char *p_end = p + count -1;
	count /= 2;
	while(count--)
	{
		temp = *p;
		*p = *p_end;
		*p_end = temp;

		p++;
		p_end--;
	}
}

/* reverse unsigned short */
static unsigned short u16_r(unsigned short data)
{
	return (data >> 8 | data << 8);
}

/* reverse unsigned int */
static unsigned int u32_r(unsigned int data)
{
	return ((unsigned int)u16_r(data) << 16) | u16_r(data >> 16); 
}

void transmit_arp_test(void)
{
	p_eth_header_t p_eth;
	p_arp_t p_arp;

	p_eth = (p_eth_header_t)(arp_packet);
	memcpy(p_eth->dest,(unsigned char *)"\xff\xff\xff\xff\xff\xff",6);
	memcpy(p_eth->src,eth_address,6);
	p_eth->type = u16_r(0x0806);

	p_arp = (p_arp_t)(p_eth->data);
	p_arp->hardware_type = u16_r(0x0001);
	p_arp->protocol_type = u16_r(0x0800);
	p_arp->hardware_length = 6;
	p_arp->protocol_length = 4;
	p_arp->op = u16_r(0x0001);
	memcpy(p_arp->src_eth_addr,eth_address,6);
	memcpy(p_arp->src_ip_addr,ip_address,4);
	memcpy(p_arp->dest_ip_addr,dest_ip_address,4);

	for(; ;) {
		eth_send(arp_packet, sizeof(arp_t) + sizeof(eth_header_t));
		mdelay(100);
	}
}
#endif

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include "netif/etharp.h"
#include "netif/ppp_oe.h"

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
struct ethernetif {
	struct eth_addr *ethaddr;
	/* Add whatever per-interface state that is needed here. */
};

/* Forward declarations. */
static void  ethernetif_input(struct netif *netif);

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
	static void
low_level_init(struct netif *netif)
{
	struct ethernetif *ethernetif = netif->state;

	/* set MAC hardware address length */
	netif->hwaddr_len = ETHARP_HWADDR_LEN;

	/* set MAC hardware address */
	netif->hwaddr[0] = 0x08;
	netif->hwaddr[1] = 0x90;
	netif->hwaddr[2] = 0x90;
	netif->hwaddr[3] = 0x90;
	netif->hwaddr[4] = 0x90;
	netif->hwaddr[5] = 0x90;

	/* maximum transfer unit */
	netif->mtu = 1500;

	/* device capabilities */
	/* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	/* Do whatever else is needed to initialize interface. */  
	eth_init(netif);
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

	static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
	struct ethernetif *ethernetif = netif->state;
	struct pbuf *q;

	//initiate transfer();

#if ETH_PAD_SIZE
	pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

#if 1
	eth_send_packet(netif,p);
#else
	for(q = p; q != NULL; q = q->next) {
		/* Send the data from the pbuf to the interface, one pbuf at a
		   time. The size of the data in each pbuf is kept in the ->len
		   variable. */
		//send data from(q->payload, q->len);
		RDIAG(ETH_DEBUG,"send payload,len:%d",q->len);
		eth_send(q->payload, q->len);//XXX add
	}
#endif

	//signal that packet should be sent();

#if ETH_PAD_SIZE
	pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

	LINK_STATS_INC(link.xmit);

	return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
	static struct pbuf *
low_level_input(struct netif *netif)
{
	struct ethernetif *ethernetif = netif->state;
	struct pbuf *p, *q;
	static unsigned int drop_packet[1600/4];
	u16_t len;

	u8_t rxbyte;
	u16_t RxStatus, RxLen = 0;
	struct board_info *db = &dm9000_info;

	//RDIAG(ETH_DEBUG);

#if 0
	/* Check packet ready or not, we must check
	   the ISR status first for DM9000A */
	if (!(DM9000_ior(DM9000_ISR) & 0x01)) {/* Rx-ISR bit must be set. */
		RERR("Rx-ISR not set");
		return 0;
	}
#endif

	/* There is _at least_ 1 package in the fifo, read them all */
	DM9000_ior(DM9000_MRCMDX);	/* Dummy read */

	/* Get most updated data,
	   only look at bits 0:1, See application notes DM9000 */
	rxbyte = DM9000_inb(DM9000_DATA) & 0x03;

	/* Status check: this byte must be 0 or 1 */
	if (rxbyte > DM9000_PKT_RDY) {
		//DM9000_iow(DM9000_RCR, 0x00);	/* Stop Device */
		//DM9000_iow(DM9000_ISR, 0x80);	/* Stop INT request */
		RERR("DM9000 error: status check fail: 0x%x!!!", rxbyte);
		return NULL;
	}

	/* No packet received, ignore */
	if (rxbyte != DM9000_PKT_RDY) {
		RDIAG(ETH_DEBUG);
		return NULL;
	}

	/* A packet ready now  & Get status/length */
	(db->rx_status)(&RxStatus, &RxLen);
	//RDIAG(ETH_DEBUG,"received packet len:%d",RxLen);
	len = RxLen;

	/* if any errors, drop packet */
	if ((RxStatus & 0xbf00) || (RxLen < 0x40) || (RxLen > DM9000_PKT_MAX)) {

		(db->inblk)(drop_packet, RxLen);

		if (RxStatus & 0x100)
			RERR("rx fifo error");
		if (RxStatus & 0x200)
			RERR("rx crc error");
		if (RxStatus & 0x400)
			RERR("rx alignment error");
		if (RxStatus & 0x800)
			RERR("rx physical layer error");
		if (RxStatus & 0x1000)
			RERR("rx receive watchdog time-out");
		if (RxStatus & 0x2000)
			RERR("rx late collision seen");
#if 0
		if (RxStatus & 0x4000)
			RDIAG(ETH_DEBUG,"rx muticast frame");
#endif
		if (RxStatus & 0x8000)
			RERR("rx length error");
		if (RxLen > DM9000_PKT_MAX) {
			RERR("rx length too big");
			dm9000_reset();
		}
		return NULL;
	}

#if 0
	/* XXX drop multicast frame */
	if (RxStatus & 0x4000) {
		RDIAG(ETH_DEBUG,"DROP RX MUTICAST FRAME");
		(db->inblk)(drop_packet, RxLen);
		return NULL;
	}
#endif

	//DM9000_DMP_PACKET("eth_rx", rdptr, RxLen);

#if ETH_PAD_SIZE
	len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

	/* We allocate a pbuf chain of pbufs from the pool. */
	p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
	/* if can't alloc pbuf, drop packet */
	if (p == NULL) {
		(db->inblk)(drop_packet, RxLen);
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		return NULL;
	}

#if ETH_PAD_SIZE
	pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

	/* We iterate over the pbuf chain until we have read the entire
	 * packet into the pbuf. */
	for(q = p; q != NULL; q = q->next) {
		/* Read enough bytes to fill this pbuf in the chain. The
		 * available data in the pbuf is given by the q->len
		 * variable.
		 * This does not necessarily have to be a memcpy, you can also preallocate
		 * pbufs for a DMA-enabled MAC and after receiving truncate it to the
		 * actually received size. In this case, ensure the tot_len member of the
		 * pbuf is the sum of the chained pbuf len members.
		 */
		//read data into(q->payload, q->len);
		//eth_rx(q->payload, q->len);
		if(RxLen > q->len)
			(db->inblk)(q->payload, q->len);
		else {
			(db->inblk)(q->payload, q->len);
			q->len = RxLen;
			break;
		}
		RxLen -= q->len;
	}

	// acknowledge that packet has been read();
#if ETH_PAD_SIZE
	pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

	LINK_STATS_INC(link.recv);
	RDIAG(ETH_DEBUG,"passing packet to upper layer");

	return p;  
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
	static void
ethernetif_input(struct netif *netif)
{
	struct ethernetif *ethernetif;
	struct eth_hdr *ethhdr;
	struct pbuf *p;

	ethernetif = netif->state;

	//RDIAG(ETH_DEBUG);
	for(;;) {
		/* move received packet into a new pbuf */
		p = low_level_input(netif);
		/* XXX error happen,or no packet could be read */
		/* no packet could be read, silently ignore this */
		if (p == NULL) {
			//RDIAG(ETH_DEBUG);
			return;
		}
		/* points to packet payload, which starts with an Ethernet header */
		ethhdr = p->payload;

		switch (htons(ethhdr->type)) {
			/* IP or ARP packet? */
			case ETHTYPE_IP:
			case ETHTYPE_ARP:
#if PPPOE_SUPPORT
				/* PPPoE packet? */
			case ETHTYPE_PPPOEDISC:
			case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
				/* full packet send to tcpip_thread to process */
				/*input  : tcpip_input() or ip_input()
				 * tcpip_input(): post message to tcpip_thread() to be handle.(async mode)
				 * ip_input(): directly handle message.(sync mode) 
				 */
				if (netif->input(p, netif)!=ERR_OK)  { 
					LWIP_DEBUGF(NETIF_DEBUG, \
							("ethernetif_input: IP input error"));
					pbuf_free(p);
					p = NULL;
				}
				break;

			default:
				pbuf_free(p);
				p = NULL;
				break;
		}
	}
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
	err_t
ethernetif_init(struct netif *netif)
{
	struct ethernetif *ethernetif;

	LWIP_ASSERT("netif != NULL", (netif != NULL));

	ethernetif = mem_malloc(sizeof(struct ethernetif));
	if (ethernetif == NULL) {
		LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory"));
		dprintf("ethernetif_init: out of memory");
		return ERR_MEM;
	}

#if LWIP_NETIF_HOSTNAME
	/* Initialize interface hostname */
	netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

	/*
	 * Initialize the snmp variables and counters inside the struct netif.
	 * The last argument should be replaced with your link speed, in units
	 * of bits per second.
	 */
	NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

	netif->state = ethernetif;
	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;
	/* We directly use etharp_output() here to save a function call.
	 * You can instead declare your own function an call etharp_output()
	 * from it if you have to do some checks before sending (e.g. if link
	 * is available...) */
	netif->output = etharp_output;
	netif->linkoutput = low_level_output;

	ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

	/* initialize the hardware */
	low_level_init(netif);

	return ERR_OK;
}

#define ETH_SEND_PACKET_QSIZE 5
#define ETH_PACKET_SIZE 1500
struct eth_send_buffer {
	unsigned char buf[ETH_PACKET_SIZE];
	int length;
	int using;
};
struct eth_send_buffer _eth_send_bufer[ETH_SEND_PACKET_QSIZE];
//return 1 on transmit is going.
static int eth_tx_going(void)
{
	return DM9000_ior(DM9000_TCR) & 0x01;
}
/* send packet in the queue */
static int eth_send_packet_queue(void)
{
	int i;
	for(i=0;i<ETH_SEND_PACKET_QSIZE; i++) {
		if(_eth_send_bufer[i].using != 0) {
			eth_send(_eth_send_bufer[i].buf,_eth_send_bufer[i].length);
			_eth_send_bufer[i].using = 0;
			break;
		}
	}

	return 0;
}

/* send packet:
 * if transmit queue is empty and no current transmit is going:
 *   place packet into dm9000, initial a transmit.
 * else: 
 *   place the packet into queue, will be transmit when transmit 
 * finish interrupt occur
 */
static int eth_send_packet(struct netif *netif, struct pbuf *p)
{
	int i;
	int size;
	struct eth_msg *msg;
	struct pbuf *q;
	int queue_not_used;
	int first_empty;
	int ret;

	/* disable eth interrupt */
	dm9000_set_int(0);
	first_empty = -1;
	queue_not_used = 0;
	for(i=0;i<ETH_SEND_PACKET_QSIZE; i++) {
		if(_eth_send_bufer[i].using == 0) {
			queue_not_used++;
			if(first_empty == -1)
				first_empty = i;
		}
	}

	if(queue_not_used == 0) {
		RERR("send packet holding room empty!!!");
		ret = -ENOMEM;
		goto OUT;
	}

	/* get the buffer */
	size = 0;
	for(q = p; q != NULL; q = q->next) {
		//RDIAG(ETH_DEBUG,"send payload,len:%d",q->len);
		if(size + q->len > ETH_PACKET_SIZE) {
			RERR("packet bad, too large");
			ret = -EIO;
			goto OUT;
		}
		memcpy(_eth_send_bufer[first_empty].buf + size, q->payload, q->len);
		size += q->len;
	}
	_eth_send_bufer[first_empty].length = size;

	ret = 0;
	if(queue_not_used == ETH_SEND_PACKET_QSIZE && !eth_tx_going()) {
		/* if currently not any transmit going, send directly */
		eth_send(_eth_send_bufer[first_empty].buf,_eth_send_bufer[first_empty].length);
		RDIAG(ETH_DEBUG,"eth packet send");
		goto OUT;
	} else {
		/* place it into queue */
		_eth_send_bufer[first_empty].using = 1;
		RDIAG(ETH_DEBUG,"eth packet queue");
	}

OUT:
	dm9000_set_int(1);
	return ret;
}

static int eth_softirq_nr;
static unsigned char eth_isr_status;
static unsigned char eth_isr_mask;
static struct netif eth_netif;
void eth_isr(void)
{
	eth_isr_status = DM9000_ior(DM9000_ISR) & 0x3f;
	eth_isr_mask = DM9000_ior(DM9000_IMR);
	DM9000_iow(DM9000_IMR, 0x80);
	DM9000_iow(DM9000_ISR, eth_isr_status); /* ack */

#if 0
	/* I am not sure, in linux, eth rx,tx is done in irq */
	softirq_raise(eth_softirq_nr);
}

void eth_softirq(void)
{
#endif

	//RDIAG(ETH_DEBUG,"DM9000_ISR:0x%8x",eth_isr_status);
	if(eth_isr_status & ISR_LSC)
		RDIAG(ETH_DEBUG,"link status change");

	if(eth_isr_status & ISR_TUR)
		RDIAG(ETH_DEBUG,"transmit under run");

	if(eth_isr_status & ISR_ROOS)
		RDIAG(ETH_DEBUG,"receive overflow counter overflow");

	if(eth_isr_status & ISR_ROS)
		RDIAG(ETH_DEBUG,"receive overflow");

	if(eth_isr_status & ISR_PRS) {
		RDIAG(ETH_DEBUG,"ISR_PRS flag set");
		ethernetif_input(&eth_netif);
	}

	if(eth_isr_status & ISR_PTS) {
		RDIAG(ETH_DEBUG,"packet transmited");
		eth_send_packet_queue();
	}

	//RDIAG(ETH_DEBUG,"eth_isr_mask:0x%8x",eth_isr_mask);
	DM9000_iow(DM9000_IMR, eth_isr_mask);
}

//void eth_dm9000_init(void)
static ip_addr_t eth_ipaddr, eth_netmask, eth_gw;
void lwip_dm9000_init(void)
{
	IP4_ADDR(&eth_gw, 192,168,1,1);
	IP4_ADDR(&eth_ipaddr, 192,168,1,120);
	IP4_ADDR(&eth_netmask, 255,255,255,0);

#if 0
	struct netif *
		netif_add(struct netif *netif, ip_addr_t *ipaddr, ip_addr_t *netmask,
				ip_addr_t *gw, void *state, netif_init_fn init, netif_input_fn input)
#endif

#if 0
	eth_softirq_nr = softirq_require(eth_softirq);
	if(eth_softirq_nr < 0)
		RERR("eth softirq invalid");
#endif

	if(NULL == netif_add(&eth_netif, &eth_ipaddr, &eth_netmask, \
				&eth_gw, NULL, ethernetif_init, tcpip_input))
		RERR("netif_add error");

	netif_set_default(&eth_netif);
	netif_set_up(&eth_netif);
}

