/*
 * Portions Copyright 2008 VMware, Inc.
 */
/*
 * QLogic Fibre Channel HBA Driver
 * Copyright (c)  2003-2008 QLogic Corporation
 *
 * See LICENSE.qla2xxx for copyright and licensing details.
 */
#include "qla_def.h"

#include <linux/blkdev.h>
#include <linux/delay.h>

#include <scsi/scsi_tcq.h>
#if defined(__VMKLNX__)
#include <vmklinux_9/vmklinux_scsi.h>
#endif

static inline uint16_t qla2x00_get_cmd_direction(srb_t *);
static inline cont_entry_t *qla2x00_prep_cont_type0_iocb(scsi_qla_host_t *);
static inline cont_a64_entry_t *qla2x00_prep_cont_type1_iocb(scsi_qla_host_t *);
static request_t *qla2x00_req_pkt(scsi_qla_host_t *, struct req_que *,
	struct rsp_que *);
void qla2x00_isp_cmd(scsi_qla_host_t *, struct req_que *);

static void qla25xx_set_que(srb_t *, struct rsp_que **);
/**
 * qla2x00_get_cmd_direction() - Determine control_flag data direction.
 * @cmd: SCSI command
 *
 * Returns the proper CF_* direction based on CDB.
 */
static inline uint16_t
qla2x00_get_cmd_direction(srb_t *sp)
{
	uint16_t cflags;

	cflags = 0;

	/* Set transfer direction */
	if (sp->cmd->sc_data_direction == DMA_TO_DEVICE) {
		cflags = CF_WRITE;
		sp->fcport->vha->hw->qla_stats.output_bytes +=
		    scsi_bufflen(sp->cmd);
	} else if (sp->cmd->sc_data_direction == DMA_FROM_DEVICE) {
		cflags = CF_READ;
		sp->fcport->vha->hw->qla_stats.input_bytes +=
		    scsi_bufflen(sp->cmd);
	}
	return (cflags);
}

/**
 * qla2x00_calc_iocbs_32() - Determine number of Command Type 2 and
 * Continuation Type 0 IOCBs to allocate.
 *
 * @dsds: number of data segment decriptors needed
 *
 * Returns the number of IOCB entries needed to store @dsds.
 */
uint16_t
qla2x00_calc_iocbs_32(uint16_t dsds)
{
	uint16_t iocbs;

	iocbs = 1;
	if (dsds > 3) {
		iocbs += (dsds - 3) / 7;
		if ((dsds - 3) % 7)
			iocbs++;
	}
	return (iocbs);
}

/**
 * qla2x00_calc_iocbs_64() - Determine number of Command Type 3 and
 * Continuation Type 1 IOCBs to allocate.
 *
 * @dsds: number of data segment decriptors needed
 *
 * Returns the number of IOCB entries needed to store @dsds.
 */
uint16_t
qla2x00_calc_iocbs_64(uint16_t dsds)
{
	uint16_t iocbs;

	iocbs = 1;
	if (dsds > 2) {
		iocbs += (dsds - 2) / 5;
		if ((dsds - 2) % 5)
			iocbs++;
	}
	return (iocbs);
}

/**
 * qla2x00_prep_cont_type0_iocb() - Initialize a Continuation Type 0 IOCB.
 * @ha: HA context
 *
 * Returns a pointer to the Continuation Type 0 IOCB packet.
 */
static inline cont_entry_t *
qla2x00_prep_cont_type0_iocb(scsi_qla_host_t *vha)
{
	cont_entry_t *cont_pkt;
	struct req_que *req = vha->req;
	/* Adjust ring index. */
	req->ring_index++;
	if (req->ring_index == req->length) {
		req->ring_index = 0;
		req->ring_ptr = req->ring;
	} else {
		req->ring_ptr++;
	}

	cont_pkt = (cont_entry_t *)req->ring_ptr;

	/* Load packet defaults. */
	*((uint32_t *)(&cont_pkt->entry_type)) =
	    __constant_cpu_to_le32(CONTINUE_TYPE);

	return (cont_pkt);
}

/**
 * qla2x00_prep_cont_type1_iocb() - Initialize a Continuation Type 1 IOCB.
 * @ha: HA context
 *
 * Returns a pointer to the continuation type 1 IOCB packet.
 */
static inline cont_a64_entry_t *
qla2x00_prep_cont_type1_iocb(scsi_qla_host_t *vha)
{
	cont_a64_entry_t *cont_pkt;
	struct req_que *req = vha->req;
	/* Adjust ring index. */
	req->ring_index++;
	if (req->ring_index == req->length) {
		req->ring_index = 0;
		req->ring_ptr = req->ring;
	} else {
		req->ring_ptr++;
	}

	cont_pkt = (cont_a64_entry_t *)req->ring_ptr;

	/* Load packet defaults. */
	*((uint32_t *)(&cont_pkt->entry_type)) =
	    __constant_cpu_to_le32(CONTINUE_A64_TYPE);

	return (cont_pkt);
}

/**
 * qla2x00_build_scsi_iocbs_32() - Build IOCB command utilizing 32bit
 * capable IOCB types.
 *
 * @sp: SRB command to process
 * @cmd_pkt: Command type 2 IOCB
 * @tot_dsds: Total number of segments to transfer
 */
void qla2x00_build_scsi_iocbs_32(srb_t *sp, cmd_entry_t *cmd_pkt,
    uint16_t tot_dsds)
{
	uint16_t	avail_dsds;
	uint32_t	*cur_dsd;
	scsi_qla_host_t *vha;
	struct scsi_cmnd *cmd;
#if !defined(__VMKLNX__)
	struct scatterlist *sg;
#endif
	cmd = sp->cmd;

	/* Update entry type to indicate Command Type 2 IOCB */
	*((uint32_t *)(&cmd_pkt->entry_type)) =
	    __constant_cpu_to_le32(COMMAND_TYPE);

	/* No data transfer */
	if (!scsi_bufflen(cmd) || cmd->sc_data_direction == DMA_NONE) {
		cmd_pkt->byte_count = __constant_cpu_to_le32(0);
		return;
	}

	vha = sp->fcport->vha;

	cmd_pkt->control_flags |= cpu_to_le16(qla2x00_get_cmd_direction(sp));

	/* Three DSDs are available in the Command Type 2 IOCB */
	avail_dsds = 3;
	cur_dsd = (uint32_t *)&cmd_pkt->dseg_0_address;

	/* Load data segments */
#if defined(__VMKLNX__)
	if (cmd->use_sg != 0) {
		struct  scatterlist *cur_seg;
		int i;

		cur_seg = (struct scatterlist *)cmd->request_buffer;
		for (i = 0; i < tot_dsds; i++) {
			cont_entry_t	*cont_pkt;

			/* Allocate additional continuation packets? */
			if (avail_dsds == 0) {
				/*
				 * Seven DSDs are available in the Continuation
				 * Type 0 IOCB.
				 */
				cont_pkt = qla2x00_prep_cont_type0_iocb(vha);
				cur_dsd = (uint32_t *)&cont_pkt->dseg_0_address;
				avail_dsds = 7;
			}

			*cur_dsd++ = cpu_to_le32(sg_dma_address(cur_seg));
			*cur_dsd++ = cpu_to_le32(sg_dma_len(cur_seg));
			avail_dsds--;

			cur_seg = sg_next(cur_seg);
		}
	} else {
		*cur_dsd++ = cpu_to_le32(sp->dma_handle);
		*cur_dsd++ = cpu_to_le32(cmd->request_bufflen);
	}
#else
	scsi_for_each_sg(cmd, sg, tot_dsds, i) {
		cont_entry_t *cont_pkt;

		/* Allocate additional continuation packets? */
		if (avail_dsds == 0) {
			/*
			 * Seven DSDs are available in the Continuation
			 * Type 0 IOCB.
			 */
			cont_pkt = qla2x00_prep_cont_type0_iocb(vha);
			cur_dsd = (uint32_t *)&cont_pkt->dseg_0_address;
			avail_dsds = 7;
		}

		*cur_dsd++ = cpu_to_le32(sg_dma_address(sg));
		*cur_dsd++ = cpu_to_le32(sg_dma_len(sg));
		avail_dsds--;
	}
#endif
}

/**
 * qla2x00_build_scsi_iocbs_64() - Build IOCB command utilizing 64bit
 * capable IOCB types.
 *
 * @sp: SRB command to process
 * @cmd_pkt: Command type 3 IOCB
 * @tot_dsds: Total number of segments to transfer
 */
void qla2x00_build_scsi_iocbs_64(srb_t *sp, cmd_entry_t *cmd_pkt,
    uint16_t tot_dsds)
{
	uint16_t	avail_dsds;
	uint32_t	*cur_dsd;
	scsi_qla_host_t	*vha;
	struct scsi_cmnd *cmd;

	cmd = sp->cmd;

	/* Update entry type to indicate Command Type 3 IOCB */
	*((uint32_t *)(&cmd_pkt->entry_type)) =
	    __constant_cpu_to_le32(COMMAND_A64_TYPE);

	/* No data transfer */
	if (!scsi_bufflen(cmd) || cmd->sc_data_direction == DMA_NONE) {
		cmd_pkt->byte_count = __constant_cpu_to_le32(0);
		return;
	}

	vha = sp->fcport->vha;

	cmd_pkt->control_flags |= cpu_to_le16(qla2x00_get_cmd_direction(sp));

	/* Two DSDs are available in the Command Type 3 IOCB */
	avail_dsds = 2;
	cur_dsd = (uint32_t *)&cmd_pkt->dseg_0_address;

	/* Load data segments */
#if defined(__VMKLNX__)
	/* Load data segments */
	if (cmd->use_sg != 0) {
		struct  scatterlist *cur_seg;
		int i;

		cur_seg = (struct scatterlist *)cmd->request_buffer;
		for (i = 0; i < tot_dsds; i++) {
			dma_addr_t	sle_dma;
			cont_a64_entry_t *cont_pkt;

			/* Allocate additional continuation packets? */
			if (avail_dsds == 0) {
				/*
				 * Five DSDs are available in the Continuation
				 * Type 1 IOCB.
				 */
				cont_pkt = qla2x00_prep_cont_type1_iocb(vha);
				cur_dsd = (uint32_t *)cont_pkt->dseg_0_address;
				avail_dsds = 5;
			}

			sle_dma = sg_dma_address(cur_seg);
			*cur_dsd++ = cpu_to_le32(LSD(sle_dma));
			*cur_dsd++ = cpu_to_le32(MSD(sle_dma));
			*cur_dsd++ = cpu_to_le32(sg_dma_len(cur_seg));
			avail_dsds--;

			cur_seg = sg_next(cur_seg);
		}
	} else {
		*cur_dsd++ = cpu_to_le32(LSD(sp->dma_handle));
		*cur_dsd++ = cpu_to_le32(MSD(sp->dma_handle));
		*cur_dsd++ = cpu_to_le32(cmd->request_bufflen);
	}
#else
	scsi_for_each_sg(cmd, sg, tot_dsds, i) {
		dma_addr_t	sle_dma;
		cont_a64_entry_t *cont_pkt;

		/* Allocate additional continuation packets? */
		if (avail_dsds == 0) {
			/*
			 * Five DSDs are available in the Continuation
			 * Type 1 IOCB.
			 */
			cont_pkt = qla2x00_prep_cont_type1_iocb(vha);
			cur_dsd = (uint32_t *)cont_pkt->dseg_0_address;
			avail_dsds = 5;
		}

		sle_dma = sg_dma_address(sg);
		*cur_dsd++ = cpu_to_le32(LSD(sle_dma));
		*cur_dsd++ = cpu_to_le32(MSD(sle_dma));
		*cur_dsd++ = cpu_to_le32(sg_dma_len(sg));
		avail_dsds--;
	}
#endif
}

/**
 * qla2x00_start_scsi() - Send a SCSI command to the ISP
 * @sp: command to send to the ISP
 *
 * Returns non-zero if a failure occured, else zero.
 */
int
qla2x00_start_scsi(srb_t *sp)
{
	int		ret;
#if !defined(__VMKLNX__)
	int		nseg;
#endif
	unsigned long   flags;
	scsi_qla_host_t	*vha;
	struct scsi_cmnd *cmd;
	uint32_t	*clr_ptr;
	uint32_t        index;
	uint32_t	handle;
	cmd_entry_t	*cmd_pkt;
	uint16_t	cnt;
	uint16_t	req_cnt;
	uint16_t	tot_dsds;
	struct device_reg_2xxx __iomem *reg;
	struct qla_hw_data *ha;
	struct req_que *req;
	struct rsp_que *rsp;
#if defined(__VMKLNX__)
	struct scatterlist *sg;
#endif
	uint32_t        timeout;

	/* Setup device pointers. */
	ret = 0;
	vha = sp->fcport->vha;
	ha = vha->hw;
	reg = &ha->iobase->isp;
	cmd = sp->cmd;
	req = ha->req_q_map[0];
	rsp = ha->rsp_q_map[0];
	/* So we know we haven't pci_map'ed anything yet */
	tot_dsds = 0;

	/* Send marker if required */
	if (vha->marker_needed != 0) {
		if (qla2x00_marker(vha, req, rsp, 0, 0, MK_SYNC_ALL) !=
			QLA_SUCCESS) {
			return (QLA_FUNCTION_FAILED);
		}
		vha->marker_needed = 0;
	}

	/* Acquire ring specific lock */
	spin_lock_irqsave(&ha->hardware_lock, flags);

	/* Check for room in outstanding command list. */
	handle = req->current_outstanding_cmd;
	for (index = 1; index < MAX_OUTSTANDING_COMMANDS; index++) {
		handle++;
		if (handle == MAX_OUTSTANDING_COMMANDS)
			handle = 1;
		if (!req->outstanding_cmds[handle])
			break;
	}
	if (index == MAX_OUTSTANDING_COMMANDS)
		goto queuing_error;

	/* Map the sg table so we have an accurate count of sg entries needed */
#if defined(__VMKLNX__)
	if (cmd->use_sg) {
		sg = (struct scatterlist *) cmd->request_buffer;
		tot_dsds = pci_map_sg(ha->pdev, sg, cmd->use_sg,
		    cmd->sc_data_direction);
		if (tot_dsds == 0)
			goto queuing_error;
	} else if (cmd->request_bufflen) {
		dma_addr_t	req_dma;

		req_dma = pci_map_single(ha->pdev, cmd->request_buffer,
		    cmd->request_bufflen, cmd->sc_data_direction);
		if (dma_mapping_error(req_dma))
			goto queuing_error;

		sp->dma_handle = req_dma;
		tot_dsds = 1;
	}
#else
	if (scsi_sg_count(cmd)) {
		nseg = dma_map_sg(&ha->pdev->dev, scsi_sglist(cmd),
		    scsi_sg_count(cmd), cmd->sc_data_direction);
		if (unlikely(!nseg))
			goto queuing_error;
	} else
		nseg = 0;

	tot_dsds = nseg;
#endif

	/* Calculate the number of request entries needed. */
	req_cnt = ha->isp_ops->calc_req_entries(tot_dsds);
	if (req->cnt < (req_cnt + 2)) {
		cnt = RD_REG_WORD_RELAXED(ISP_REQ_Q_OUT(ha, reg));
		if (req->ring_index < cnt)
			req->cnt = cnt - req->ring_index;
		else
			req->cnt = req->length -
			    (req->ring_index - cnt);
	}
	if (req->cnt < (req_cnt + 2))
		goto queuing_error;

	/* Build command packet */
	req->current_outstanding_cmd = handle;
	req->outstanding_cmds[handle] = sp;
	sp->cmd->host_scribble = (unsigned char *)(unsigned long)handle;
	req->cnt -= req_cnt;

	cmd_pkt = (cmd_entry_t *)req->ring_ptr;
	cmd_pkt->handle = handle;
	/* Zero out remaining portion of packet. */
	clr_ptr = (uint32_t *)cmd_pkt + 2;
	memset(clr_ptr, 0, REQUEST_ENTRY_SIZE - 8);
	cmd_pkt->dseg_count = cpu_to_le16(tot_dsds);

	/* Set target ID and LUN number*/
	SET_TARGET_ID(ha, cmd_pkt->target, sp->fcport->loop_id);
	cmd_pkt->lun = cpu_to_le16(sp->cmd->device->lun);

	/* Update tagged queuing modifier */
	cmd_pkt->control_flags = __constant_cpu_to_le16(CF_SIMPLE_TAG);

	/* Load SCSI command packet. */
	memcpy(cmd_pkt->scsi_cdb, cmd->cmnd, cmd->cmd_len);
	cmd_pkt->byte_count = cpu_to_le32((uint32_t)scsi_bufflen(cmd));

#if defined(__VMKLNX__)
	/*
	 * Make SCSI disk cmd timeout user-changeable during driver loading time
	 */
	if (!(sp->flags & SRB_IOCTL) && !(sp->fcport->flags & FCF_TAPE_PRESENT)) {
		timeout = ql2xcmdtimeout;
	} else
#endif
	timeout = (uint32_t) CMD_TIMEOUT(cmd)/HZ;
	if (sp->flags & SRB_NO_TIMER || timeout > FW_MAX_TIMEOUT)
		cmd_pkt->timeout = __constant_cpu_to_le16(0);
	else {
		if (sp->flags & SRB_IOCTL) {
			/*
	 		* Allocate at least 5 (+ QLA_CMD_TIMER_DELTA) seconds for RISC timeout.
	 		*/
			cmd_pkt->timeout = cpu_to_le16((uint16_t)timeout -
				    (5 + QLA_CMD_TIMER_DELTA));
		}
		else {
			cmd_pkt->timeout = cpu_to_le16((uint16_t)timeout);
		}
	}

	/* Build IOCB segments */
	ha->isp_ops->build_iocbs(sp, cmd_pkt, tot_dsds);

	/* Set total data segment count. */
	cmd_pkt->entry_count = (uint8_t)req_cnt;
	wmb();

	/* Adjust ring index. */
	req->ring_index++;
	if (req->ring_index == req->length) {
		req->ring_index = 0;
		req->ring_ptr = req->ring;
	} else
		req->ring_ptr++;

	/* Host Statistics. */
	ha->total_ios++;

	sp->flags |= SRB_DMA_VALID;

	/* Set chip new ring index. */
	WRT_REG_WORD(ISP_REQ_Q_IN(ha, reg), req->ring_index);
	RD_REG_WORD_RELAXED(ISP_REQ_Q_IN(ha, reg));	/* PCI Posting. */

	/* Manage unprocessed RIO/ZIO commands in response queue. */
	if (vha->flags.process_response_queue &&
	    rsp->ring_ptr->signature != RESPONSE_PROCESSED)
		qla2x00_process_response_queue(rsp);

	spin_unlock_irqrestore(&ha->hardware_lock, flags);
	return (QLA_SUCCESS);

queuing_error:
#if defined(__VMKLNX__)
	if (cmd->use_sg && tot_dsds) {
		sg = (struct scatterlist *) cmd->request_buffer;
		pci_unmap_sg(ha->pdev, sg, cmd->use_sg,
		    cmd->sc_data_direction);
	} else if (tot_dsds) {
		pci_unmap_single(ha->pdev, sp->dma_handle,
		    cmd->request_bufflen, cmd->sc_data_direction);
	}
#else
	if (tot_dsds)
		scsi_dma_unmap(cmd);
#endif

	spin_unlock_irqrestore(&ha->hardware_lock, flags);

	return (QLA_FUNCTION_FAILED);
}

/**
 * qla2x00_marker() - Send a marker IOCB to the firmware.
 * @ha: HA context
 * @loop_id: loop ID
 * @lun: LUN
 * @type: marker modifier
 *
 * Can be called from both normal and interrupt context.
 *
 * Returns non-zero if a failure occured, else zero.
 */
int
__qla2x00_marker(scsi_qla_host_t *vha, struct req_que *req, struct rsp_que *rsp,
			uint16_t loop_id, uint16_t lun, uint8_t type)
{
	mrk_entry_t *mrk;
	struct mrk_entry_24xx *mrk24;
	struct qla_hw_data *ha = vha->hw;
	scsi_qla_host_t *base_vha = pci_get_drvdata(ha->pdev);

	mrk24 = NULL;
	mrk = (mrk_entry_t *)qla2x00_req_pkt(vha, req, rsp);
	if (mrk == NULL) {
		DEBUG2_3(printk("%s(%ld): failed to allocate Marker IOCB.\n",
		    __func__, base_vha->host_no));

		return (QLA_FUNCTION_FAILED);
	}

	mrk->entry_type = MARKER_TYPE;
	mrk->modifier = type;
	if (type != MK_SYNC_ALL) {
		if (IS_FWI2_CAPABLE(ha)) {
			mrk24 = (struct mrk_entry_24xx *) mrk;
			mrk24->nport_handle = cpu_to_le16(loop_id);
			int_to_fcp_lun(lun, mrk24->lun);
			host_to_fcp_swap(mrk24->lun, sizeof(mrk24->lun));
			mrk24->vp_index = vha->vp_idx;
			mrk24->handle = MAKE_HANDLE(req->id, mrk24->handle);
		} else {
			SET_TARGET_ID(ha, mrk->target, loop_id);
			mrk->lun = cpu_to_le16(lun);
		}
	}
	wmb();

	qla2x00_isp_cmd(vha, req);

	return (QLA_SUCCESS);
}

int
qla2x00_marker(scsi_qla_host_t *vha, struct req_que *req, struct rsp_que *rsp,
		uint16_t loop_id, uint16_t lun, uint8_t type)
{
	int ret;
	unsigned long flags = 0;

	spin_lock_irqsave(&vha->hw->hardware_lock, flags);
	ret = __qla2x00_marker(vha, req, rsp, loop_id, lun, type);
	spin_unlock_irqrestore(&vha->hw->hardware_lock, flags);

	return (ret);
}

/**
 * qla2x00_req_pkt() - Retrieve a request packet from the request ring.
 * @ha: HA context
 *
 * Note: The caller must hold the hardware lock before calling this routine.
 *
 * Returns NULL if function failed, else, a pointer to the request packet.
 */
static request_t *
qla2x00_req_pkt(scsi_qla_host_t *vha, struct req_que *req, struct rsp_que *rsp)
{
	struct qla_hw_data *ha = vha->hw;
	device_reg_t __iomem *reg = ISP_QUE_REG(ha, req->id);
	request_t	*pkt = NULL;
	uint16_t	cnt;
	uint32_t	*dword_ptr;
	uint32_t	timer;
	uint16_t	req_cnt = 1;

	/* Wait 1 second for slot. */
	for (timer = HZ; timer; timer--) {
		if ((req_cnt + 2) >= req->cnt) {
			/* Calculate number of free request entries. */
			if (ha->mqenable)
				cnt = (uint16_t)
					RD_REG_DWORD(&reg->isp25mq.req_q_out);
			else {
				if (IS_FWI2_CAPABLE(ha))
					cnt = (uint16_t)RD_REG_DWORD(
				    	&reg->isp24.req_q_out);
				else
					cnt = qla2x00_debounce_register(
				    	ISP_REQ_Q_OUT(ha, &reg->isp));
			}
			if  (req->ring_index < cnt)
				req->cnt = cnt - req->ring_index;
			else
				req->cnt = req->length -
				    (req->ring_index - cnt);
		}
		/* If room for request in request ring. */
		if ((req_cnt + 2) < req->cnt) {
			req->cnt--;
			pkt = req->ring_ptr;

			/* Zero out packet. */
			dword_ptr = (uint32_t *)pkt;
			for (cnt = 0; cnt < REQUEST_ENTRY_SIZE / 4; cnt++)
				*dword_ptr++ = 0;

			/* Set system defined field. */
			pkt->sys_define = (uint8_t)req->ring_index;

			/* Set entry count. */
			pkt->entry_count = 1;

			break;
		}

		/* Release ring specific lock */
		spin_unlock(&ha->hardware_lock);

		udelay(2);   /* 2 us */

		/* Check for pending interrupts. */
		/* During init we issue marker directly */
		if (!vha->marker_needed && !vha->flags.init_done)
			qla2x00_poll(rsp);

		spin_lock_irq(&ha->hardware_lock);
	}
	if (!pkt) {
		DEBUG2_3(printk("%s(): **** FAILED ****\n", __func__));
	}

	return (pkt);
}

/**
 * qla2x00_isp_cmd() - Modify the request ring pointer.
 * @ha: HA context
 *
 * Note: The caller must hold the hardware lock before calling this routine.
 */
void
qla2x00_isp_cmd(scsi_qla_host_t *vha, struct req_que *req)
{
	struct qla_hw_data *ha = vha->hw;
	device_reg_t __iomem *reg = ISP_QUE_REG(ha, req->id);
	struct device_reg_2xxx __iomem *ioreg = &ha->iobase->isp;

	DEBUG5(printk("%s(): IOCB data:\n", __func__));
	DEBUG5(qla2x00_dump_buffer(
	    (uint8_t *)req->ring_ptr, REQUEST_ENTRY_SIZE));

	/* Adjust ring index. */
	req->ring_index++;
	if (req->ring_index == req->length) {
		req->ring_index = 0;
		req->ring_ptr = req->ring;
	} else
		req->ring_ptr++;

	/* Set chip new ring index. */
	if (ha->mqenable) {
		WRT_REG_DWORD(&reg->isp25mq.req_q_in, req->ring_index);
		RD_REG_DWORD(&ioreg->hccr);
	}
	else {
		if (IS_FWI2_CAPABLE(ha)) {
			WRT_REG_DWORD(&reg->isp24.req_q_in, req->ring_index);
			RD_REG_DWORD_RELAXED(&reg->isp24.req_q_in);
		} else {
			WRT_REG_WORD(ISP_REQ_Q_IN(ha, &reg->isp),
					req->ring_index);
			RD_REG_WORD_RELAXED(ISP_REQ_Q_IN(ha, &reg->isp));
		}
	}

}

/**
 * qla2x00_ms_req_pkt() - Retrieve a Management Server request packet from
 * 				the request ring.
 * @ha: HA context
 * @sp: pointer to handle post function call
 *
 * Note: The caller must hold the hardware lock before calling this routine.
 *
 * Returns NULL if function failed, else, a pointer to the request packet.
 */
request_t *
qla2x00_ms_req_pkt(scsi_qla_host_t *vha, struct req_que *req,
		struct rsp_que *rsp, srb_t  *sp)
{
	struct qla_hw_data *ha = vha->hw;
	struct device_reg_2xxx __iomem *reg = &ha->iobase->isp;
	struct device_reg_24xx __iomem *reg24 = &ha->iobase->isp24;
	request_t	*pkt = NULL;
	uint16_t	cnt, index;
	uint32_t	timer;
	uint8_t		found = 0;
	uint16_t	req_cnt = 1;

	/* Wait 1 second for slot. */
	for (timer = HZ; timer; timer--) {
		if ((req_cnt + 2) >= req->cnt) {
			/* Calculate number of free request entries. */
			if (IS_FWI2_CAPABLE(ha))
				cnt = (uint16_t)RD_REG_DWORD(&reg24->req_q_out);
			else
				cnt = qla2x00_debounce_register(
				    ISP_REQ_Q_OUT(ha, reg));

			if (req->ring_index < cnt) {
				req->cnt = cnt - req->ring_index;
			} else {
				req->cnt = req->length -
				    (req->ring_index - cnt);
			}
		}

		/* Check for room in outstanding command list. */
		cnt = req->current_outstanding_cmd;
		for (index = 1; index < MAX_OUTSTANDING_COMMANDS; index++) {
			cnt++;
			if (cnt == MAX_OUTSTANDING_COMMANDS)
				cnt = 1;

			if (req->outstanding_cmds[cnt] == 0) {
				found = 1;
				req->current_outstanding_cmd = cnt;
				break;
			}
		}

		/* If room for request in request ring. */
		if (found && (req_cnt + 2) < req->cnt) {
			pkt = req->ring_ptr;

			/* Zero out packet. */
			memset(pkt, 0, REQUEST_ENTRY_SIZE);

			DEBUG5(printk("%s(): putting sp=%p in "
			    "outstanding_cmds[%x]\n",
			    __func__,
			    sp, cnt));

			req->outstanding_cmds[cnt] = sp;

			/* save the handle */
			sp->cmd->host_scribble = (unsigned char *) (u_long) cnt;
			CMD_SP(sp->cmd) = (void *)sp;

			req->cnt--;
			pkt->handle = (uint32_t)cnt;

			/* Set system defined field. */
			pkt->sys_define = (uint8_t)req->ring_index;
			pkt->entry_status = 0;

			break;
		}

		/* Release ring specific lock */
		spin_unlock(&ha->hardware_lock);
		udelay(20);

		/* Check for pending interrupts. */
		qla2x00_poll(rsp);

		spin_lock_irq(&ha->hardware_lock);
	}
	if (!pkt) {
		DEBUG2_3(printk("%s(): **** FAILED ****\n", __func__));
	}

	return (pkt);
}

/**
 * qla24xx_calc_iocbs() - Determine number of Command Type 3 and
 * Continuation Type 1 IOCBs to allocate.
 *
 * @dsds: number of data segment decriptors needed
 *
 * Returns the number of IOCB entries needed to store @dsds.
 */
static inline uint16_t
qla24xx_calc_iocbs(uint16_t dsds)
{
	uint16_t iocbs;

	iocbs = 1;
	if (dsds > 1) {
		iocbs += (dsds - 1) / 5;
		if ((dsds - 1) % 5)
			iocbs++;
	}
	return iocbs;
}

/**
 * qla24xx_build_scsi_iocbs() - Build IOCB command utilizing Command Type 7
 * IOCB types.
 *
 * @sp: SRB command to process
 * @cmd_pkt: Command type 3 IOCB
 * @tot_dsds: Total number of segments to transfer
 */
static inline void
qla24xx_build_scsi_iocbs(srb_t *sp, struct cmd_type_7 *cmd_pkt,
    uint16_t tot_dsds)
{
	uint16_t	avail_dsds;
	uint32_t	*cur_dsd;
	scsi_qla_host_t	*vha;
	struct scsi_cmnd *cmd;
#if !defined(__VMKLNX__)
	struct scatterlist *sg;
#endif
        struct req_que *req;

	cmd = sp->cmd;

	/* Update entry type to indicate Command Type 3 IOCB */
	*((uint32_t *)(&cmd_pkt->entry_type)) =
	    __constant_cpu_to_le32(COMMAND_TYPE_7);

	/* No data transfer */
	if (!scsi_bufflen(cmd) || cmd->sc_data_direction == DMA_NONE) {
		cmd_pkt->byte_count = __constant_cpu_to_le32(0);
		return;
	}

	vha = sp->fcport->vha;
        req = vha->req;

	/* Set transfer direction */
	if (cmd->sc_data_direction == DMA_TO_DEVICE) {
		cmd_pkt->task_mgmt_flags =
		    __constant_cpu_to_le16(TMF_WRITE_DATA);
		sp->fcport->vha->hw->qla_stats.output_bytes +=
		    scsi_bufflen(sp->cmd);
	} else if (cmd->sc_data_direction == DMA_FROM_DEVICE) {
		cmd_pkt->task_mgmt_flags =
		    __constant_cpu_to_le16(TMF_READ_DATA);
		sp->fcport->vha->hw->qla_stats.input_bytes +=
		    scsi_bufflen(sp->cmd);
	}

	/* One DSD is available in the Command Type 3 IOCB */
	avail_dsds = 1;
	cur_dsd = (uint32_t *)&cmd_pkt->dseg_0_address;

	/* Load data segments */
#if defined(__VMKLNX__)
	if (cmd->use_sg != 0) {
		struct  scatterlist *cur_seg;
		int i;

		cur_seg = (struct scatterlist *)cmd->request_buffer;
		for (i = 0; i < tot_dsds; i++) {
			dma_addr_t	sle_dma;
			cont_a64_entry_t *cont_pkt;

			/* Allocate additional continuation packets? */
			if (avail_dsds == 0) {
				/*
				 * Five DSDs are available in the Continuation
				 * Type 1 IOCB.
				 */
				cont_pkt = qla2x00_prep_cont_type1_iocb(vha);
				cur_dsd = (uint32_t *)cont_pkt->dseg_0_address;
				avail_dsds = 5;
			}

			sle_dma = sg_dma_address(cur_seg);
			*cur_dsd++ = cpu_to_le32(LSD(sle_dma));
			*cur_dsd++ = cpu_to_le32(MSD(sle_dma));
			*cur_dsd++ = cpu_to_le32(sg_dma_len(cur_seg));
			avail_dsds--;

			cur_seg = sg_next(cur_seg);
		}
	} else {
		*cur_dsd++ = cpu_to_le32(LSD(sp->dma_handle));
		*cur_dsd++ = cpu_to_le32(MSD(sp->dma_handle));
		*cur_dsd++ = cpu_to_le32(cmd->request_bufflen);
	}
#else
	scsi_for_each_sg(cmd, sg, tot_dsds, i) {
		dma_addr_t	sle_dma;
		cont_a64_entry_t *cont_pkt;

		/* Allocate additional continuation packets? */
		if (avail_dsds == 0) {
			/*
			 * Five DSDs are available in the Continuation
			 * Type 1 IOCB.
			 */
			cont_pkt = qla2x00_prep_cont_type1_iocb(vha);
			cur_dsd = (uint32_t *)cont_pkt->dseg_0_address;
			avail_dsds = 5;
		}

		sle_dma = sg_dma_address(sg);
		*cur_dsd++ = cpu_to_le32(LSD(sle_dma));
		*cur_dsd++ = cpu_to_le32(MSD(sle_dma));
		*cur_dsd++ = cpu_to_le32(sg_dma_len(sg));
		avail_dsds--;
	}
#endif
}


/**
 * qla24xx_start_scsi() - Send a SCSI command to the ISP
 * @sp: command to send to the ISP
 *
 * Returns non-zero if a failure occured, else zero.
 */
int
qla24xx_start_scsi(srb_t *sp)
{
	int		ret;
#if !defined(__VMKLNX__)
	int		nseg;
#endif
	unsigned long   flags;
	uint32_t	*clr_ptr;
	uint32_t        index;
	uint32_t	handle;
	struct cmd_type_7 *cmd_pkt;
	uint16_t	cnt;
	uint16_t	req_cnt;
	uint16_t	tot_dsds;
	struct req_que *req = NULL;
	struct rsp_que *rsp = NULL;
	struct scsi_cmnd *cmd = sp->cmd;
	struct scsi_qla_host *vha = sp->fcport->vha;
	struct qla_hw_data *ha = vha->hw;
#if defined(__VMKLNX__)
    struct scatterlist *sg;
#endif
	uint32_t        timeout;

	/* Setup device pointers. */
	ret = 0;
	qla25xx_set_que(sp, &rsp);
	req = vha->req;
	/* So we know we haven't pci_map'ed anything yet */
	tot_dsds = 0;

	/* Send marker if required */
	if (vha->marker_needed != 0) {
		if (qla2x00_marker(vha, req, rsp, 0, 0, MK_SYNC_ALL) != QLA_SUCCESS)
			return QLA_FUNCTION_FAILED;
		vha->marker_needed = 0;
	}

	/* Acquire ring specific lock */
	spin_lock_irqsave(&ha->hardware_lock, flags);

	/* Check for room in outstanding command list. */
	handle = req->current_outstanding_cmd;
	for (index = 1; index < MAX_OUTSTANDING_COMMANDS; index++) {
		handle++;
		if (handle == MAX_OUTSTANDING_COMMANDS)
			handle = 1;
		if (!req->outstanding_cmds[handle])
			break;
	}
	if (index == MAX_OUTSTANDING_COMMANDS)
		goto queuing_error;

	/* Map the sg table so we have an accurate count of sg entries needed */
#if defined(__VMKLNX__)
	if (cmd->use_sg) {
		sg = (struct scatterlist *) cmd->request_buffer;
		tot_dsds = pci_map_sg(ha->pdev, sg, cmd->use_sg,
		    cmd->sc_data_direction);
		if (tot_dsds == 0)
			goto queuing_error;
	} else if (cmd->request_bufflen) {
		dma_addr_t      req_dma;

		req_dma = pci_map_single(ha->pdev, cmd->request_buffer,
		    cmd->request_bufflen, cmd->sc_data_direction);
		if (dma_mapping_error(req_dma))
			goto queuing_error;

		sp->dma_handle = req_dma;
		tot_dsds = 1;
	}
#else
	if (scsi_sg_count(cmd)) {
		nseg = dma_map_sg(&ha->pdev->dev, scsi_sglist(cmd),
		    scsi_sg_count(cmd), cmd->sc_data_direction);
		if (unlikely(!nseg))
			goto queuing_error;
	} else
		nseg = 0;

	tot_dsds = nseg;
#endif
	req_cnt = qla24xx_calc_iocbs(tot_dsds);
	if (req->cnt < (req_cnt + 2)) {
		cnt = RD_REG_DWORD_RELAXED(req->req_q_out);

		if (req->ring_index < cnt)
			req->cnt = cnt - req->ring_index;
		else
			req->cnt = req->length -
				(req->ring_index - cnt);
	}
	if (req->cnt < (req_cnt + 2))
		goto queuing_error;

	/* Build command packet. */
	req->current_outstanding_cmd = handle;
	req->outstanding_cmds[handle] = sp;
	sp->cmd->host_scribble = (unsigned char *)(unsigned long)handle;
	req->cnt -= req_cnt;

	cmd_pkt = (struct cmd_type_7 *)req->ring_ptr;
	cmd_pkt->handle = MAKE_HANDLE(req->id, handle);

	/* Zero out remaining portion of packet. */
	/*    tagged queuing modifier -- default is TSK_SIMPLE (0). */
	clr_ptr = (uint32_t *)cmd_pkt + 2;
	memset(clr_ptr, 0, REQUEST_ENTRY_SIZE - 8);
	cmd_pkt->dseg_count = cpu_to_le16(tot_dsds);

	/* Set NPORT-ID and LUN number*/
	cmd_pkt->nport_handle = cpu_to_le16(sp->fcport->loop_id);
	cmd_pkt->port_id[0] = sp->fcport->d_id.b.al_pa;
	cmd_pkt->port_id[1] = sp->fcport->d_id.b.area;
	cmd_pkt->port_id[2] = sp->fcport->d_id.b.domain;
	cmd_pkt->vp_index = sp->fcport->vp_idx;

	int_to_scsilun(sp->cmd->device->lun, &cmd_pkt->lun);
	host_to_fcp_swap((uint8_t *)&cmd_pkt->lun, sizeof(cmd_pkt->lun));

	/* Load SCSI command packet. */
	memcpy(cmd_pkt->fcp_cdb, cmd->cmnd, cmd->cmd_len);
	host_to_fcp_swap(cmd_pkt->fcp_cdb, sizeof(cmd_pkt->fcp_cdb));

	cmd_pkt->byte_count = cpu_to_le32((uint32_t)scsi_bufflen(cmd));

#if defined(__VMKLNX__)
	/*
	 * Make SCSI disk cmd timeout user-changeable during driver loading time
	 */
	if (!(sp->flags & SRB_IOCTL) && !(sp->fcport->flags & FCF_TAPE_PRESENT)) {
		timeout = ql2xcmdtimeout;
	} else
#endif
	timeout = (uint32_t) CMD_TIMEOUT(cmd)/HZ;
	if (sp->flags & SRB_NO_TIMER || timeout > FW_MAX_TIMEOUT)
		cmd_pkt->timeout = __constant_cpu_to_le16(0);
	else {
		if (sp->flags & SRB_IOCTL) {
			/*
	 		* Allocate at least 5 (+ QLA_CMD_TIMER_DELTA) seconds for RISC timeout.
	 		*/
			cmd_pkt->timeout = cpu_to_le16((uint16_t)timeout -
				    (5 + QLA_CMD_TIMER_DELTA));
		}
		else {
			cmd_pkt->timeout = cpu_to_le16((uint16_t)timeout);
		}
	}

	/* Build IOCB segments */
	qla24xx_build_scsi_iocbs(sp, cmd_pkt, tot_dsds);

	/* Set total data segment count. */
	cmd_pkt->entry_count = (uint8_t)req_cnt;
	/* Specify response queue number where completion should happen */
	cmd_pkt->entry_status = (uint8_t) rsp->id;
	wmb();

	/* Adjust ring index. */
	req->ring_index++;
	if (req->ring_index == req->length) {
		req->ring_index = 0;
		req->ring_ptr = req->ring;
	} else
		req->ring_ptr++;

	/* Host Statistics. */
	ha->total_ios++;

	sp->flags |= SRB_DMA_VALID;

	/* Set chip new ring index. */
	WRT_REG_DWORD(req->req_q_in, req->ring_index);
	RD_REG_DWORD_RELAXED(&ha->iobase->isp24.hccr);

	/* Manage unprocessed RIO/ZIO commands in response queue. */
	if (vha->flags.process_response_queue &&
		rsp->ring_ptr->signature != RESPONSE_PROCESSED)
		qla24xx_process_response_queue(vha, rsp);

	spin_unlock_irqrestore(&ha->hardware_lock, flags);
	return QLA_SUCCESS;

queuing_error:
#if defined(__VMKLNX__)
	if (cmd->use_sg && tot_dsds) {
		sg = (struct scatterlist *) cmd->request_buffer;
		pci_unmap_sg(ha->pdev, sg, cmd->use_sg,
		    cmd->sc_data_direction);
	} else if (tot_dsds) {
		pci_unmap_single(ha->pdev, sp->dma_handle,
		    cmd->request_bufflen, cmd->sc_data_direction);
	}
#else
	if (tot_dsds)
		scsi_dma_unmap(cmd);
#endif

	spin_unlock_irqrestore(&ha->hardware_lock, flags);

	return QLA_FUNCTION_FAILED;
}

static void qla25xx_set_que(srb_t *sp, struct rsp_que **rsp)
{
	struct scsi_cmnd *cmd = sp->cmd;
	scsi_qla_host_t *vha = shost_priv(cmd->device->host);
	struct qla_hw_data *ha = vha->hw;
	uint64_t affinity = 0;

	if (ha->flags.cpu_affinity_enabled && !(sp->flags & SRB_IOCTL)) {
		affinity = (uint64_t)vmklnx_scsi_get_cmd_ioqueue_handle(cmd, cmd->device->host); 
		if (affinity < 0 || affinity >= ha->max_rsp_queues - 1)
			affinity = 0;

		*rsp = ha->rsp_q_map[affinity + 1];
	}
	else
		*rsp = ha->rsp_q_map[0];
}