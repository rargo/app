#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define MEM_LIBC_MALLOC 0
#define MEMP_MEM_MALLOC 0
#define MEMP_SEPARATE_POOLS             1
#define MEM_USE_POOLS                   1
#define MEMP_USE_CUSTOM_POOLS           1

/* TODO confirm this */
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 1
#define SYS_LIGHTWEIGHT_PROT 1


/* sys_sleep_ms() is in sys_arch.c, it offer 10ms resolution 
 * it may be a problem.
 */
#define sys_msleep sys_sleep_ms

/* XXX enable all debug mesg */

#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL

/**
 * TCPIP_THREAD_STACKSIZE: The stack size used by the main tcpip thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define TCPIP_THREAD_STACKSIZE         256*1024 

/**
 * TCPIP_THREAD_PRIO: The priority assigned to the main tcpip thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define TCPIP_THREAD_PRIO               8

/**
 * TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called.
 */
#define TCPIP_MBOX_SIZE                 32


/**
 * SLIP_THREAD_STACKSIZE: The stack size used by the slipif_loop thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define SLIPIF_THREAD_STACKSIZE         64*1024

/**
 * SLIPIF_THREAD_PRIO: The priority assigned to the slipif_loop thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define SLIPIF_THREAD_PRIO              6

/**
 * PPP_THREAD_STACKSIZE: The stack size used by the pppInputThread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define PPP_THREAD_STACKSIZE            64*1024

/**
 * PPP_THREAD_PRIO: The priority assigned to the pppInputThread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define PPP_THREAD_PRIO                 7


/**
 * DEFAULT_THREAD_STACKSIZE: The stack size used by any other lwIP thread.
 * The stack size value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define DEFAULT_THREAD_STACKSIZE        64*1024

/**
 * DEFAULT_THREAD_PRIO: The priority assigned to any other lwIP thread.
 * The priority value itself is platform-dependent, but is passed to
 * sys_thread_new() when the thread is created.
 */
#define DEFAULT_THREAD_PRIO             10

/**
 * DEFAULT_RAW_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_RAW. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#define DEFAULT_RAW_RECVMBOX_SIZE       32

/**
 * DEFAULT_UDP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_UDP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#define DEFAULT_UDP_RECVMBOX_SIZE       32

/**
 * DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#define DEFAULT_TCP_RECVMBOX_SIZE       32

/**
 * DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when the acceptmbox is created.
 */
#define DEFAULT_ACCEPTMBOX_SIZE         32


/**
 * LWIP_DBG_TYPES_ON: A mask that can be used to globally enable/disable
 * debug messages of certain types.
 */
#define LWIP_DBG_TYPES_ON               LWIP_DBG_ON


/**
 * ETHARP_DEBUG: Enable debugging in etharp.c.
 */
#define ETHARP_DEBUG                    LWIP_DBG_OFF


/**
 * NETIF_DEBUG: Enable debugging in netif.c.
 */
#define NETIF_DEBUG                     LWIP_DBG_OFF


/**
 * PBUF_DEBUG: Enable debugging in pbuf.c.
 */
#define PBUF_DEBUG                      LWIP_DBG_OFF


/**
 * API_LIB_DEBUG: Enable debugging in api_lib.c.
 */
#define API_LIB_DEBUG                   LWIP_DBG_OFF


/**
 * API_MSG_DEBUG: Enable debugging in api_msg.c.
 */
#define API_MSG_DEBUG                   LWIP_DBG_OFF


/**
 * SOCKETS_DEBUG: Enable debugging in sockets.c.
 */
#define SOCKETS_DEBUG                   LWIP_DBG_OFF


/**
 * ICMP_DEBUG: Enable debugging in icmp.c.
 */
#define ICMP_DEBUG                      LWIP_DBG_OFF


/**
 * IGMP_DEBUG: Enable debugging in igmp.c.
 */
#define IGMP_DEBUG                      LWIP_DBG_OFF


/**
 * INET_DEBUG: Enable debugging in inet.c.
 */

#define INET_DEBUG                      LWIP_DBG_OFF


/**
 * IP_DEBUG: Enable debugging for IP.
 */

#define IP_DEBUG                        LWIP_DBG_OFF


/**
 * IP_REASS_DEBUG: Enable debugging in ip_frag.c for both frag & reass.
 */

#define IP_REASS_DEBUG                  LWIP_DBG_OFF


/**
 * RAW_DEBUG: Enable debugging in raw.c.
 */

#define RAW_DEBUG                       LWIP_DBG_OFF


/**
 * MEM_DEBUG: Enable debugging in mem.c.
 */

#define MEM_DEBUG                       LWIP_DBG_OFF


/**
 * MEMP_DEBUG: Enable debugging in memp.c.
 */

#define MEMP_DEBUG                      LWIP_DBG_OFF


/**
 * SYS_DEBUG: Enable debugging in sys.c.
 */

#define SYS_DEBUG                       LWIP_DBG_OFF


/**
 * TIMERS_DEBUG: Enable debugging in timers.c.
 */

#define TIMERS_DEBUG                    LWIP_DBG_OFF


/**
 * TCP_DEBUG: Enable debugging for TCP.
 */

#define TCP_DEBUG                       LWIP_DBG_OFF


/**
 * TCP_INPUT_DEBUG: Enable debugging in tcp_in.c for incoming debug.
 */

#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF


/**
 * TCP_FR_DEBUG: Enable debugging in tcp_in.c for fast retransmit.
 */

#define TCP_FR_DEBUG                    LWIP_DBG_OFF


/**
 * TCP_RTO_DEBUG: Enable debugging in TCP for retransmit
 * timeout.
 */

#define TCP_RTO_DEBUG                   LWIP_DBG_OFF


/**
 * TCP_CWND_DEBUG: Enable debugging for TCP congestion window.
 */

#define TCP_CWND_DEBUG                  LWIP_DBG_OFF


/**
 * TCP_WND_DEBUG: Enable debugging in tcp_in.c for window updating.
 */

#define TCP_WND_DEBUG                   LWIP_DBG_OFF


/**
 * TCP_OUTPUT_DEBUG: Enable debugging in tcp_out.c output functions.
 */

#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF


/**
 * TCP_RST_DEBUG: Enable debugging for TCP with the RST message.
 */

#define TCP_RST_DEBUG                   LWIP_DBG_OFF


/**
 * TCP_QLEN_DEBUG: Enable debugging for TCP queue lengths.
 */

#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF


/**
 * UDP_DEBUG: Enable debugging in UDP.
 */

#define UDP_DEBUG                       LWIP_DBG_OFF


/**
 * TCPIP_DEBUG: Enable debugging in tcpip.c.
 */

#define TCPIP_DEBUG                     LWIP_DBG_OFF


/**
 * PPP_DEBUG: Enable debugging for PPP.
 */
#define PPP_DEBUG                       LWIP_DBG_OFF


/**
 * SLIP_DEBUG: Enable debugging in slipif.c.
 */
#define SLIP_DEBUG                      LWIP_DBG_OFF


/**
 * DHCP_DEBUG: Enable debugging in dhcp.c.
 */
#define DHCP_DEBUG                      LWIP_DBG_OFF


/**
 * AUTOIP_DEBUG: Enable debugging in autoip.c.
 */
#define AUTOIP_DEBUG                    LWIP_DBG_OFF


/**
 * SNMP_MSG_DEBUG: Enable debugging for SNMP messages.
 */
#define SNMP_MSG_DEBUG                  LWIP_DBG_OFF


/**
 * SNMP_MIB_DEBUG: Enable debugging for SNMP MIBs.
 */
#define SNMP_MIB_DEBUG                  LWIP_DBG_OFF


/**
 * DNS_DEBUG: Enable debugging for DNS.
 */
#define DNS_DEBUG                       LWIP_DBG_OFF


/**
 * IP6_DEBUG: Enable debugging for IPv6.
 */
#define IP6_DEBUG                       LWIP_DBG_OFF


#define MEM_ALIGNMENT                   4

/**
 * TCP_CALCULATE_EFF_SEND_MSS: "The maximum size of a segment that TCP really
 * sends, the 'effective send MSS,' MUST be the smaller of the send MSS (which
 * reflects the available reassembly buffer size at the remote host) and the
 * largest size permitted by the IP layer" (RFC 1122)
 * Setting this to 1 enables code that checks TCP_MSS against the MTU of the
 * netif used for a connection and limits the MSS if it would be too big otherwise.
 */
//#define TCP_CALCULATE_EFF_SEND_MSS      0 /*XXX */


/*
   ---------------------------------
   ---------- PPP options ----------
   ---------------------------------
*/
/**
 * PPP_SUPPORT==1: Enable PPP.
 */
#define PPP_SUPPORT                     1


#define PPPOS_SUPPORT                   PPP_SUPPORT


/**
 * PAP_SUPPORT==1: Support PAP.
 */
#define PAP_SUPPORT                     1

/**
 * CHAP_SUPPORT==1: Support CHAP.
 */
#define CHAP_SUPPORT                    1

/**
 * VJ_SUPPORT==1: Support VJ header compression.
 */
#define VJ_SUPPORT                      1

/**
 * MD5_SUPPORT==1: Support MD5 (see also CHAP).
 */
#define MD5_SUPPORT                     1

#define PPP_DEFMRU                     1500             /* Try for this */

#endif
