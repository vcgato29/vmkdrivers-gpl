/*
 * Portions Copyright 2009, 2010 VMware, Inc.
 */ 
/*
 * This is the Fusion MPT base driver providing common API layer interface
 * for access to MPT (Message Passing Technology) firmware.
 *
 * This code is based on drivers/scsi/mpt2sas/mpt2_base.h
 * Copyright (C) 2007-2010  LSI Corporation
 *  (mailto:DL-MPTFusionLinux@lsi.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * NO WARRANTY
 * THE PROGRAM IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT
 * LIMITATION, ANY WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Each Recipient is
 * solely responsible for determining the appropriateness of using and
 * distributing the Program and assumes all risks associated with its
 * exercise of rights under this Agreement, including but not limited to
 * the risks and costs of program errors, damage to or loss of data,
 * programs or equipment, and unavailability or interruption of operations.

 * DISCLAIMER OF LIABILITY
 * NEITHER RECIPIENT NOR ANY CONTRIBUTORS SHALL HAVE ANY LIABILITY FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING WITHOUT LIMITATION LOST PROFITS), HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OR DISTRIBUTION OF THE PROGRAM OR THE EXERCISE OF ANY RIGHTS GRANTED
 * HEREUNDER, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#ifndef MPT2SAS_BASE_H_INCLUDED
#define MPT2SAS_BASE_H_INCLUDED

#include "mpi/mpi2_type.h"
#include "mpi/mpi2.h"
#include "mpi/mpi2_ioc.h"
#include "mpi/mpi2_cnfg.h"
#include "mpi/mpi2_init.h"
#include "mpi/mpi2_raid.h"
#include "mpi/mpi2_targ.h"
#include "mpi/mpi2_tool.h"
#include "mpi/mpi2_sas.h"

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsi_transport_sas.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_eh.h>

#include "mpt2sas_compatibility.h"
#include "mpt2sas_debug.h"

/* driver versioning info */
#define MPT2SAS_DRIVER_NAME		"mpt2sas"
#define MPT2SAS_AUTHOR	"LSI Corporation <DL-MPTFusionLinux@lsi.com>"
#define MPT2SAS_DESCRIPTION	"LSI MPT Fusion SAS 2.0 Device Driver"
#if defined(__VMKLNX__)
#define MPT2SAS_DRIVER_VERSION		"06.00.00.00.6vmw"
#else
#define MPT2SAS_DRIVER_VERSION		"06.00.00.00"
#endif
#define MPT2SAS_MAJOR_VERSION		06
#define MPT2SAS_MINOR_VERSION		00
#define MPT2SAS_BUILD_VERSION		00
#define MPT2SAS_RELEASE_VERSION		00

/*
 * Set MPT2SAS_SG_DEPTH value based on user input.
 */
#define MPT2SAS_MAX_PHYS_SEGMENTS	MAX_PHYS_SEGMENTS
#define MPT2SAS_MIN_PHYS_SEGMENTS	16
#ifdef CONFIG_SCSI_MPT2SAS_MAX_SGE
#define MPT2SAS_SG_DEPTH		CONFIG_SCSI_MPT2SAS_MAX_SGE
#else
#define MPT2SAS_SG_DEPTH		MPT2SAS_MAX_PHYS_SEGMENTS
#endif

/*
 * Generic Defines
 */
#define MPT2SAS_SATA_QUEUE_DEPTH	32
#define MPT2SAS_SAS_QUEUE_DEPTH		254
#define MPT2SAS_RAID_QUEUE_DEPTH	128

#define MPT_NAME_LENGTH			32	/* generic length of strings */
#define MPT_STRING_LENGTH		64

#define	MPT_MAX_CALLBACKS		16

#define	 CAN_SLEEP			1
#define  NO_SLEEP			0

#define INTERNAL_CMDS_COUNT		10	/* reserved cmds */

#define MPI2_HIM_MASK			0xFFFFFFFF /* mask every bit*/

#define MPT2SAS_INVALID_DEVICE_HANDLE	0xFFFF

/*
 * reset phases
 */
#define MPT2_IOC_PRE_RESET		1 /* prior to host reset */
#define MPT2_IOC_AFTER_RESET		2 /* just after host reset */
#define MPT2_IOC_DONE_RESET		3 /* links re-initialized */

/*
 * logging format
 */
#define MPT2SAS_FMT			"%s: "
#define MPT2SAS_INFO_FMT		KERN_INFO MPT2SAS_FMT
#define MPT2SAS_NOTE_FMT		KERN_NOTICE MPT2SAS_FMT
#define MPT2SAS_WARN_FMT		KERN_WARNING MPT2SAS_FMT
#define MPT2SAS_ERR_FMT			KERN_ERR MPT2SAS_FMT

/*
 * per target private data
 */
#define MPT_TARGET_FLAGS_RAID_COMPONENT	0x01
#define MPT_TARGET_FLAGS_VOLUME		0x02
#define MPT_TARGET_FLAGS_DELETED	0x04

/* Enable Multipath suport */

#if !defined(__VMKLNX__)
#define MPT2SAS_MULTIPATH
#endif

/*
 * Dell HBA branding
 */
#define MPT2SAS_DELL_BRANDING_SIZE                 32

#define MPT2SAS_DELL_6GBPS_SAS_HBA_BRANDING        "Dell 6Gbps SAS HBA"
#define MPT2SAS_DELL_PERC_H200_ADAPTER_BRANDING    "Dell PERC H200 Adapter"
#define MPT2SAS_DELL_PERC_H200_INTEGRATED_BRANDING "Dell PERC H200 Integrated"
#define MPT2SAS_DELL_PERC_H200_MODULAR_BRANDING    "Dell PERC H200 Modular"
#define MPT2SAS_DELL_PERC_H200_EMBEDDED_BRANDING   "Dell PERC H200 Embedded"
#define MPT2SAS_DELL_PERC_H200_BRANDING            "Dell PERC H200"
#define MPT2SAS_DELL_6GBPS_SAS_BRANDING            "Dell 6Gbps SAS"

/*
 * Dell HBA SSDIDs
 */
#define MPT2SAS_DELL_6GBPS_SAS_HBA_SSDID           0x1F1C
#define MPT2SAS_DELL_PERC_H200_ADAPTER_SSDID       0x1F1D
#define MPT2SAS_DELL_PERC_H200_INTEGRATED_SSDID    0x1F1E
#define MPT2SAS_DELL_PERC_H200_MODULAR_SSDID       0x1F1F
#define MPT2SAS_DELL_PERC_H200_EMBEDDED_SSDID      0x1F20
#define MPT2SAS_DELL_PERC_H200_SSDID               0x1F21
#define MPT2SAS_DELL_6GBPS_SAS_SSDID               0x1F22

/*
 *  End to End Data Protection Support
 */
#define EEDP_SUPPORT
#ifdef EEDP_SUPPORT
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))

#define PRO_R MPI2_SCSIIO_EEDPFLAGS_CHECK_REMOVE_OP
#define PRO_W MPI2_SCSIIO_EEDPFLAGS_INSERT_OP
#define PRO_V MPI2_SCSIIO_EEDPFLAGS_INSERT_OP

/* the read capacity 16 byte parameter block - defined in SBC-3 */
struct read_cap_parameter{
	u64	logical_block_addr;
	u32	logical_block_length;
	u8	prot_en:1;
	u8	p_type:3;
	u8	reserved0:4;
	u8	logical_blocks_per_phyical_block:4;
	u8	reserved1:4;
	u16	lowest_aligned_log_block_address:14;
	u16	reserved2:2;
	u8	reserved3[16];
};
#endif
#endif

/* OEM Identifiers */
#define MFG10_OEM_ID_INVALID                   (0x00000000)
#define MFG10_OEM_ID_DELL                      (0x00000001)
#define MFG10_OEM_ID_FSC                       (0x00000002)
#define MFG10_OEM_ID_SUN                       (0x00000003)
#define MFG10_OEM_ID_IBM                       (0x00000004)

/* GENERIC Flags 0*/
#define MFG10_GF0_OCE_DISABLED                 (0x00000001)
#define MFG10_GF0_R1E_DRIVE_COUNT              (0x00000002)
#define MFG10_GF0_R10_DISPLAY                  (0x00000004)
#define MFG10_GF0_SSD_DATA_SCRUB_DISABLE       (0x00000008)
#define MFG10_GF0_SINGLE_DRIVE_R0              (0x00000010)

/* OEM Specific Flags will come from OEM specific header files */
typedef struct _MPI2_CONFIG_PAGE_MAN_10 {
    MPI2_CONFIG_PAGE_HEADER Header;                                 /* 00h */
    U8                      OEMIdentifier;                          /* 04h */
    U8                      Reserved1;                              /* 05h */
    U16                     Reserved2;                              /* 08h */
    U32                     Reserved3;                              /* 0Ch */
    U32                     GenericFlags0;                          /* 10h */
    U32                     GenericFlags1;                          /* 14h */
    U32                     Reserved4;                              /* 18h */
    U32                     OEMSpecificFlags0;                      /* 1Ch */
    U32                     OEMSpecificFlags1;                      /* 20h */
    U32                     Reserved5[18];                          /* 24h - 60h*/
} MPI2_CONFIG_PAGE_MAN_10,
  MPI2_POINTER PTR_MPI2_CONFIG_PAGE_MAN_10,
  Mpi2ManufacturingPage10_t, MPI2_POINTER pMpi2ManufacturingPage10_t;

#define RAID_CHANNEL 1

#define MFG_PAGE10_HIDE_SSDS_MASK	(0x00000003)
#define MFG_PAGE10_HIDE_ALL_DISKS	(0x00)
#define MFG_PAGE10_EXPOSE_ALL_DISKS	(0x01)
#define MFG_PAGE10_HIDE_IF_VOL_PRESENT	(0x02)

/**
 * struct MPT2SAS_TARGET - starget private hostdata
 * @starget: starget object
 * @raid_device: raid_device pointer to access volume data
 * @sas_address: target sas address
 * @handle: device handle
 * @num_luns: number luns
 * @flags: MPT_TARGET_FLAGS_XXX flags
 * @deleted: target flaged for deletion
 * @tm_busy: target is busy with TM request.
 */
struct MPT2SAS_TARGET {
#if defined(__VMKLNX__)
	struct list_head list;
	u32     nexus_loss;
	u32     configured;
#endif
	struct scsi_target *starget;
	u64	sas_address;
	struct _raid_device *raid_device;
	u16	handle;
	int	num_luns;
	u32	flags;
	u8	deleted;
	u8	tm_busy;
};

/*
 * per device private data
 */
#define MPT_DEVICE_FLAGS_INIT		0x01
#define MPT_DEVICE_TLR_ON		0x02

#if defined(__VMKLNX__)
#define MPT_DEVICE_SATA			0x04
#define MPT2SAS_LOGINFO_NEXUS_LOSS	(0x31170000)
#endif

/**
 * struct MPT2SAS_DEVICE - sdev private hostdata
 * @sas_target: starget private hostdata
 * @lun: lun number
 * @flags: MPT_DEVICE_XXX flags
 * @configured_lun: lun is configured
 * @block: device is in SDEV_BLOCK state
 * @tlr_snoop_check: flag used in determining whether to disable TLR
 * @eedp_enable: eedp support enable bit
 * @eedp_type: 0(type_1), 1(type_2), 2(type_3)
 * @eedp_block_length: block size
 */
struct MPT2SAS_DEVICE {
	struct MPT2SAS_TARGET *sas_target;
	unsigned int	lun;
	u32	flags;
	u8	configured_lun;
	u8	block;
	u8	tlr_snoop_check;
#ifdef EEDP_SUPPORT
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
	u8	eedp_enable;
	u8	eedp_type;
#endif
#endif
};

#define MPT2_CMD_NOT_USED	0x8000	/* free */
#define MPT2_CMD_COMPLETE	0x0001	/* completed */
#define MPT2_CMD_PENDING	0x0002	/* pending */
#define MPT2_CMD_REPLY_VALID	0x0004	/* reply is valid */
#define MPT2_CMD_RESET		0x0008	/* host reset dropped the command */

/**
 * struct _internal_cmd - internal commands struct
 * @mutex: mutex
 * @done: completion
 * @reply: reply message pointer
 * @sense: sense data
 * @status: MPT2_CMD_XXX status
 * @smid: system message id
 */
struct _internal_cmd {
	struct mutex mutex;
	struct completion done;
	void 	*reply;
	void 	*sense;
	u16	status;
	u16	smid;
};

#if (defined(CONFIG_SUSE_KERNEL) && defined(scsi_is_sas_phy_local)) || LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
#define MPT_WIDE_PORT_API	1
#define MPT_WIDE_PORT_API_PLUS	1
#endif

/**
 * struct _sas_device - attached device information
 * @list: sas device list
 * @starget: starget object
 * @sas_address: device sas address
 * @device_name: retrieved from the SAS IDENTIFY frame.
 * @handle: device handle
 * @sas_address_parent: sas address of parent expander or sas host
 * @enclosure_handle: enclosure handle
 * @enclosure_logical_id: enclosure logical identifier
 * @volume_handle: volume handle (valid when hidden raid member)
 * @volume_wwid: volume unique identifier
 * @device_info: bitfield provides detailed info about the device
 * @id: target id
 * @channel: target channel
 * @slot: number number
 * @phy: phy identifier provided in sas device page 0
 * @responding: used in _scsih_sas_device_mark_responding
 */
struct _sas_device {
	struct list_head list;
	struct scsi_target *starget;
	u64	sas_address;
#if defined(__VMKLNX__)
	u64	sata_id;
#endif
	u64	device_name;
	u16	handle;
	u64	sas_address_parent;
	u16	enclosure_handle;
	u64	enclosure_logical_id;
	u16	volume_handle;
	u64	volume_wwid;
	u32	device_info;
	int	id;
	int	channel;
	u16	slot;
	u8	phy;
	u8	responding;
	u8	*serial_number;
#ifdef MPT2SAS_MULTIPATH
	struct MPT2SAS_ADAPTER *ioc;
	struct _sas_device *sas_device_alt;
#endif
};

/**
 * struct _raid_device - raid volume link list
 * @list: sas device list
 * @starget: starget object
 * @sdev: scsi device struct (volumes are single lun)
 * @wwid: unique identifier for the volume
 * @handle: device handle
 * @block_size: Block size of the volume
 * @id: target id
 * @channel: target channel
 * @volume_type: the raid level
 * @device_info: bitfield provides detailed info about the hidden components
 * @num_pds: number of hidden raid components
 * @responding: used in _scsih_raid_device_mark_responding
 * @percent_complete: rescyn percent complete
 * @direct_io_enabled: Whether direct io to PDs are allowed or not
 * @stripe_exponent: X where 2powX is the stripe sz in blocks
 * @max_lba: Maximum number of LBA in the volume
 * @stripe_sz: Stripe Size of the volume
 * @device_info: Device info of the volume member disk
 * @pd_handle: Array of handles of the physical drives for direct I/O in le16
 */
#define MPT_MAX_WHK_PDS		8
struct _raid_device {
	struct list_head list;
	struct scsi_target *starget;
	struct scsi_device *sdev;
	u64	wwid;
	u16	handle;
	u16	block_sz;
	int	id;
	int	channel;
	u8	volume_type;
	u8	num_pds;
	u8	responding;
#if !defined(__VMKLNX__)
	u8	percent_complete;
#endif
	u8	direct_io_enabled;
	u8	stripe_exponent;
	u64	max_lba;
	u32	stripe_sz;
	u32	device_info;
	u16	pd_handle[MPT_MAX_WHK_PDS];
};

/**
 * struct _boot_device - boot device info
 * @is_raid: flag to indicate whether this is volume
 * @device: holds pointer for either struct _sas_device or
 *     struct _raid_device
 */
struct _boot_device {
	u8 is_raid;
	void *device;
};

/**
 * struct _sas_port - wide/narrow sas port information
 * @port_list: list of ports belonging to expander
 * @num_phys: number of phys belonging to this port
 * @remote_identify: attached device identification
 * @rphy: sas transport rphy object
 * @port: sas transport wide/narrow port object
 * @phy_list: _sas_phy list objects belonging to this port
 */
struct _sas_port {
	struct list_head port_list;
	u8	num_phys;
	struct sas_identify remote_identify;
	struct sas_rphy *rphy;
#if defined(MPT_WIDE_PORT_API)
	struct sas_port *port;
#endif
	struct list_head phy_list;
};

/**
 * struct _sas_phy - phy information
 * @port_siblings: list of phys belonging to a port
 * @identify: phy identification
 * @remote_identify: attached device identification
 * @phy: sas transport phy object
 * @phy_id: unique phy id
 * @handle: device handle for this phy
 * @attached_handle: device handle for attached device
 * @phy_belongs_to_port: port has been created for this phy
 */
struct _sas_phy {
	struct list_head port_siblings;
	struct sas_identify identify;
	struct sas_identify remote_identify;
	struct sas_phy *phy;
	u8	phy_id;
	u16	handle;
	u16	attached_handle;
	u8	phy_belongs_to_port;
};

/**
 * struct _sas_node - sas_host/expander information
 * @list: list of expanders
 * @parent_dev: parent device class
 * @num_phys: number phys belonging to this sas_host/expander
 * @sas_address: sas address of this sas_host/expander
 * @handle: handle for this sas_host/expander
 * @sas_address_parent: sas address of parent expander or sas host
 * @enclosure_handle: handle for this a member of an enclosure
 * @device_info: bitwise defining capabilities of this sas_host/expander
 * @responding: used in _scsih_expander_device_mark_responding
 * @phy: a list of phys that make up this sas_host/expander
 * @sas_port_list: list of ports attached to this sas_host/expander
 */
struct _sas_node {
	struct list_head list;
	struct device *parent_dev;
	u8	num_phys;
	u64	sas_address;
	u16	handle;
	u64	sas_address_parent;
	u16	enclosure_handle;
	u64	enclosure_logical_id;
	u8	responding;
	struct	_sas_phy *phy;
	struct list_head sas_port_list;
};

/**
 * enum reset_type - reset state
 * @FORCE_BIG_HAMMER: issue diagnostic reset
 * @SOFT_RESET: issue message_unit_reset, if fails to to big hammer
 */
enum reset_type {
	FORCE_BIG_HAMMER,
	SOFT_RESET,
};

/**
 * struct request_tracker - firmware request tracker
 * @smid: system message id
 * @scmd: scsi request pointer
 * @cb_idx: callback index
 * @direct_io: To indicate whether I/O is direct (WHK)
 * @tracker_list: list of free request (ioc->free_list)
 */
struct request_tracker {
	u16	smid;
	struct scsi_cmnd *scmd;
	u8	cb_idx;
	u8	direct_io;
	struct list_head tracker_list;
};

/**
 * struct _tr_list - target reset list
 * @handle: device handle
 * @state: state machine
 */
struct _tr_list {
	struct list_head list;
	u16	handle;
	u16	state;
};

typedef void (*MPT_ADD_SGE)(void *paddr, u32 flags_length, dma_addr_t dma_addr);

/**
 * struct MPT2SAS_ADAPTER - per adapter struct
 * @list: ioc_list
 * @shost: shost object
 * @id: unique adapter id
 * @pci_irq: irq number
 * @name: generic ioc string
 * @tmp_string: tmp string used for logging
 * @pdev: pci pdev object
 * @chip: memory mapped register space
 * @chip_phys: physical addrss prior to mapping
 * @logging_level: see mpt2sas_debug.h
 * @fwfault_debug: debuging FW timeouts
 * @ir_firmware: IR firmware present
 * @bars: bitmask of BAR's that must be configured
 * @mask_interrupts: ignore interrupt
 * @fault_reset_work_q_name: fw fault work queue
 * @fault_reset_work_q: ""
 * @fault_reset_work: ""
 * @firmware_event_name: fw event work queue
 * @firmware_event_thread: ""
 * @fw_event_lock:
 * @fw_event_list: list of fw events
 * @aen_event_read_flag: event log was read
 * @broadcast_aen_busy: broadcast aen waiting to be serviced
 * @shost_recovery: host reset in progress
 * @ioc_reset_in_progress_lock:
 * @ioc_link_reset_in_progress: phy/hard reset in progress
 * @ignore_loginfos: ignore loginfos during task managment
 * @remove_host: flag for when driver unloads, to avoid sending dev resets
 * @wait_for_port_enable_to_complete:
 * @msix_enable: flag indicating msix is enabled
 * @msix_vector_count: number msix vectors
 * @msix_table: virt address to the msix table
 * @msix_table_backup: backup msix table
 * @scsi_io_cb_idx: shost generated commands
 * @tm_cb_idx: task management commands
 * @scsih_cb_idx: scsih internal commands
 * @ctl_cb_idx: ctl internal commands
 * @ctl_tm_cb_idx: ctl task management commands
 * @base_cb_idx: base internal commands
 * @config_cb_idx: base internal commands
 * @tm_tr_cb_idx : device removal target reset handshake
 * @tm_tr_volume_cb_idx : volume removal target reset
 * @base_cmds:
 * @transport_cmds:
 * @scsih_cmds:
 * @tm_cmds:
 * @ctl_cmds:
 * @config_cmds:
 * @base_add_sg_single: handler for either 32/64 bit sgl's
 * @event_type: bits indicating which events to log
 * @event_context: unique id for each logged event
 * @event_log: event log pointer
 * @event_masks: events that are masked
 * @facts: static facts data
 * @pfacts: static port facts data
 * @manu_pg0: static manufacturing page 0
 * @manu_pg10: static manufacturing page 10
 * @bios_pg2: static bios page 2
 * @bios_pg3: static bios page 3
 * @ioc_pg8: static ioc page 8
 * @iounit_pg0: static iounit page 0
 * @iounit_pg1: static iounit page 1
 * @sas_hba: sas host object
 * @sas_expander_list: expander object list
 * @sas_node_lock:
 * @sas_device_list: sas device object list
 * @sas_device_init_list: sas device object list (used only at init time)
 * @sas_device_lock:
 * @io_missing_delay: time for IO completed by fw when PDR enabled
 * @device_missing_delay: time for device missing by fw when PDR enabled
 * @sas_id : used for setting volume target IDs
 * @pd_handles : bitmask for PD handles
 * @pd_handles_sz : size of pd_handle bitmask
 * @config_page_sz: config page size
 * @config_page: reserve memory for config page payload
 * @config_page_dma:
 * @hba_queue_depth: hba request queue depth
 * @sge_size: sg element size for either 32/64 bit
 * @scsiio_depth: SCSI_IO queue depth
 * @request_sz: per request frame size
 * @request: pool of request frames
 * @request_dma:
 * @request_dma_sz:
 * @scsi_lookup: firmware request tracker list
 * @scsi_lookup_lock:
 * @free_list: free list of request
 * @pending_io_count:
 * @reset_wq:
 * @pending_tm_count: pending task mangement request
 * @terminated_tm_count: terminated request
 * @pending_tm_wq: wait queue
 * @out_of_frames
 * @no_frames_tm_wq
 * @chain: pool of chains
 * @chain_dma:
 * @max_sges_in_main_message: number sg elements in main message
 * @max_sges_in_chain_message: number sg elements per chain
 * @chains_needed_per_io: max chains per io
 * @chain_offset_value_for_main_message: location 1st sg in main
 * @chain_depth: total chains allocated
 * @hi_priority_smid:
 * @hi_priority:
 * @hi_priority_dma:
 * @hi_priority_depth:
 * @hpr_lookup:
 * @hpr_free_list:
 * @internal_smid:
 * @internal:
 * @internal_dma:
 * @internal_depth:
 * @internal_lookup:
 * @internal_free_list:
 * @sense: pool of sense
 * @sense_dma:
 * @sense_dma_pool:
 * @reply_depth: hba reply queue depth:
 * @reply_sz: per reply frame size:
 * @reply: pool of replys:
 * @reply_dma:
 * @reply_dma_pool:
 * @reply_free_queue_depth: reply free depth
 * @reply_free: pool for reply free queue (32 bit addr)
 * @reply_free_dma:
 * @reply_free_dma_pool:
 * @reply_free_host_index: tail index in pool to insert free replys
 * @reply_post_queue_depth: reply post queue depth
 * @reply_post_free: pool for reply post (64bit descriptor)
 * @reply_post_free_dma:
 * @reply_post_free_dma_pool:
 * @reply_post_host_index: head index in the pool where FW completes IO
 * @delayed_tr_list: target reset link list
 * @delayed_tr_volume_list: volume target reset link list
 */
struct MPT2SAS_ADAPTER {
	struct list_head list;
	struct Scsi_Host *shost;
	u8		id;
	u32		pci_irq;
	char		name[MPT_NAME_LENGTH];
	char		tmp_string[MPT_STRING_LENGTH];
	struct pci_dev	*pdev;
	Mpi2SystemInterfaceRegs_t __iomem *chip;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18))
	resource_size_t	chip_phys;
#else
	u64		chip_phys;
#endif
	int		logging_level;
	int		fwfault_debug;
	u8		ir_firmware;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25))
	int		bars;
#endif
	u8		mask_interrupts;

	/* fw fault handler */
	char		fault_reset_work_q_name[20];
	struct workqueue_struct *fault_reset_work_q;
#if defined(__VMKLNX__) || (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19))
	struct delayed_work fault_reset_work;
#else
	struct work_struct fault_reset_work;
#endif

	/* fw event handler */
	char		firmware_event_name[20];
	struct workqueue_struct	*firmware_event_thread;
	spinlock_t	fw_event_lock;
	struct list_head fw_event_list;

	 /* misc flags */
	int		aen_event_read_flag;
	u8		broadcast_aen_busy;
	u8		shost_recovery;

	struct mutex	reset_in_progress_mutex;
	struct completion	shost_recovery_done;
	spinlock_t 	ioc_reset_in_progress_lock;
	u8		ioc_link_reset_in_progress;
	int		ioc_reset_in_progress_status;

	u8		ignore_loginfos;
	u8		remove_host;
	u8		wait_for_port_enable_to_complete;
	struct completion	port_enable_done;

	u8		msix_enable;
	u16		msix_vector_count;
	u32		*msix_table;
	u32		*msix_table_backup;
	u32		ioc_reset_count;

	/* internal commands, callback index */
	u8		scsi_io_cb_idx;
	u8		tm_cb_idx;
	u8		transport_cb_idx;
	u8		scsih_cb_idx;
	u8		ctl_cb_idx;
	u8		ctl_tm_cb_idx;
	u8		base_cb_idx;
	u8		config_cb_idx;
	u8		tm_tr_cb_idx;
	u8		tm_tr_volume_cb_idx;
#ifdef MPT2SAS_MULTIPATH
	u8		tm_tr_mp_cb_idx;
#endif
	u8		tm_sas_control_cb_idx;
	struct _internal_cmd base_cmds;
	struct _internal_cmd transport_cmds;
	struct _internal_cmd scsih_cmds;
	struct _internal_cmd tm_cmds;
	struct _internal_cmd ctl_cmds;
	struct _internal_cmd config_cmds;

	MPT_ADD_SGE	base_add_sg_single;

	/* event log */
	u32		event_type[MPI2_EVENT_NOTIFY_EVENTMASK_WORDS];
	u32		event_context;
	void		*event_log;
	u32		event_masks[MPI2_EVENT_NOTIFY_EVENTMASK_WORDS];

	/* static config pages */
	Mpi2IOCFactsReply_t facts;
	Mpi2PortFactsReply_t *pfacts;
	Mpi2ManufacturingPage0_t manu_pg0;
	Mpi2ManufacturingPage10_t manu_pg10;
	Mpi2BiosPage2_t	bios_pg2;
	Mpi2BiosPage3_t	bios_pg3;
	Mpi2IOCPage8_t ioc_pg8;
	Mpi2IOUnitPage0_t iounit_pg0;
	Mpi2IOUnitPage1_t iounit_pg1;

	struct _boot_device req_boot_device;
	struct _boot_device req_alt_boot_device;
	struct _boot_device current_boot_device;

	/* sas hba, expander, and device list */
	struct _sas_node sas_hba;
	struct list_head sas_expander_list;
	spinlock_t	sas_node_lock;
	struct list_head sas_device_list;
	struct list_head sas_device_init_list;
	spinlock_t	sas_device_lock;
	struct list_head raid_device_list;
	spinlock_t	raid_device_lock;
#if defined(__VMKLNX__)
	struct list_head target_list;
	spinlock_t	target_list_lock;
#endif
	u8		io_missing_delay;
	u16		device_missing_delay;
	int		sas_id;

	void		*pd_handles;
	u16		pd_handles_sz;

	/* config page */
	u16		config_page_sz;
	void 		*config_page;
	dma_addr_t	config_page_dma;

	/* scsiio request */
	u16		hba_queue_depth;
	u16		sge_size;
	u16 		scsiio_depth;
	u16		request_sz;
	u8		*request;
	dma_addr_t	request_dma;
	u32		request_dma_sz;
	struct request_tracker *scsi_lookup;
	ulong		scsi_lookup_pages;
	spinlock_t	scsi_lookup_lock;
	struct list_head free_list;
	int		pending_io_count;
	wait_queue_head_t reset_wq;
	int		pending_tm_count;
	u32		terminated_tm_count;
	wait_queue_head_t pending_tm_wq;
	u8		  out_of_frames;
	wait_queue_head_t no_frames_tm_wq;

	/* chain */
	u8		*chain;
	dma_addr_t	chain_dma;
	u16 		max_sges_in_main_message;
	u16		max_sges_in_chain_message;
	u16		chains_needed_per_io;
	u16		chain_offset_value_for_main_message;
	u32		chain_depth;

	/* hi-priority queue */
	u16		hi_priority_smid;
	u8		*hi_priority;
	dma_addr_t	hi_priority_dma;
	u16		hi_priority_depth;
	struct request_tracker *hpr_lookup;
	struct list_head hpr_free_list;

	/* internal queue */
	u16		internal_smid;
	u8		*internal;
	dma_addr_t	internal_dma;
	u16		internal_depth;
	struct request_tracker *internal_lookup;
	struct list_head internal_free_list;

	/* sense */
	u8		*sense;
	dma_addr_t	sense_dma;
	struct dma_pool *sense_dma_pool;

	/* reply */
	u16		reply_sz;
	u8		*reply;
	dma_addr_t	reply_dma;
	struct dma_pool *reply_dma_pool;

	/* reply free queue */
	u16 		reply_free_queue_depth;
	u32		*reply_free;
	dma_addr_t	reply_free_dma;
	struct dma_pool *reply_free_dma_pool;
	u32		reply_free_host_index;

	/* reply post queue */
	u16 		reply_post_queue_depth;
	Mpi2ReplyDescriptorsUnion_t *reply_post_free;
	dma_addr_t	reply_post_free_dma;
	struct dma_pool *reply_post_free_dma_pool;
	u32		reply_post_host_index;

	struct list_head delayed_tr_list;
	struct list_head delayed_tr_volume_list;
#ifdef MPT2SAS_MULTIPATH
	struct list_head delayed_tr_mp_list;
#endif

	/* diag buffer support */
	u8		*diag_buffer[MPI2_DIAG_BUF_TYPE_COUNT];
	u32		diag_buffer_sz[MPI2_DIAG_BUF_TYPE_COUNT];
	dma_addr_t	diag_buffer_dma[MPI2_DIAG_BUF_TYPE_COUNT];
	u8		diag_buffer_status[MPI2_DIAG_BUF_TYPE_COUNT];
	u32		unique_id[MPI2_DIAG_BUF_TYPE_COUNT];
	u32		product_specific[MPI2_DIAG_BUF_TYPE_COUNT][23];
	u32		diagnostic_flags[MPI2_DIAG_BUF_TYPE_COUNT];
	u32		ring_buffer_offset;
	u32		ring_buffer_sz;
#if defined(__VMKLNX__)
	u8		sata_dev_found;
#endif
	u8		is_warhawk;
	u8		hide_ir_msg;
	u8		mfg_pg10_hide_flag;
	u8		hide_drives;

#ifdef MPT2SAS_WHK_DDIOCOUNT
	u64		ddio_count;
	u64		ddio_err_count;
#endif
};

typedef u8 (*MPT_CALLBACK)(struct MPT2SAS_ADAPTER *ioc, u16 smid, u8 msix_index,
    u32 reply);

/* base shared API */
extern struct list_head mpt2sas_ioc_list;
void mpt2sas_base_start_watchdog(struct MPT2SAS_ADAPTER *ioc);
void mpt2sas_base_stop_watchdog(struct MPT2SAS_ADAPTER *ioc);
void mpt2sas_base_save_msix_table(struct MPT2SAS_ADAPTER *ioc);
void mpt2sas_base_restore_msix_table(struct MPT2SAS_ADAPTER *ioc);

int mpt2sas_base_attach(struct MPT2SAS_ADAPTER *ioc);
void mpt2sas_base_detach(struct MPT2SAS_ADAPTER *ioc);
int mpt2sas_base_map_resources(struct MPT2SAS_ADAPTER *ioc);
void mpt2sas_base_free_resources(struct MPT2SAS_ADAPTER *ioc);
int mpt2sas_base_hard_reset_handler(struct MPT2SAS_ADAPTER *ioc, int sleep_flag,
    enum reset_type type);

void *mpt2sas_base_get_msg_frame(struct MPT2SAS_ADAPTER *ioc, u16 smid);
void *mpt2sas_base_get_sense_buffer(struct MPT2SAS_ADAPTER *ioc, u16 smid);
void mpt2sas_base_build_zero_len_sge(struct MPT2SAS_ADAPTER *ioc, void *paddr);
__le32 mpt2sas_base_get_sense_buffer_dma(struct MPT2SAS_ADAPTER *ioc,
    u16 smid);

/* hi-priority queue */
u16 mpt2sas_base_get_smid_hpr(struct MPT2SAS_ADAPTER *ioc, u8 cb_idx);
u16 mpt2sas_base_get_smid_scsiio(struct MPT2SAS_ADAPTER *ioc, u8 cb_idx,
    struct scsi_cmnd *scmd);

u16 mpt2sas_base_get_smid(struct MPT2SAS_ADAPTER *ioc, u8 cb_idx);
void mpt2sas_base_free_smid(struct MPT2SAS_ADAPTER *ioc, u16 smid);
void mpt2sas_base_put_smid_scsi_io(struct MPT2SAS_ADAPTER *ioc, u16 smid,
    u16 handle);
void mpt2sas_base_put_smid_hi_priority(struct MPT2SAS_ADAPTER *ioc, u16 smid);
void mpt2sas_base_put_smid_default(struct MPT2SAS_ADAPTER *ioc, u16 smid);
void mpt2sas_base_initialize_callback_handler(void);
u8 mpt2sas_base_register_callback_handler(MPT_CALLBACK cb_func);
void mpt2sas_base_release_callback_handler(u8 cb_idx);

u8 mpt2sas_base_done(struct MPT2SAS_ADAPTER *ioc, u16 smid, u8 msix_index,
    u32 reply);
void *mpt2sas_base_get_reply_virt_addr(struct MPT2SAS_ADAPTER *ioc,
    u32 phys_addr);

u32 mpt2sas_base_get_iocstate(struct MPT2SAS_ADAPTER *ioc, int cooked);

void mpt2sas_base_fault_info(struct MPT2SAS_ADAPTER *ioc , u16 fault_code);
int mpt2sas_base_sas_iounit_control(struct MPT2SAS_ADAPTER *ioc,
    Mpi2SasIoUnitControlReply_t *mpi_reply, Mpi2SasIoUnitControlRequest_t
    *mpi_request);
int mpt2sas_base_scsi_enclosure_processor(struct MPT2SAS_ADAPTER *ioc,
    Mpi2SepReply_t *mpi_reply, Mpi2SepRequest_t *mpi_request);

void mpt2sas_base_validate_event_type(struct MPT2SAS_ADAPTER *ioc,
    u32 *event_type);

void mpt2sas_halt_firmware(struct MPT2SAS_ADAPTER *ioc);

/* scsih shared API */
u8 mpt2sas_scsih_event_callback(struct MPT2SAS_ADAPTER *ioc, u8 msix_index,
    u32 reply);
void mpt2sas_scsih_reset_handler(struct MPT2SAS_ADAPTER *ioc, int reset_phase);
#ifdef MPT2SAS_MULTIPATH
void mpt2sas_scsih_check_tm_for_multipath(struct MPT2SAS_ADAPTER *ioc,
    u16 handle, u8 task_type);
#endif
int mpt2sas_scsih_issue_tm(struct MPT2SAS_ADAPTER *ioc, u16 handle, uint channel, uint id,
    uint lun, u8 type, u16 smid_task, ulong timeout, struct scsi_cmnd *scmd);
void mpt2sas_scsih_set_tm_flag(struct MPT2SAS_ADAPTER *ioc, u16 handle);
void mpt2sas_scsih_clear_tm_flag(struct MPT2SAS_ADAPTER *ioc, u16 handle);

struct _sas_node *mpt2sas_scsih_expander_find_by_handle(
    struct MPT2SAS_ADAPTER *ioc, u16 handle);
struct _sas_node *mpt2sas_scsih_expander_find_by_sas_address(
    struct MPT2SAS_ADAPTER *ioc, u64 sas_address);
struct _sas_device *mpt2sas_scsih_sas_device_find_by_sas_address(
    struct MPT2SAS_ADAPTER *ioc, u64 sas_address);

/* config shared API */
u8 mpt2sas_config_done(struct MPT2SAS_ADAPTER *ioc, u16 smid, u8 msix_index,
    u32 reply);
int mpt2sas_config_get_number_hba_phys(struct MPT2SAS_ADAPTER *ioc,
    u8 *num_phys);
int mpt2sas_config_get_manufacturing_pg0(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2ManufacturingPage0_t *config_page);
int mpt2sas_config_get_manufacturing_pg10(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2ManufacturingPage10_t *config_page);
int mpt2sas_config_get_bios_pg2(struct MPT2SAS_ADAPTER *ioc, Mpi2ConfigReply_t
    *mpi_reply, Mpi2BiosPage2_t *config_page);
int mpt2sas_config_get_bios_pg3(struct MPT2SAS_ADAPTER *ioc, Mpi2ConfigReply_t
    *mpi_reply, Mpi2BiosPage3_t *config_page);
int mpt2sas_config_get_iounit_pg0(struct MPT2SAS_ADAPTER *ioc, Mpi2ConfigReply_t
    *mpi_reply, Mpi2IOUnitPage0_t *config_page);
int mpt2sas_config_get_sas_device_pg0(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2SasDevicePage0_t *config_page, u32 form,
    u32 handle);
int mpt2sas_config_get_sas_device_pg1(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2SasDevicePage1_t *config_page, u32 form,
    u32 handle);
int mpt2sas_config_get_sas_iounit_pg0(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2SasIOUnitPage0_t *config_page, u16 sz);
int mpt2sas_config_get_iounit_pg1(struct MPT2SAS_ADAPTER *ioc, Mpi2ConfigReply_t
    *mpi_reply, Mpi2IOUnitPage1_t *config_page);
int mpt2sas_config_set_iounit_pg1(struct MPT2SAS_ADAPTER *ioc, Mpi2ConfigReply_t
    *mpi_reply, Mpi2IOUnitPage1_t *config_page);
int mpt2sas_config_get_sas_iounit_pg1(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2SasIOUnitPage1_t *config_page, u16 sz);
int mpt2sas_config_set_sas_iounit_pg1(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2SasIOUnitPage1_t *config_page, u16 sz);
int mpt2sas_config_get_ioc_pg8(struct MPT2SAS_ADAPTER *ioc, Mpi2ConfigReply_t
    *mpi_reply, Mpi2IOCPage8_t *config_page);
int mpt2sas_config_get_expander_pg0(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2ExpanderPage0_t *config_page, u32 form,
    u32 handle);
int mpt2sas_config_get_expander_pg1(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2ExpanderPage1_t *config_page,
    u32 phy_number, u16 handle);
int mpt2sas_config_get_enclosure_pg0(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2SasEnclosurePage0_t *config_page,
    u32 form, u32 handle);
int mpt2sas_config_get_phy_pg0(struct MPT2SAS_ADAPTER *ioc, Mpi2ConfigReply_t
    *mpi_reply, Mpi2SasPhyPage0_t *config_page, u32 phy_number);
int mpt2sas_config_get_phy_pg1(struct MPT2SAS_ADAPTER *ioc, Mpi2ConfigReply_t
    *mpi_reply, Mpi2SasPhyPage1_t *config_page, u32 phy_number);
int mpt2sas_config_get_raid_volume_pg1(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2RaidVolPage1_t *config_page, u32 form,
    u32 handle);
int mpt2sas_config_get_number_pds(struct MPT2SAS_ADAPTER *ioc, u16 handle,
    u8 *num_pds);
int mpt2sas_config_get_raid_volume_pg0(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2RaidVolPage0_t *config_page, u32 form,
    u32 handle, u16 sz);
int mpt2sas_config_get_phys_disk_pg0(struct MPT2SAS_ADAPTER *ioc,
    Mpi2ConfigReply_t *mpi_reply, Mpi2RaidPhysDiskPage0_t *config_page,
    u32 form, u32 form_specific);
int mpt2sas_config_get_volume_handle(struct MPT2SAS_ADAPTER *ioc, u16 pd_handle,
    u16 *volume_handle);
int mpt2sas_config_get_volume_wwid(struct MPT2SAS_ADAPTER *ioc,
    u16 volume_handle, u64 *wwid);

/* ctl shared API */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,25))
extern struct device_attribute *mpt2sas_host_attrs[];
#else
extern struct class_device_attribute *mpt2sas_host_attrs[];
#endif
extern struct device_attribute *mpt2sas_dev_attrs[];
void mpt2sas_ctl_init(void);
void mpt2sas_ctl_exit(void);
u8 mpt2sas_ctl_done(struct MPT2SAS_ADAPTER *ioc, u16 smid, u8 msix_index,
    u32 reply);
u8 mpt2sas_ctl_tm_done(struct MPT2SAS_ADAPTER *ioc, u16 smid, u8 msix_index,
    u32 reply);
void mpt2sas_ctl_reset_handler(struct MPT2SAS_ADAPTER *ioc, int reset_phase);
u8 mpt2sas_ctl_event_callback(struct MPT2SAS_ADAPTER *ioc, u8 msix_index,
    u32 reply);
void mpt2sas_ctl_add_to_event_log(struct MPT2SAS_ADAPTER *ioc,
    Mpi2EventNotificationReply_t *mpi_reply);

void mpt2sas_enable_diag_buffer(struct MPT2SAS_ADAPTER *ioc, u8 bits_to_regsiter);

/* transport shared API */
u8 mpt2sas_transport_done(struct MPT2SAS_ADAPTER *ioc, u16 smid, u8 msix_index,
    u32 reply);
struct _sas_port *mpt2sas_transport_port_add(struct MPT2SAS_ADAPTER *ioc,
    u16 handle, u64 sas_address);
void mpt2sas_transport_port_remove(struct MPT2SAS_ADAPTER *ioc, u64 sas_address,
    u64 sas_address_parent);
int mpt2sas_transport_add_host_phy(struct MPT2SAS_ADAPTER *ioc, struct _sas_phy
    *mpt2sas_phy, Mpi2SasPhyPage0_t phy_pg0, struct device *parent_dev);
int mpt2sas_transport_add_expander_phy(struct MPT2SAS_ADAPTER *ioc,
    struct _sas_phy *mpt2sas_phy, Mpi2ExpanderPage1_t expander_pg1,
    struct device *parent_dev);
void mpt2sas_transport_update_links(struct MPT2SAS_ADAPTER *ioc,
    u64 sas_address, u16 handle, u8 phy_number, u8 link_rate);
extern struct sas_function_template mpt2sas_transport_functions;
extern struct scsi_transport_template *mpt2sas_transport_template;

#if defined(__VMKLNX__)
typedef union _SAS_ADDRESS
{
	U8				WWID[8]; /* 0x00 */
	struct
	{
		U32			High;
		U32			Low;
	} Word;
} SAS_ADDRESS;

/*
 * define the IDENTIFY DEVICE structure
 */
typedef struct _ATA_IDENTIFY_DEVICE_DATA {
    U16  GeneralConfiguration;                                  /* 0 */
    U16  LogicalCylinders;                                      /* 1 */
    U16  SpecificConfiguration;                                 /* 2 */
    U16  LogicalHeads;                                          /* 3 */
    U16  Retired4to5[2];                                        /* 4-5 */
    U16  LogicalSectorsPerLogicalTrack;                         /* 6 */
    U16  Reserved7to8[2];                                       /* 7-8 */
    U16  Retired3;                                              /* 9 */
    U16  SerialNumber[10];                                      /* 10-19 */
    U16  Retired20to21[2];                                      /* 20-21 */
    U16  Obsolete22;                                            /* 22 */
    U16  FirmwareRevision[4];                                   /* 23-26 */
    U16  ModelNumber[20];                                       /* 27-46*/
    U16  MaxMultipleSize;                                       /* 47 */
    U16  Reserved48;                                            /* 48 */
    U16  Capabilities[2];                                       /* 49-50 */
    U16  Obsolete51to52[2];                                     /* 51-52 */
    U16  ValidWords;                                            /* 53 */
    U16  CurrentLogicalCylinders;                               /* 54 */
    U16  CurrentLogicalHeads;                                   /* 55 */
    U16  CurrentLogicalSectorsPerTrack;                         /* 56 */
    U16  CurrentCapacityInSectors[2];                           /* 57-58 */
    U16  CurrentMultipleSize;                                   /* 59 */
    U16  TotalUserSectors[2];                                   /* 60-61 */
    U16  Obsolete62;                                            /* 62 */
    U16  MultiwordDmaMode;                                      /* 63 */
    U16  PioModesSupported;                                     /* 64 */
    U16  MinimumMultiwordDmaTransferCycleTime;                  /* 65 */
    U16  ManufacturerRecommendedMultiwordDmaTransferCycleTime;  /* 66 */
    U16  MinumumPioTransferCycleTimeWithoutFlowControl;         /* 67 */
    U16  MinumumPioTransferCycleTimeWithFlowControl;            /* 68 */
    U16  Reserved69to70[2];                                     /* 69-70 */
    U16  Reserved71to74[4];                                     /* 71-74 */
    U16  QueueDepth;                                            /* 75 */
    U16  SerialATACapabilities;                                 /* 76 */
    U16  Reserved77;                                            /* 77 */
    U16  SerialFeaturesSupported[2];                            /* 78-79 */
    U16  MajorVersionNumber;                                    /* 80 */
    U16  MinorVersionNumber;                                    /* 81 */
    U16  CommandSetsSupported[3];                               /* 82-84 */
    U16  CommandSetsEnabled[2];                                 /* 85-86 */
    U16  CommandSetDefaults;                                    /* 87 */
    U16  UltraDmaModes;                                         /* 88 */
    U16  TimeForSecurityErase;                                  /* 89 */
    U16  TimeForEnahncedSecurityErase;                          /* 90 */
    U16  CurrentAdvancedPowerManagementValue;                   /* 91 */
    U16  MasterPasswordRevisionCode;                            /* 92 */
    U16  HardwareResetResult;                                   /* 93 */
    U16  AcousticManagement;                                    /* 94 */ /* GWK Reserved in ATA-5 */
    U16  Reserved95to99[5];                                     /* 95-99 */
    U16  MaximumLBAfor48bitAddressing[4];                       /* 100-103 */ /* GWK Reserved in ATA-5 */
    U16  Reserved104to107[4];                                   /* 104-107 */
    U16  WorldWideName[4];                                      /* 108-111 */
    U16  Reserved112to126[15];                                  /* 112-126 */
    U16  RemovableMediaStatusNotification;                      /* 127 */
    U16  SecurityStatus;                                        /* 128 */
    U16  VendorSpecific[31];                                    /* 129-159 */
    U16  CFApowerMode;                                          /* 160 */
    U16  Reserved161to175[15];                                  /* 161-175 */
    U16  MediaSerialNumber[30];                                 /* 176-205 */
    U16  SCTCommandSetSupported;                                /* 206 */
    U16  Reserves207to254[48];                                  /* 207-254 */
    U16  IntegrityWord;                                         /* 255 */
} ATA_IDENTIFY_DEVICE_DATA, *PTR_ATA_IDENTIFY_DEVICE_DATA;

extern void
mpt2sas_connect_devices(struct MPT2SAS_ADAPTER *ioc, u64 sas_address,
    u16 handle);
extern int
mpt2sas_base_get_sata_identify(struct MPT2SAS_ADAPTER *ioc, u16 handle,
    Mpi2SataPassthroughReply_t *mpi_reply, char *id_buffer, int sz);
extern int
mpt2sas_scsih_get_sas_address_for_sata_disk(struct MPT2SAS_ADAPTER *ioc,
		U64 *pSASAddress, u16 handle);
extern struct _raid_device *
_scsih_raid_device_find_by_id(struct MPT2SAS_ADAPTER *ioc, int id, int channel);
#endif

#endif /* MPT2SAS_BASE_H_INCLUDED */
