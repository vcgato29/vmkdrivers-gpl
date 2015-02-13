/*******************************************************************************

  Intel(R) Gigabit Ethernet Linux driver
  Copyright(c) 2007-2009 Intel Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/

#ifndef _E1000_82575_H_
#define _E1000_82575_H_

#define ID_LED_DEFAULT_82575_SERDES ((ID_LED_DEF1_DEF2 << 12) | \
                                     (ID_LED_DEF1_DEF2 <<  8) | \
                                     (ID_LED_DEF1_DEF2 <<  4) | \
                                     (ID_LED_OFF1_ON2))
/*
 * Receive Address Register Count
 * Number of high/low register pairs in the RAR.  The RAR (Receive Address
 * Registers) holds the directed and multicast addresses that we monitor.
 * These entries are also used for MAC-based filtering.
 */
/*
 * For 82576, there are an additional set of RARs that begin at an offset
 * separate from the first set of RARs.
 */
#define E1000_RAR_ENTRIES_82575   16
#define E1000_RAR_ENTRIES_82576   24
#define E1000_RAR_ENTRIES_82580        24
#define E1000_SW_SYNCH_MB              0x00000100
#define E1000_STAT_DEV_RST_SET         0x00100000
#define E1000_CTRL_DEV_RST             0x20000000
#define E1000_RAR_ENTRIES_I350         32

struct e1000_adv_data_desc {
	__le64 buffer_addr;    /* Address of the descriptor's data buffer */
	union {
		u32 data;
		struct {
			u32 datalen :16; /* Data buffer length */
			u32 rsvd    :4;
			u32 dtyp    :4;  /* Descriptor type */
			u32 dcmd    :8;  /* Descriptor command */
		} config;
	} lower;
	union {
		u32 data;
		struct {
			u32 status  :4;  /* Descriptor status */
			u32 idx     :4;
			u32 popts   :6;  /* Packet Options */
			u32 paylen  :18; /* Payload length */
		} options;
	} upper;
};

#define E1000_TXD_DTYP_ADV_C    0x2  /* Advanced Context Descriptor */
#define E1000_TXD_DTYP_ADV_D    0x3  /* Advanced Data Descriptor */
#define E1000_ADV_TXD_CMD_DEXT  0x20 /* Descriptor extension (0 = legacy) */
#define E1000_ADV_TUCMD_IPV4    0x2  /* IP Packet Type: 1=IPv4 */
#define E1000_ADV_TUCMD_IPV6    0x0  /* IP Packet Type: 0=IPv6 */
#define E1000_ADV_TUCMD_L4T_UDP 0x0  /* L4 Packet TYPE of UDP */
#define E1000_ADV_TUCMD_L4T_TCP 0x4  /* L4 Packet TYPE of TCP */
#define E1000_ADV_TUCMD_MKRREQ  0x10 /* Indicates markers are required */
#define E1000_ADV_DCMD_EOP      0x1  /* End of Packet */
#define E1000_ADV_DCMD_IFCS     0x2  /* Insert FCS (Ethernet CRC) */
#define E1000_ADV_DCMD_RS       0x8  /* Report Status */
#define E1000_ADV_DCMD_VLE      0x40 /* Add VLAN tag */
#define E1000_ADV_DCMD_TSE      0x80 /* TCP Seg enable */
/* Extended Device Control */
#define E1000_CTRL_EXT_NSICR    0x00000001 /* Disable Intr Clear all on read */

struct e1000_adv_context_desc {
	union {
		u32 ip_config;
		struct {
			u32 iplen    :9;
			u32 maclen   :7;
			u32 vlan_tag :16;
		} fields;
	} ip_setup;
	u32 seq_num;
	union {
		u64 l4_config;
		struct {
			u32 mkrloc :9;
			u32 tucmd  :11;
			u32 dtyp   :4;
			u32 adv    :8;
			u32 rsvd   :4;
			u32 idx    :4;
			u32 l4len  :8;
			u32 mss    :16;
		} fields;
	} l4_setup;
};

/* SRRCTL bit definitions */
#define E1000_SRRCTL_BSIZEPKT_SHIFT                     10 /* Shift _right_ */
#define E1000_SRRCTL_BSIZEHDRSIZE_MASK                  0x00000F00
#define E1000_SRRCTL_BSIZEHDRSIZE_SHIFT                 2  /* Shift _left_ */
#define E1000_SRRCTL_DESCTYPE_LEGACY                    0x00000000
#define E1000_SRRCTL_DESCTYPE_ADV_ONEBUF                0x02000000
#define E1000_SRRCTL_DESCTYPE_HDR_SPLIT                 0x04000000
#define E1000_SRRCTL_DESCTYPE_HDR_SPLIT_ALWAYS          0x0A000000
#define E1000_SRRCTL_DESCTYPE_HDR_REPLICATION           0x06000000
#define E1000_SRRCTL_DESCTYPE_HDR_REPLICATION_LARGE_PKT 0x08000000
#define E1000_SRRCTL_DESCTYPE_MASK                      0x0E000000
#define E1000_SRRCTL_TIMESTAMP                          0x40000000
#define E1000_SRRCTL_DROP_EN                            0x80000000

#define E1000_SRRCTL_BSIZEPKT_MASK      0x0000007F
#define E1000_SRRCTL_BSIZEHDR_MASK      0x00003F00

#define E1000_TX_HEAD_WB_ENABLE   0x1
#define E1000_TX_SEQNUM_WB_ENABLE 0x2

#define E1000_MRQC_ENABLE_RSS_4Q            0x00000002
#define E1000_MRQC_ENABLE_VMDQ              0x00000003
#define E1000_MRQC_ENABLE_VMDQ_RSS_2Q       0x00000005
#define E1000_MRQC_RSS_FIELD_IPV4_UDP       0x00400000
#define E1000_MRQC_RSS_FIELD_IPV6_UDP       0x00800000
#define E1000_MRQC_RSS_FIELD_IPV6_UDP_EX    0x01000000
#define E1000_MRQC_ENABLE_RSS_8Q            0x00000002

#define E1000_VMRCTL_MIRROR_PORT_SHIFT      8
#define E1000_VMRCTL_MIRROR_DSTPORT_MASK    (7 << E1000_VMRCTL_MIRROR_PORT_SHIFT)
#define E1000_VMRCTL_POOL_MIRROR_ENABLE     (1 << 0)
#define E1000_VMRCTL_UPLINK_MIRROR_ENABLE   (1 << 1)
#define E1000_VMRCTL_DOWNLINK_MIRROR_ENABLE (1 << 2)

#define E1000_EICR_TX_QUEUE ( \
    E1000_EICR_TX_QUEUE0 |    \
    E1000_EICR_TX_QUEUE1 |    \
    E1000_EICR_TX_QUEUE2 |    \
    E1000_EICR_TX_QUEUE3)

#define E1000_EICR_RX_QUEUE ( \
    E1000_EICR_RX_QUEUE0 |    \
    E1000_EICR_RX_QUEUE1 |    \
    E1000_EICR_RX_QUEUE2 |    \
    E1000_EICR_RX_QUEUE3)

#define E1000_EIMS_RX_QUEUE E1000_EICR_RX_QUEUE
#define E1000_EIMS_TX_QUEUE E1000_EICR_TX_QUEUE

#define EIMS_ENABLE_MASK ( \
    E1000_EIMS_RX_QUEUE  | \
    E1000_EIMS_TX_QUEUE  | \
    E1000_EIMS_TCP_TIMER | \
    E1000_EIMS_OTHER)

/* Immediate Interrupt Rx (A.K.A. Low Latency Interrupt) */
#define E1000_IMIR_PORT_IM_EN     0x00010000  /* TCP port enable */
#define E1000_IMIR_PORT_BP        0x00020000  /* TCP port check bypass */
#define E1000_IMIREXT_SIZE_BP     0x00001000  /* Packet size bypass */
#define E1000_IMIREXT_CTRL_URG    0x00002000  /* Check URG bit in header */
#define E1000_IMIREXT_CTRL_ACK    0x00004000  /* Check ACK bit in header */
#define E1000_IMIREXT_CTRL_PSH    0x00008000  /* Check PSH bit in header */
#define E1000_IMIREXT_CTRL_RST    0x00010000  /* Check RST bit in header */
#define E1000_IMIREXT_CTRL_SYN    0x00020000  /* Check SYN bit in header */
#define E1000_IMIREXT_CTRL_FIN    0x00040000  /* Check FIN bit in header */
#define E1000_IMIREXT_CTRL_BP     0x00080000  /* Bypass check of ctrl bits */

/* Receive Descriptor - Advanced */
union e1000_adv_rx_desc {
	struct {
		__le64 pkt_addr;             /* Packet buffer address */
		__le64 hdr_addr;             /* Header buffer address */
	} read;
	struct {
		struct {
			union {
				__le32 data;
				struct {
					__le16 pkt_info; /*RSS type, Pkt type*/
					__le16 hdr_info; /* Split Header,
				        	          * header buffer len*/
				} hs_rss;
			} lo_dword;
			union {
				__le32 rss;          /* RSS Hash */
				struct {
					__le16 ip_id;    /* IP id */
					__le16 csum;     /* Packet Checksum */
				} csum_ip;
			} hi_dword;
		} lower;
		struct {
			__le32 status_error;     /* ext status/error */
			__le16 length;           /* Packet length */
			__le16 vlan;             /* VLAN tag */
		} upper;
	} wb;  /* writeback */
};

#define E1000_RXDADV_RSSTYPE_MASK        0x0000000F
#define E1000_RXDADV_RSSTYPE_SHIFT       12
#define E1000_RXDADV_HDRBUFLEN_MASK      0x7FE0
#define E1000_RXDADV_HDRBUFLEN_SHIFT     5
#define E1000_RXDADV_SPLITHEADER_EN      0x00001000
#define E1000_RXDADV_SPH                 0x8000
#define E1000_RXDADV_STAT_TS             0x10000 /* Pkt was time stamped */
#define E1000_RXDADV_STAT_TSIP           0x08000 /* timestamp in packet */
#define E1000_RXDADV_ERR_HBO             0x00800000

/* RSS Hash results */
#define E1000_RXDADV_RSSTYPE_NONE        0x00000000
#define E1000_RXDADV_RSSTYPE_IPV4_TCP    0x00000001
#define E1000_RXDADV_RSSTYPE_IPV4        0x00000002
#define E1000_RXDADV_RSSTYPE_IPV6_TCP    0x00000003
#define E1000_RXDADV_RSSTYPE_IPV6_EX     0x00000004
#define E1000_RXDADV_RSSTYPE_IPV6        0x00000005
#define E1000_RXDADV_RSSTYPE_IPV6_TCP_EX 0x00000006
#define E1000_RXDADV_RSSTYPE_IPV4_UDP    0x00000007
#define E1000_RXDADV_RSSTYPE_IPV6_UDP    0x00000008
#define E1000_RXDADV_RSSTYPE_IPV6_UDP_EX 0x00000009

/* RSS Packet Types as indicated in the receive descriptor */
#define E1000_RXDADV_PKTTYPE_NONE        0x00000000
#define E1000_RXDADV_PKTTYPE_IPV4        0x00000010 /* IPV4 hdr present */
#define E1000_RXDADV_PKTTYPE_IPV4_EX     0x00000020 /* IPV4 hdr + extensions */
#define E1000_RXDADV_PKTTYPE_IPV6        0x00000040 /* IPV6 hdr present */
#define E1000_RXDADV_PKTTYPE_IPV6_EX     0x00000080 /* IPV6 hdr + extensions */
#define E1000_RXDADV_PKTTYPE_TCP         0x00000100 /* TCP hdr present */
#define E1000_RXDADV_PKTTYPE_UDP         0x00000200 /* UDP hdr present */
#define E1000_RXDADV_PKTTYPE_SCTP        0x00000400 /* SCTP hdr present */
#define E1000_RXDADV_PKTTYPE_NFS         0x00000800 /* NFS hdr present */

#define E1000_RXDADV_PKTTYPE_IPSEC_ESP   0x00001000 /* IPSec ESP */
#define E1000_RXDADV_PKTTYPE_IPSEC_AH    0x00002000 /* IPSec AH */
#define E1000_RXDADV_PKTTYPE_LINKSEC     0x00004000 /* LinkSec Encap */
#define E1000_RXDADV_PKTTYPE_ETQF        0x00008000 /* PKTTYPE is ETQF index */
#define E1000_RXDADV_PKTTYPE_ETQF_MASK   0x00000070 /* ETQF has 8 indices */
#define E1000_RXDADV_PKTTYPE_ETQF_SHIFT  4          /* Right-shift 4 bits */

/* LinkSec results */
/* Security Processing bit Indication */
#define E1000_RXDADV_LNKSEC_STATUS_SECP         0x00020000
#define E1000_RXDADV_LNKSEC_ERROR_BIT_MASK      0x18000000
#define E1000_RXDADV_LNKSEC_ERROR_NO_SA_MATCH   0x08000000
#define E1000_RXDADV_LNKSEC_ERROR_REPLAY_ERROR  0x10000000
#define E1000_RXDADV_LNKSEC_ERROR_BAD_SIG       0x18000000

#define E1000_RXDADV_IPSEC_STATUS_SECP          0x00020000
#define E1000_RXDADV_IPSEC_ERROR_BIT_MASK       0x18000000
#define E1000_RXDADV_IPSEC_ERROR_INVALID_PROTOCOL       0x08000000
#define E1000_RXDADV_IPSEC_ERROR_INVALID_LENGTH         0x10000000
#define E1000_RXDADV_IPSEC_ERROR_AUTHENTICATION_FAILED  0x18000000

/* Transmit Descriptor - Advanced */
union e1000_adv_tx_desc {
	struct {
		__le64 buffer_addr;    /* Address of descriptor's data buf */
		__le32 cmd_type_len;
		__le32 olinfo_status;
	} read;
	struct {
		__le64 rsvd;       /* Reserved */
		__le32 nxtseq_seed;
		__le32 status;
	} wb;
};

/* Adv Transmit Descriptor Config Masks */
#define E1000_ADVTXD_DTYP_CTXT    0x00200000 /* Advanced Context Descriptor */
#define E1000_ADVTXD_DTYP_DATA    0x00300000 /* Advanced Data Descriptor */
#define E1000_ADVTXD_DCMD_EOP     0x01000000 /* End of Packet */
#define E1000_ADVTXD_DCMD_IFCS    0x02000000 /* Insert FCS (Ethernet CRC) */
#define E1000_ADVTXD_DCMD_RS      0x08000000 /* Report Status */
#define E1000_ADVTXD_DCMD_DDTYP_ISCSI  0x10000000 /* DDP hdr type or iSCSI */
#define E1000_ADVTXD_DCMD_DEXT    0x20000000 /* Descriptor extension (1=Adv) */
#define E1000_ADVTXD_DCMD_VLE     0x40000000 /* VLAN pkt enable */
#define E1000_ADVTXD_DCMD_TSE     0x80000000 /* TCP Seg enable */
#define E1000_ADVTXD_MAC_LINKSEC  0x00040000 /* Apply LinkSec on packet */
#define E1000_ADVTXD_MAC_TSTAMP   0x00080000 /* IEEE1588 Timestamp packet */
#define E1000_ADVTXD_STAT_SN_CRC  0x00000002 /* NXTSEQ/SEED present in WB */
#define E1000_ADVTXD_IDX_SHIFT    4  /* Adv desc Index shift */
#define E1000_ADVTXD_POPTS_ISCO_1ST  0x00000000 /* 1st TSO of iSCSI PDU */
#define E1000_ADVTXD_POPTS_ISCO_MDL  0x00000800 /* Middle TSO of iSCSI PDU */
#define E1000_ADVTXD_POPTS_ISCO_LAST 0x00001000 /* Last TSO of iSCSI PDU */
#define E1000_ADVTXD_POPTS_ISCO_FULL 0x00001800 /* 1st&Last TSO-full iSCSI PDU*/
#define E1000_ADVTXD_POPTS_IPSEC     0x00000400 /* IPSec offload request */
#define E1000_ADVTXD_PAYLEN_SHIFT    14 /* Adv desc PAYLEN shift */

/* Context descriptors */
struct e1000_adv_tx_context_desc {
	__le32 vlan_macip_lens;
	__le32 seqnum_seed;
	__le32 type_tucmd_mlhl;
	__le32 mss_l4len_idx;
};

#define E1000_ADVTXD_MACLEN_SHIFT    9  /* Adv ctxt desc mac len shift */
#define E1000_ADVTXD_VLAN_SHIFT     16  /* Adv ctxt vlan tag shift */
#define E1000_ADVTXD_TUCMD_IPV4    0x00000400  /* IP Packet Type: 1=IPv4 */
#define E1000_ADVTXD_TUCMD_IPV6    0x00000000  /* IP Packet Type: 0=IPv6 */
#define E1000_ADVTXD_TUCMD_L4T_UDP 0x00000000  /* L4 Packet TYPE of UDP */
#define E1000_ADVTXD_TUCMD_L4T_TCP 0x00000800  /* L4 Packet TYPE of TCP */
#define E1000_ADVTXD_TUCMD_L4T_SCTP 0x00001000  /* L4 Packet TYPE of SCTP */
#define E1000_ADVTXD_TUCMD_IPSEC_TYPE_ESP    0x00002000 /* IPSec Type ESP */
/* IPSec Encrypt Enable for ESP */
#define E1000_ADVTXD_TUCMD_IPSEC_ENCRYPT_EN  0x00004000
#define E1000_ADVTXD_TUCMD_MKRREQ  0x00002000 /* Req requires Markers and CRC */
#define E1000_ADVTXD_L4LEN_SHIFT     8  /* Adv ctxt L4LEN shift */
#define E1000_ADVTXD_MSS_SHIFT      16  /* Adv ctxt MSS shift */
/* Adv ctxt IPSec SA IDX mask */
#define E1000_ADVTXD_IPSEC_SA_INDEX_MASK     0x000000FF
/* Adv ctxt IPSec ESP len mask */
#define E1000_ADVTXD_IPSEC_ESP_LEN_MASK      0x000000FF

/* Additional Transmit Descriptor Control definitions */
#define E1000_TXDCTL_QUEUE_ENABLE  0x02000000 /* Enable specific Tx Queue */
#define E1000_TXDCTL_SWFLSH        0x04000000 /* Tx Desc. write-back flushing */
/* Tx Queue Arbitration Priority 0=low, 1=high */
#define E1000_TXDCTL_PRIORITY      0x08000000

/* Additional Receive Descriptor Control definitions */
#define E1000_RXDCTL_QUEUE_ENABLE  0x02000000 /* Enable specific Rx Queue */
#define E1000_RXDCTL_SWFLSH        0x04000000 /* Rx Desc. write-back flushing */

/* Direct Cache Access (DCA) definitions */
#define E1000_DCA_CTRL_DCA_ENABLE  0x00000000 /* DCA Enable */
#define E1000_DCA_CTRL_DCA_DISABLE 0x00000001 /* DCA Disable */

#define E1000_DCA_CTRL_DCA_MODE_CB1 0x00 /* DCA Mode CB1 */
#define E1000_DCA_CTRL_DCA_MODE_CB2 0x02 /* DCA Mode CB2 */

#define E1000_DCA_RXCTRL_CPUID_MASK 0x0000001F /* Rx CPUID Mask */
#define E1000_DCA_RXCTRL_DESC_DCA_EN (1 << 5) /* DCA Rx Desc enable */
#define E1000_DCA_RXCTRL_HEAD_DCA_EN (1 << 6) /* DCA Rx Desc header enable */
#define E1000_DCA_RXCTRL_DATA_DCA_EN (1 << 7) /* DCA Rx Desc payload enable */

#define E1000_DCA_TXCTRL_CPUID_MASK 0x0000001F /* Tx CPUID Mask */
#define E1000_DCA_TXCTRL_DESC_DCA_EN (1 << 5) /* DCA Tx Desc enable */
#define E1000_DCA_TXCTRL_TX_WB_RO_EN (1 << 11) /* Tx Desc writeback RO bit */

#define E1000_DCA_TXCTRL_CPUID_MASK_82576 0xFF000000 /* Tx CPUID Mask */
#define E1000_DCA_RXCTRL_CPUID_MASK_82576 0xFF000000 /* Rx CPUID Mask */
#define E1000_DCA_TXCTRL_CPUID_SHIFT_82576 24 /* Tx CPUID */
#define E1000_DCA_RXCTRL_CPUID_SHIFT_82576 24 /* Rx CPUID */

/* Additional interrupt register bit definitions */
#define E1000_ICR_LSECPNS       0x00000020          /* PN threshold - server */
#define E1000_IMS_LSECPNS       E1000_ICR_LSECPNS   /* PN threshold - server */
#define E1000_ICS_LSECPNS       E1000_ICR_LSECPNS   /* PN threshold - server */

/* ETQF register bit definitions */
#define E1000_ETQF_FILTER_ENABLE   (1 << 26)
#define E1000_ETQF_IMM_INT         (1 << 29)
#define E1000_ETQF_1588            (1 << 30)
#define E1000_ETQF_QUEUE_ENABLE    (1 << 31)
/*
 * ETQF filter list: one static filter per filter consumer. This is
 *                   to avoid filter collisions later. Add new filters
 *                   here!!
 *
 * Current filters:
 *    EAPOL 802.1x (0x888e): Filter 0
 */
#define E1000_ETQF_FILTER_EAPOL          0

#define E1000_FTQF_VF_BP               0x00008000
#define E1000_FTQF_1588_TIME_STAMP     0x08000000
#define E1000_FTQF_MASK                0xF0000000
#define E1000_FTQF_MASK_PROTO_BP       0x10000000
#define E1000_FTQF_MASK_SOURCE_ADDR_BP 0x20000000
#define E1000_FTQF_MASK_DEST_ADDR_BP   0x40000000
#define E1000_FTQF_MASK_SOURCE_PORT_BP 0x80000000

#define E1000_NVM_APME_82575          0x0400
#define MAX_NUM_VFS                   8

#define E1000_DTXSWC_MAC_SPOOF_MASK   0x000000FF /* Per VF MAC spoof control */
#define E1000_DTXSWC_VLAN_SPOOF_MASK  0x0000FF00 /* Per VF VLAN spoof control */
#define E1000_DTXSWC_LLE_MASK         0x00FF0000 /* Per VF Local LB enables */
#define E1000_DTXSWC_VLAN_SPOOF_SHIFT 8
#define E1000_DTXSWC_LLE_SHIFT        16
#define E1000_DTXSWC_VMDQ_LOOPBACK_EN (1 << 31)  /* global VF LB enable */

/* Easy defines for setting default pool, would normally be left a zero */
#define E1000_VT_CTL_DEFAULT_POOL_SHIFT 7
#define E1000_VT_CTL_DEFAULT_POOL_MASK  (0x7 << E1000_VT_CTL_DEFAULT_POOL_SHIFT)

/* Other useful VMD_CTL register defines */
#define E1000_VT_CTL_IGNORE_MAC         (1 << 28)
#define E1000_VT_CTL_DISABLE_DEF_POOL   (1 << 29)
#define E1000_VT_CTL_VM_REPL_EN         (1 << 30)

/* Per VM Offload register setup */
#define E1000_VMOLR_RLPML_MASK 0x00003FFF /* Long Packet Maximum Length mask */
#define E1000_VMOLR_LPE        0x00010000 /* Accept Long packet */
#define E1000_VMOLR_RSSE       0x00020000 /* Enable RSS */
#define E1000_VMOLR_AUPE       0x01000000 /* Accept untagged packets */
#define E1000_VMOLR_ROMPE      0x02000000 /* Accept overflow multicast */
#define E1000_VMOLR_ROPE       0x04000000 /* Accept overflow unicast */
#define E1000_VMOLR_BAM        0x08000000 /* Accept Broadcast packets */
#define E1000_VMOLR_MPME       0x10000000 /* Multicast promiscuous mode */
#define E1000_VMOLR_STRVLAN    0x40000000 /* Vlan stripping enable */
#define E1000_VMOLR_STRCRC     0x80000000 /* CRC stripping enable */

#define E1000_VLVF_ARRAY_SIZE     32
#define E1000_VLVF_VLANID_MASK    0x00000FFF
#define E1000_VLVF_POOLSEL_SHIFT  12
#define E1000_VLVF_POOLSEL_MASK   (0xFF << E1000_VLVF_POOLSEL_SHIFT)
#define E1000_VLVF_LVLAN          0x00100000
#define E1000_VLVF_VLANID_ENABLE  0x80000000

#define E1000_VMVIR_VLANA_DEFAULT 0x40000000 /* Always use default VLAN */
#define E1000_VMVIR_VLANA_NEVER   0x80000000 /* Never insert VLAN tag */

#define E1000_VF_INIT_TIMEOUT 200 /* Number of retries to clear RSTI */

#define E1000_IOVCTL 0x05BBC
#define E1000_IOVCTL_REUSE_VFQ 0x00000001

#define E1000_RPLOLR_STRVLAN   0x40000000
#define E1000_RPLOLR_STRCRC    0x80000000

#define E1000_DTXCTL_8023LL     0x0004
#define E1000_DTXCTL_VLAN_ADDED 0x0008
#define E1000_DTXCTL_OOS_ENABLE 0x0010
#define E1000_DTXCTL_MDP_EN     0x0020
#define E1000_DTXCTL_SPOOF_INT  0x0040

#define ALL_QUEUES   0xFFFF

/* RX packet buffer size defines */
#define E1000_RXPBS_SIZE_MASK_82576  0x0000007F
void e1000_vmdq_set_loopback_pf(struct e1000_hw *hw, bool enable);
void e1000_vmdq_set_replication_pf(struct e1000_hw *hw, bool enable);
u16 e1000_rxpbs_adjust_82580(u32 data);
#endif /* _E1000_82575_H_ */
