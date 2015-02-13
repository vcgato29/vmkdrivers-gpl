/* bnx2x_self_test.h: Broadcom Everest network driver.
 *
 * Copyright (c) 2007-2011 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 *
 * Maintained by: Eilon Greenstein <eilong@broadcom.com>
 * Written by: Yitchak Gertner <gertner@broadcom.com>
 */

/* self test */

static int errors;
static int idle_chk_warnings;


#define IDLE_CHK_E1			0x1
#define IDLE_CHK_E1H			0x2
#define IDLE_CHK_E2			0x4

#define IDLE_CHK_ERROR			1
#define IDLE_CHK_ERROR_NO_TRAFFIC	2
#define IDLE_CHK_WARNING		3


#define CHIP_MASK_CHK(chip_mask) \
			(((((chip_mask) & IDLE_CHK_E1) && is_e1) || \
			  (((chip_mask) & IDLE_CHK_E1H) && is_e1h) || \
			  (((chip_mask) & IDLE_CHK_E2) && is_e2)) ? 1 : 0)

#define CONDITION_CHK(condition, severity, fail_msg, arg_list...) \
	do { \
		if (condition) { \
			switch (severity) { \
			case IDLE_CHK_ERROR: \
				BNX2X_DBG_ERR("ERROR " fail_msg, ##arg_list); \
				errors++; \
				break; \
			case IDLE_CHK_ERROR_NO_TRAFFIC: \
				BNX2X_DBG_ERR("INFO " fail_msg, ##arg_list); \
				break; \
			case IDLE_CHK_WARNING: \
				BNX2X_DBG_ERR("WARNING " fail_msg, \
					      ##arg_list); \
				idle_chk_warnings++; \
				break; \
			} \
		} \
	} while (0);


static void bnx2x_idle_chk6(struct bnx2x *bp, u32 chip_mask, u32 offset,
			    int loop, int inc, int severity)
{
	int is_e1  = CHIP_IS_E1(bp);
	int is_e1h = CHIP_IS_E1H(bp);
	int is_e2  = CHIP_IS_E2(bp);
	int i;
	u32 val1, val2;
	u32 rd_ptr, wr_ptr, rd_bank, wr_bank;

	if (!CHIP_MASK_CHK(chip_mask))
		return;

	for (i = 0; i < loop; i++) {
		val1 = REG_RD(bp, offset + i*inc);
		val2 = REG_RD(bp, offset + i*inc + 4);
		rd_ptr = ((val1 & 0x3FFFFFC0) >> 6);
		wr_ptr = ((((val1 & 0xC0000000) >> 30) & 0x3) |
			  ((val2 & 0x3FFFFF) << 2));
		CONDITION_CHK((rd_ptr != wr_ptr), severity,
				"QM: PTRTBL entry %d - rd_ptr is not"
				" equal to wr_ptr. Values are 0x%x 0x%x\n",
				i, rd_ptr, wr_ptr);
		rd_bank = ((val1 & 0x30) >> 4);
		wr_bank = (val1 & 0x03);
		CONDITION_CHK((rd_bank != wr_bank), severity,
				"QM: PTRTBL entry %d - rd_bank is not"
				" equal to wr_bank. Values are 0x%x 0x%x\n",
				i, rd_bank, wr_bank);
	}
}

static int bnx2x_idle_chk(struct bnx2x *bp)
{

/* read one reg and check the condition */
#define IDLE_CHK_1(chip_mask, offset, condition, severity, fail_msg) \
	do { \
		if (CHIP_MASK_CHK(chip_mask)) { \
			val = REG_RD(bp, offset); \
			CONDITION_CHK(condition, severity, \
				      fail_msg ". Value is 0x%x\n", val); \
		} \
	} while (0);

/* loop to read one reg and check the condition */
#define IDLE_CHK_2(chip_mask, offset, loop, inc, condition, severity, \
		   fail_msg) \
	do { \
		if (CHIP_MASK_CHK(chip_mask)) \
			for (i = 0; i < (loop); i++) { \
				val = REG_RD(bp, offset + i*(inc)); \
				CONDITION_CHK(condition, severity, \
					      fail_msg ". Value is 0x%x in loop %d\n", \
					      val, i); \
			} \
	} while (0);

/* read two regs and check the condition */
#define IDLE_CHK_3(chip_mask, offset1, offset2, condition, severity, \
		   fail_msg) \
	do { \
		if (CHIP_MASK_CHK(chip_mask)) { \
			val1 = REG_RD(bp, offset1); \
			val2 = REG_RD(bp, offset2); \
			CONDITION_CHK(condition, severity, \
				      fail_msg ". Values are 0x%x 0x%x\n", \
				      val1, val2); \
		} \
	} while (0);

/* loop to read two regs and check the condition */
#define IDLE_CHK_4(chip_mask, offset1, offset2, loop, inc, condition, \
		   severity, fail_msg) \
	do { \
		if (CHIP_MASK_CHK(chip_mask)) \
			for (i = 0; i < (loop); i++) { \
				val1 = REG_RD(bp, offset1 + i*(inc)); \
				val2 = (REG_RD(bp, offset2 + i*(inc)) >> 1); \
				CONDITION_CHK(condition, severity, fail_msg \
					      " - LCID %d CID_CAM 0x%x" \
					      "  Value is 0x%x\n", \
					      i, val2, val1); \
			} \
	} while (0);

/* read one reg and check according to another reg */
#define IDLE_CHK_5(chip_mask, offset, offset1, offset2, condition, severity, \
		   fail_msg) \
	do { \
		if (CHIP_MASK_CHK(chip_mask)) \
			if (!REG_RD(bp, offset)) \
				IDLE_CHK_3(chip_mask, offset1, offset2, \
					   condition, severity, fail_msg); \
	} while (0);

/* read wide-bus reg and check sub-fields */
#define IDLE_CHK_6(chip_mask, offset, loop, inc, severity) \
	bnx2x_idle_chk6(bp, chip_mask, offset, loop, inc, severity)

/* loop to read wide-bus reg and check according to another reg */
#define IDLE_CHK_7(chip_mask, offset, offset1, offset2, loop, inc, condition, \
		   severity, fail_msg) \
	do { \
		if (CHIP_MASK_CHK(chip_mask)) \
			for (i = 0; i < (loop); i++) { \
				if (REG_RD(bp, offset + i*4) == 1) { \
					val1 = REG_RD(bp, offset1 + i*(inc)); \
					val1 = REG_RD(bp, offset1 + i*(inc) + \
						      4); \
					val1 = ((REG_RD(bp, offset1 + i*(inc) \
						 + 8) & 0x00000078) >> 3); \
					val2 = (REG_RD(bp, offset2 + i*4)>>1); \
					CONDITION_CHK(condition, severity, \
						      fail_msg " - LCID %d " \
						      "CID_CAM 0x%x" \
						      "  Value is 0x%x\n", \
						      i, val2, val1); \
				} \
			} \
	} while (0);

#define IDLE_CHK_8 IDLE_CHK_1


	int is_e1  = CHIP_IS_E1(bp);
	int is_e1h = CHIP_IS_E1H(bp);
	int is_e2  = CHIP_IS_E2(bp);
	int i;
	u32 val, val1, val2;

	errors = 0;
	idle_chk_warnings = 0;

	if (!netif_running(bp->dev))
		return errors;


	// Read register 0x2114 val and check if condition on val exist
	IDLE_CHK_1(0x3, 0x2114, ((val & 0x0FF010) != 0), IDLE_CHK_ERROR, "PCIE: ucorr_err_status is not 0");
	// Read register 0x2114 val and check if condition on val exist
	IDLE_CHK_1(0x3, 0x2114, ((val & 0x100000) != 0), IDLE_CHK_WARNING, "PCIE: ucorr_err_status - Unsupported request error");
	// Read register 0x2120 val and check if condition on val exist
	IDLE_CHK_1(0x3, 0x2120, (((val & 0x31C1) != 0x2000) && ((val & 0x31C1) != 0)), IDLE_CHK_WARNING, "PCIE: corr_err_status is not 0x2000");
	// Read register 0x2814 val and check if condition on val exist
	IDLE_CHK_1(0x3, 0x2814, ((val & ~0x40100) != 0), IDLE_CHK_ERROR, "PCIE: attentions register is not 0x40100");
	// Read register 0x281c val and check if condition on val exist
	IDLE_CHK_1(0x2, 0x281c, ((val & ~0x40040100) != 0), IDLE_CHK_ERROR, "PCIE: attentions register is not 0x40040100");
	// Read register 0x2820 val and check if condition on val exist
	IDLE_CHK_1(0x2, 0x2820, ((val & ~0x40040100) != 0), IDLE_CHK_ERROR, "PCIE: attentions register is not 0x40040100");
	// Read register PXP2_REG_PGL_EXP_ROM2 val and check if condition on val exist
	IDLE_CHK_1(0x3, PXP2_REG_PGL_EXP_ROM2, ((val != 0xffffffff)), IDLE_CHK_WARNING, "PXP2: There are outstanding read requests. Not all completios have arrived for read requests on tags that are marked with 0");
	// Read register 0x212c val in loop (incr by 4) and check if condition on val exist
	IDLE_CHK_2(0x3, 0x212c, 4, 4, ((val != 0) && (errors > 0)), IDLE_CHK_WARNING, "PCIE: error packet header is not 0");
	// Read register 0x2104 val and check if condition on val exist
	IDLE_CHK_1(0xC, 0x2104, ((val & 0x0FD010) != 0), IDLE_CHK_ERROR, "PCIE: ucorr_err_status is not 0");
	// Read register 0x2104 val and check if condition on val exist
	IDLE_CHK_1(0xC, 0x2104, ((val & 0x100000) != 0), IDLE_CHK_WARNING, "PCIE: ucorr_err_status - Unsupported request error");
	// Read register 0x2104 val and check if condition on val exist
	IDLE_CHK_1(0xC, 0x2104, ((val & 0x2000) != 0), IDLE_CHK_WARNING, "PCIE: ucorr_err_status - Flow Control Protocol Error");
	// Read register 0x2110 val and check if condition on val exist
	IDLE_CHK_1(0xC, 0x2110, (((val & 0x31C1) != 0x2000) && ((val & 0x31C1) != 0)), IDLE_CHK_WARNING, "PCIE: corr_err_status is not 0x2000");
	// Read register 0x2814 val and check if condition on val exist
	IDLE_CHK_1(0xC, 0x2814, ((val & 0x2000000) != 0), IDLE_CHK_WARNING, "PCIE: TTX_BRIDGE_FORWARD_ERR - Received master request while BME was 0.");
	// Read register 0x2814 val and check if condition on val exist
	IDLE_CHK_1(0xC, 0x2814, ((val & ~0x2040902) != 0), IDLE_CHK_ERROR, "PCIE: Func 0 1: attentions register is not 0x2040902");
	// Read register 0x2854 val and check if condition on val exist
	IDLE_CHK_1(0xC, 0x2854, ((val & ~0x10240902) != 0), IDLE_CHK_ERROR, "PCIE: Func 2 3 4: attentions register is not 0x10240902");
	// Read register 0x285c val and check if condition on val exist
	IDLE_CHK_1(0xC, 0x285c, ((val & ~0x10240902) != 0), IDLE_CHK_ERROR, "PCIE: Func 5 6 7: attentions register is not 0x10240902");
	// Read register PXP2_REG_PGL_EXP_ROM2 val and check if condition on val exist
	IDLE_CHK_1(0xC, PXP2_REG_PGL_EXP_ROM2, ((val != 0xffffffff)), IDLE_CHK_WARNING, "PXP2: There are outstanding read requests for tags 0-31. Not all completios have arrived for read requests on tags that are marked with 0");
	// Read register 0x211c val in loop (incr by 4) and check if condition on val exist
	IDLE_CHK_2(0xC, 0x211c, 4, 4, ((val != 0) && (errors > 0)), IDLE_CHK_WARNING, "PCIE: error packet header is not 0");
	// Read register PGLUE_B_REG_INCORRECT_RCV_DETAILS val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_INCORRECT_RCV_DETAILS, (val != 0), IDLE_CHK_ERROR, "PGLUE_B: Packet received from PCIe not according to the rules.");
	// Read register PGLUE_B_REG_WAS_ERROR_VF_31_0 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_WAS_ERROR_VF_31_0, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: was_error for VFs 0-31 is not 0");
	// Read register PGLUE_B_REG_WAS_ERROR_VF_63_32 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_WAS_ERROR_VF_63_32, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: was_error for VFs 32-63 is not 0");
	// Read register PGLUE_B_REG_WAS_ERROR_VF_95_64 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_WAS_ERROR_VF_95_64, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: was_error for VFs 64-95 is not 0");
	// Read register PGLUE_B_REG_WAS_ERROR_VF_127_96 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_WAS_ERROR_VF_127_96, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: was_error for VFs 96-127 is not 0");
	// Read register PGLUE_B_REG_WAS_ERROR_PF_7_0 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_WAS_ERROR_PF_7_0, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: was_error for PFs 0-7 is not 0");
	// Read register PGLUE_B_REG_TX_ERR_WR_ADD_31_0 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_TX_ERR_WR_ADD_31_0, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: Error in master write. Address(31:0) is not 0");
	// Read register PGLUE_B_REG_TX_ERR_WR_ADD_63_32 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_TX_ERR_WR_ADD_63_32, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: Error in master write. Address(63:32) is not 0");
	// Read register PGLUE_B_REG_TX_ERR_WR_DETAILS val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_TX_ERR_WR_DETAILS, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: Error in master write. Error details register is not 0. (4:0) VQID. (23:21) - PFID. (24) - VF_VALID. (30:25) - VFID.");
	// Read register PGLUE_B_REG_TX_ERR_WR_DETAILS2 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_TX_ERR_WR_DETAILS2, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: Error in master write. Error details 2nd register is not 0. (21) - was_error set; (22) - BME cleared; (23) - FID_enable cleared; (24) - VF with parent PF FLR_request or IOV_disable_request.");
	// Read register PGLUE_B_REG_TX_ERR_RD_ADD_31_0 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_TX_ERR_RD_ADD_31_0, (val != 0), IDLE_CHK_WARNING, "PGLUE: Error in master read address(31:0) is not 0");
	// Read register PGLUE_B_REG_TX_ERR_RD_ADD_63_32 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_TX_ERR_RD_ADD_63_32, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: Error in master read address(63:32) is not 0");
	// Read register PGLUE_B_REG_TX_ERR_RD_DETAILS val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_TX_ERR_RD_DETAILS, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: Error in master read Error details register is not 0. (4:0) VQID. (23:21) - PFID. (24) - VF_VALID. (30:25) - VFID.");
	// Read register PGLUE_B_REG_TX_ERR_RD_DETAILS2 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_TX_ERR_RD_DETAILS2, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: Error in master read Error details 2nd register is not 0. (21) - was_error set; (22) - BME cleared; (23) - FID_enable cleared; (24) - VF with parent PF FLR_request or IOV_disable_request.");
	// Read register PGLUE_B_REG_VF_LENGTH_VIOLATION_DETAILS val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_VF_LENGTH_VIOLATION_DETAILS, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: Target VF length violation access.");
	// Read register PGLUE_B_REG_VF_GRC_SPACE_VIOLATION_DETAILS val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_VF_GRC_SPACE_VIOLATION_DETAILS, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: Target VF GRC space access failed permission check.");
	// Read register PGLUE_B_REG_TAGS_63_32 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_TAGS_63_32, ((val != 0xffffffff)), IDLE_CHK_WARNING, "PGLUE_B: There are outstanding read requests for tags 32-63. Not all completios have arrived for read requests on tags that are marked with 0");
	// Read register PXP_REG_HST_VF_DISABLED_ERROR_VALID val1 and register PXP_REG_HST_VF_DISABLED_ERROR_DATA val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xC, PXP_REG_HST_VF_DISABLED_ERROR_VALID, PXP_REG_HST_VF_DISABLED_ERROR_DATA, (val1 != 0), IDLE_CHK_WARNING, "PXP: Access to disabled VF took place");
	// Read register PXP_REG_HST_PER_VIOLATION_VALID val and check if condition on val exist
	IDLE_CHK_1(0xC, PXP_REG_HST_PER_VIOLATION_VALID, (val != 0), IDLE_CHK_WARNING, "PXP: Zone A permission violation occurred");
	// Read register PXP_REG_HST_INCORRECT_ACCESS_VALID val and check if condition on val exist
	IDLE_CHK_1(0xC, PXP_REG_HST_INCORRECT_ACCESS_VALID, (val != 0), IDLE_CHK_WARNING, "PXP: Incorrect transaction took place");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ0_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ0_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ0 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ1_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ1_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ1 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ2_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ2_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ2 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ3_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ3_ENTRY_CNT, (val > 2), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ3 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ4_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ4_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ4 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ5_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ5_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ5 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ6_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ6_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ6 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ7_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ7_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ7 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ8_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ8_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ8 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ9_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ9_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ9 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ10_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ10_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ10 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ11_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ11_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ11 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ12_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ12_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ12 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ13_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ13_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ13 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ14_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ14_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ14 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ15_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ15_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ15 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ16_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ16_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ16 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ17_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ17_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ17 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ18_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ18_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ18 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ19_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ19_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ19 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ20_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ20_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ20 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ21_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ21_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ21 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ22_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ22_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ22 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ23_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ23_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ23 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ24_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ24_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ24 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ25_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ25_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ25 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ26_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ26_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ26 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ27_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ27_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ27 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ28_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ28_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ28 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ29_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ29_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ29 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ30_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ30_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ30 is not empty");
	// Check PXP VQ occupancy PXP2_REG_RQ_VQ31_ENTRY_CNT and check if condition on val exist
	IDLE_CHK_8(0xF, PXP2_REG_RQ_VQ31_ENTRY_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: VQ31 is not empty");
	// Read register PXP2_REG_RQ_UFIFO_NUM_OF_ENTRY val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_RQ_UFIFO_NUM_OF_ENTRY, (val != 0), IDLE_CHK_ERROR, "PXP2: rq_ufifo_num_of_entry is not 0");
	// Read register PXP2_REG_RQ_RBC_DONE val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_RQ_RBC_DONE, (val != 1), IDLE_CHK_ERROR, "PXP2: rq_rbc_done is not 1");
	// Read register PXP2_REG_RQ_CFG_DONE val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_RQ_CFG_DONE, (val != 1), IDLE_CHK_ERROR, "PXP2: rq_cfg_done is not 1");
	// Read register PXP2_REG_PSWRQ_BW_CREDIT val and check if condition on val exist
	IDLE_CHK_1(0x3, PXP2_REG_PSWRQ_BW_CREDIT, (val != 0x1B), IDLE_CHK_ERROR, "PXP2: rq_read_credit and rq_write_credit are not 3");
	// Read register PXP2_REG_RD_START_INIT val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_RD_START_INIT, (val != 1), IDLE_CHK_ERROR, "PXP2: rd_start_init is not 1");
	// Read register PXP2_REG_RD_INIT_DONE val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_RD_INIT_DONE, (val != 1), IDLE_CHK_ERROR, "PXP2: rd_init_done is not 1");
	// Read register PXP2_REG_RD_SR_CNT val1 and register PXP2_REG_RD_SR_NUM_CFG val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xF, PXP2_REG_RD_SR_CNT, PXP2_REG_RD_SR_NUM_CFG, (val1 != (val2-1)), IDLE_CHK_WARNING, "PXP2: rd_sr_cnt is not equal to rd_sr_num_cfg");
	// Read register PXP2_REG_RD_BLK_CNT val1 and register PXP2_REG_RD_BLK_NUM_CFG val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xF, PXP2_REG_RD_BLK_CNT, PXP2_REG_RD_BLK_NUM_CFG, (val1 != val2), IDLE_CHK_WARNING, "PXP2: rd_blk_cnt is not equal to rd_blk_num_cfg");
	// Read register PXP2_REG_RD_SR_CNT val1 and register PXP2_REG_RD_SR_NUM_CFG val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xF, PXP2_REG_RD_SR_CNT, PXP2_REG_RD_SR_NUM_CFG, (val1 < (val2-3)), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: There are more than two unused SRs");
	// Read register PXP2_REG_RD_BLK_CNT val1 and register PXP2_REG_RD_BLK_NUM_CFG val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xF, PXP2_REG_RD_BLK_CNT, PXP2_REG_RD_BLK_NUM_CFG, (val1 < (val2-2)), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: There are more than two unused blocks");
	// Read register PXP2_REG_RD_PORT_IS_IDLE_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_RD_PORT_IS_IDLE_0, (val != 1), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: P0 All delivery ports are not idle");
	// Read register PXP2_REG_RD_PORT_IS_IDLE_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_RD_PORT_IS_IDLE_1, (val != 1), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: P1 All delivery ports are not idle");
	// Read register PXP2_REG_RD_ALMOST_FULL_0 val in loop (incr by 4) and check if condition on val exist
	IDLE_CHK_2(0xF, PXP2_REG_RD_ALMOST_FULL_0, 11, 4, (val != 0), IDLE_CHK_ERROR, "PXP2: rd_almost_full is not 0");
	// Read register PXP2_REG_RD_DISABLE_INPUTS  val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_RD_DISABLE_INPUTS , (val != 0), IDLE_CHK_ERROR, "PXP2: PSWRD inputs are disabled");
	// Read register PXP2_REG_HST_HEADER_FIFO_STATUS val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_HST_HEADER_FIFO_STATUS, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: HST header FIFO status is not 0");
	// Read register PXP2_REG_HST_DATA_FIFO_STATUS val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_HST_DATA_FIFO_STATUS, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: HST data FIFO status is not 0");
	// Read register PXP2_REG_PGL_WRITE_BLOCKED val and check if condition on val exist
	IDLE_CHK_1(0x3, PXP2_REG_PGL_WRITE_BLOCKED, (val != 0), IDLE_CHK_ERROR, "PXP2: pgl_write_blocked is not 0");
	// Read register PXP2_REG_PGL_READ_BLOCKED val and check if condition on val exist
	IDLE_CHK_1(0x3, PXP2_REG_PGL_READ_BLOCKED, (val != 0), IDLE_CHK_ERROR, "PXP2: pgl_read_blocked is not 0");
	// Read register PXP2_REG_PGL_WRITE_BLOCKED val and check if condition on val exist
	IDLE_CHK_1(0xC, PXP2_REG_PGL_WRITE_BLOCKED, (val != 0), IDLE_CHK_WARNING, "PXP2: pgl_write_blocked is not 0");
	// Read register PXP2_REG_PGL_READ_BLOCKED val and check if condition on val exist
	IDLE_CHK_1(0xC, PXP2_REG_PGL_READ_BLOCKED, (val != 0), IDLE_CHK_WARNING, "PXP2: pgl_read_blocked is not 0");
	// Read register PXP2_REG_PGL_TXW_CDTS val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_PGL_TXW_CDTS, (((val >> 17) & 1) != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PXP2: There is data which is ready");
	// Read register PXP_REG_HST_ARB_IS_IDLE val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP_REG_HST_ARB_IS_IDLE, (val != 1), IDLE_CHK_WARNING, "PXP: HST arbiter is not idle");
	// Read register PXP_REG_HST_CLIENTS_WAITING_TO_ARB val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP_REG_HST_CLIENTS_WAITING_TO_ARB, (val != 0), IDLE_CHK_WARNING, "PXP: HST one of the clients is waiting for delivery");
	// Read register PXP_REG_HST_DISCARD_INTERNAL_WRITES_STATUS val and check if condition on val exist
	IDLE_CHK_1(0xE, PXP_REG_HST_DISCARD_INTERNAL_WRITES_STATUS, (val != 0), IDLE_CHK_WARNING, "PXP: HST Close the gates: Discarding internal writes");
	// Read register PXP_REG_HST_DISCARD_DOORBELLS_STATUS val and check if condition on val exist
	IDLE_CHK_1(0xE, PXP_REG_HST_DISCARD_DOORBELLS_STATUS, (val != 0), IDLE_CHK_WARNING, "PXP: HST Close the gates: Discarding doorbells");
	// Read register DMAE_REG_GO_C0 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C0, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 0 go is not 0");
	// Read register DMAE_REG_GO_C1 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C1, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 1 go is not 0");
	// Read register DMAE_REG_GO_C2 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C2, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 2 go is not 0");
	// Read register DMAE_REG_GO_C3 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C3, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 3 go is not 0");
	// Read register DMAE_REG_GO_C4 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C4, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 4 go is not 0");
	// Read register DMAE_REG_GO_C5 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C5, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 5 go is not 0");
	// Read register DMAE_REG_GO_C6 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C6, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 6 go is not 0");
	// Read register DMAE_REG_GO_C7 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C7, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 7 go is not 0");
	// Read register DMAE_REG_GO_C8 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C8, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 8 go is not 0");
	// Read register DMAE_REG_GO_C9 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C9, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 9 go is not 0");
	// Read register DMAE_REG_GO_C10 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C10, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 10 go is not 0");
	// Read register DMAE_REG_GO_C11 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C11, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 11 go is not 0");
	// Read register DMAE_REG_GO_C12 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C12, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 12 go is not 0");
	// Read register DMAE_REG_GO_C13 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C13, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 13 go is not 0");
	// Read register DMAE_REG_GO_C14 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C14, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 14 go is not 0");
	// Read register DMAE_REG_GO_C15 val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_GO_C15, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DMAE: command 15 go is not 0");
	// Read register CFC_REG_ERROR_VECTOR val and check if condition on val exist
	IDLE_CHK_1(0xF, CFC_REG_ERROR_VECTOR, (val != 0), IDLE_CHK_ERROR, "CFC: error vector is not 0");
	// Read register CFC_REG_NUM_LCIDS_ARRIVING val and check if condition on val exist
	IDLE_CHK_1(0xF, CFC_REG_NUM_LCIDS_ARRIVING, (val != 0), IDLE_CHK_ERROR, "CFC: number of arriving LCIDs is not 0");
	// Read register CFC_REG_NUM_LCIDS_ALLOC val and check if condition on val exist
	IDLE_CHK_1(0xF, CFC_REG_NUM_LCIDS_ALLOC, (val != 0), IDLE_CHK_ERROR, "CFC: number of alloc LCIDs is not 0");
	// Read register CFC_REG_NUM_LCIDS_LEAVING val and check if condition on val exist
	IDLE_CHK_1(0xF, CFC_REG_NUM_LCIDS_LEAVING, (val != 0), IDLE_CHK_ERROR, "CFC: number of leaving LCIDs is not 0");
	// Error if any AC (ACTIVITY COUNTERS) > 2 (regardless of connType)
	IDLE_CHK_4(0xF, CFC_REG_ACTIVITY_COUNTER, CFC_REG_CID_CAM, CFC_REG_ACTIVITY_COUNTER_SIZE >> 2, 4, (val1 > 2), IDLE_CHK_ERROR, "CFC: AC > 2");
	// Incase of AC (ACTIVITY COUNTERS) = 1, check if allowed on this connectionType (in Val1)
	IDLE_CHK_7(0xF, CFC_REG_ACTIVITY_COUNTER, CFC_REG_INFO_RAM, CFC_REG_CID_CAM, CFC_REG_INFO_RAM_SIZE >> 4, 16, (val1 == 0), IDLE_CHK_ERROR, "CFC: AC is 1 on connType 0 (ETH)");
	// Incase of AC (ACTIVITY COUNTERS) = 1, check if allowed on this connectionType (in Val1)
	IDLE_CHK_7(0xF, CFC_REG_ACTIVITY_COUNTER, CFC_REG_INFO_RAM, CFC_REG_CID_CAM, CFC_REG_INFO_RAM_SIZE >> 4, 16, (val1 == 3), IDLE_CHK_WARNING, "CFC: AC is 1 on connType 3 (iSCSI)");
	// Incase of AC (ACTIVITY COUNTERS) = 1, check if allowed on this connectionType (in Val1)
	IDLE_CHK_7(0xF, CFC_REG_ACTIVITY_COUNTER, CFC_REG_INFO_RAM, CFC_REG_CID_CAM, CFC_REG_INFO_RAM_SIZE >> 4, 16, ((val1 != 0) && (val1 != 3)), IDLE_CHK_ERROR, "CFC: AC is 1 on connType different than 0 (ETH) or 3 (ISCSI)");
	// Read register QM_REG_QTASKCTR_0 val in loop (incr by 4) and check if condition on val exist
	IDLE_CHK_2(0xF, QM_REG_QTASKCTR_0, 64, 4, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "QM: Queue is not empty");
	// Read register QM_REG_VOQCREDIT_0 val1 and register QM_REG_VOQINITCREDIT_0 val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xF, QM_REG_VOQCREDIT_0, QM_REG_VOQINITCREDIT_0, (val1 != val2), IDLE_CHK_ERROR_NO_TRAFFIC, "QM: VOQ_0, VOQ credit is not equal to initial credit");
	// Read register QM_REG_VOQCREDIT_1 val1 and register QM_REG_VOQINITCREDIT_1 val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xF, QM_REG_VOQCREDIT_1, QM_REG_VOQINITCREDIT_1, (val1 != val2), IDLE_CHK_ERROR_NO_TRAFFIC, "QM: VOQ_1, VOQ credit is not equal to initial credit");
	// Read register QM_REG_VOQCREDIT_4 val1 and register QM_REG_VOQINITCREDIT_4 val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xF, QM_REG_VOQCREDIT_4, QM_REG_VOQINITCREDIT_4, (val1 != val2), IDLE_CHK_ERROR, "QM: VOQ_4, VOQ credit is not equal to initial credit");
	// Read register QM_REG_PORT0BYTECRD val1 and register QM_REG_BYTECRDINITVAL val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0x3, QM_REG_PORT0BYTECRD, QM_REG_BYTECRDINITVAL, (val1 != val2), IDLE_CHK_ERROR_NO_TRAFFIC, "QM: P0 Byte credit is not equal to initial credit");
	// Read register QM_REG_PORT1BYTECRD val1 and register QM_REG_BYTECRDINITVAL val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0x3, QM_REG_PORT1BYTECRD, QM_REG_BYTECRDINITVAL, (val1 != val2), IDLE_CHK_ERROR_NO_TRAFFIC, "QM: P1 Byte credit is not equal to initial credit");
	// Read register CCM_REG_CAM_OCCUP val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_CAM_OCCUP, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "CCM: XX protection CAM is not empty");
	// Read register TCM_REG_CAM_OCCUP val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_CAM_OCCUP, (val != 0), IDLE_CHK_ERROR, "TCM: XX protection CAM is not empty");
	// Read register UCM_REG_CAM_OCCUP val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_CAM_OCCUP, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "UCM: XX protection CAM is not empty");
	// Read register XCM_REG_CAM_OCCUP val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_CAM_OCCUP, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "XCM: XX protection CAM is not empty");
	// Read register BRB1_REG_NUM_OF_FULL_BLOCKS val and check if condition on val exist
	IDLE_CHK_1(0xF, BRB1_REG_NUM_OF_FULL_BLOCKS, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "BRB1: BRB is not empty");
	// Read register CSEM_REG_SLEEP_THREADS_VALID val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_SLEEP_THREADS_VALID, (val != 0), IDLE_CHK_ERROR, "CSEM: There are sleeping threads");
	// Read register TSEM_REG_SLEEP_THREADS_VALID val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_SLEEP_THREADS_VALID, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "TSEM: There are sleeping threads");
	// Read register USEM_REG_SLEEP_THREADS_VALID val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_SLEEP_THREADS_VALID, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "USEM: There are sleeping threads");
	// Read register XSEM_REG_SLEEP_THREADS_VALID val and check if condition on val exist
	IDLE_CHK_1(0xF, XSEM_REG_SLEEP_THREADS_VALID, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "XSEM: There are sleeping threads");
	// Read register CSEM_REG_SLOW_EXT_STORE_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_SLOW_EXT_STORE_EMPTY, (val != 1), IDLE_CHK_ERROR, "CSEM: External store FIFO is not empty");
	// Read register TSEM_REG_SLOW_EXT_STORE_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_SLOW_EXT_STORE_EMPTY, (val != 1), IDLE_CHK_ERROR_NO_TRAFFIC, "TSEM: External store FIFO is not empty");
	// Read register USEM_REG_SLOW_EXT_STORE_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_SLOW_EXT_STORE_EMPTY, (val != 1), IDLE_CHK_ERROR_NO_TRAFFIC, "USEM: External store FIFO is not empty");
	// Read register XSEM_REG_SLOW_EXT_STORE_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, XSEM_REG_SLOW_EXT_STORE_EMPTY, (val != 1), IDLE_CHK_ERROR_NO_TRAFFIC, "XSEM: External store FIFO is not empty");
	// Read register CSDM_REG_SYNC_PARSER_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, CSDM_REG_SYNC_PARSER_EMPTY, (val != 1), IDLE_CHK_ERROR, "CSDM: Parser serial FIFO is not empty");
	// Read register TSDM_REG_SYNC_PARSER_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, TSDM_REG_SYNC_PARSER_EMPTY, (val != 1), IDLE_CHK_ERROR_NO_TRAFFIC, "TSDM: Parser serial FIFO is not empty");
	// Read register USDM_REG_SYNC_PARSER_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, USDM_REG_SYNC_PARSER_EMPTY, (val != 1), IDLE_CHK_ERROR, "USDM: Parser serial FIFO is not empty");
	// Read register XSDM_REG_SYNC_PARSER_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, XSDM_REG_SYNC_PARSER_EMPTY, (val != 1), IDLE_CHK_ERROR, "XSDM: Parser serial FIFO is not empty");
	// Read register CSDM_REG_SYNC_SYNC_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, CSDM_REG_SYNC_SYNC_EMPTY, (val != 1), IDLE_CHK_ERROR, "CSDM: Parser SYNC serial FIFO is not empty");
	// Read register TSDM_REG_SYNC_SYNC_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, TSDM_REG_SYNC_SYNC_EMPTY, (val != 1), IDLE_CHK_ERROR, "TSDM: Parser SYNC serial FIFO is not empty");
	// Read register USDM_REG_SYNC_SYNC_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, USDM_REG_SYNC_SYNC_EMPTY, (val != 1), IDLE_CHK_ERROR, "USDM: Parser SYNC serial FIFO is not empty");
	// Read register XSDM_REG_SYNC_SYNC_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, XSDM_REG_SYNC_SYNC_EMPTY, (val != 1), IDLE_CHK_ERROR, "XSDM: Parser SYNC serial FIFO is not empty");
	// Read register CSDM_REG_RSP_PXP_CTRL_RDATA_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, CSDM_REG_RSP_PXP_CTRL_RDATA_EMPTY, (val != 1), IDLE_CHK_ERROR, "CSDM: pxp_ctrl rd_data fifo is not empty in sdm_dma_rsp block");
	// Read register TSDM_REG_RSP_PXP_CTRL_RDATA_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, TSDM_REG_RSP_PXP_CTRL_RDATA_EMPTY, (val != 1), IDLE_CHK_ERROR, "TSDM: pxp_ctrl rd_data fifo is not empty in sdm_dma_rsp block");
	// Read register USDM_REG_RSP_PXP_CTRL_RDATA_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, USDM_REG_RSP_PXP_CTRL_RDATA_EMPTY, (val != 1), IDLE_CHK_ERROR_NO_TRAFFIC, "USDM: pxp_ctrl rd_data fifo is not empty in sdm_dma_rsp block");
	// Read register XSDM_REG_RSP_PXP_CTRL_RDATA_EMPTY val and check if condition on val exist
	IDLE_CHK_1(0xF, XSDM_REG_RSP_PXP_CTRL_RDATA_EMPTY, (val != 1), IDLE_CHK_ERROR_NO_TRAFFIC, "XSDM: pxp_ctrl rd_data fifo is not empty in sdm_dma_rsp block");
	// Read register DORQ_REG_DQ_FILL_LVLF val and check if condition on val exist
	IDLE_CHK_1(0xF, DORQ_REG_DQ_FILL_LVLF, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "DORQ: DORQ queue is not empty");
	// Read register CFC_REG_CFC_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, CFC_REG_CFC_INT_STS, (val != 0), IDLE_CHK_ERROR, "CFC: Interrupt status is not 0");
	// Read register CDU_REG_CDU_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, CDU_REG_CDU_INT_STS, (val != 0), IDLE_CHK_ERROR, "CDU: Interrupt status is not 0");
	// Read register CCM_REG_CCM_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_CCM_INT_STS, (val != 0), IDLE_CHK_ERROR, "CCM: Interrupt status is not 0");
	// Read register TCM_REG_TCM_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_TCM_INT_STS, (val != 0), IDLE_CHK_ERROR, "TCM: Interrupt status is not 0");
	// Read register UCM_REG_UCM_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_UCM_INT_STS, (val != 0), IDLE_CHK_ERROR, "UCM: Interrupt status is not 0");
	// Read register XCM_REG_XCM_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_XCM_INT_STS, (val != 0), IDLE_CHK_ERROR, "XCM: Interrupt status is not 0");
	// Read register PBF_REG_PBF_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, PBF_REG_PBF_INT_STS, (val != 0), IDLE_CHK_ERROR, "PBF: Interrupt status is not 0");
	// Read register TM_REG_TM_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, TM_REG_TM_INT_STS, (val != 0), IDLE_CHK_ERROR, "TIMERS: Interrupt status is not 0");
	// Read register DORQ_REG_DORQ_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, DORQ_REG_DORQ_INT_STS, (val != 0), IDLE_CHK_ERROR, "DORQ: Interrupt status is not 0");
	// Read register SRC_REG_SRC_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, SRC_REG_SRC_INT_STS, (val != 0), IDLE_CHK_ERROR, "SRCH: Interrupt status is not 0");
	// Read register PRS_REG_PRS_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, PRS_REG_PRS_INT_STS, (val != 0), IDLE_CHK_ERROR, "PRS: Interrupt status is not 0");
	// Read register BRB1_REG_BRB1_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, BRB1_REG_BRB1_INT_STS, ((val & ~0xFC00) != 0), IDLE_CHK_ERROR, "BRB1: Interrupt status is not 0");
	// Read register GRCBASE_XPB + PB_REG_PB_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, GRCBASE_XPB + PB_REG_PB_INT_STS, (val != 0), IDLE_CHK_ERROR, "XPB: Interrupt status is not 0");
	// Read register GRCBASE_UPB + PB_REG_PB_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, GRCBASE_UPB + PB_REG_PB_INT_STS, (val != 0), IDLE_CHK_ERROR, "UPB: Interrupt status is not 0");
	// Read register PXP2_REG_PXP2_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP2_REG_PXP2_INT_STS_0, (val != 0), IDLE_CHK_WARNING, "PXP2: Interrupt status 0 is not 0");
	// Read register PXP2_REG_PXP2_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xE, PXP2_REG_PXP2_INT_STS_1, (val != 0), IDLE_CHK_WARNING, "PXP2: Interrupt status 1 is not 0");
	// Read register QM_REG_QM_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_QM_INT_STS, (val != 0), IDLE_CHK_ERROR, "QM: Interrupt status is not 0");
	// Read register PXP_REG_PXP_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP_REG_PXP_INT_STS_0, (val != 0), IDLE_CHK_WARNING, "PXP: P0 Interrupt status is not 0");
	// Read register PXP_REG_PXP_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP_REG_PXP_INT_STS_1, (val != 0), IDLE_CHK_WARNING, "PXP: P1 Interrupt status is not 0");
	// Read register PGLUE_B_REG_PGLUE_B_INT_STS val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_PGLUE_B_INT_STS, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: Interrupt status is not 0");
	// Read register DORQ_REG_RSPA_CRD_CNT val and check if condition on val exist
	IDLE_CHK_1(0xF, DORQ_REG_RSPA_CRD_CNT, (val != 2), IDLE_CHK_ERROR_NO_TRAFFIC, "DORQ: Credit to XCM is not full");
	// Read register DORQ_REG_RSPB_CRD_CNT val and check if condition on val exist
	IDLE_CHK_1(0xF, DORQ_REG_RSPB_CRD_CNT, (val != 2), IDLE_CHK_ERROR, "DORQ: Credit to UCM is not full");
	// Read register QM_REG_VOQCRDERRREG val and check if condition on val exist
	IDLE_CHK_1(0x3, QM_REG_VOQCRDERRREG, (val != 0), IDLE_CHK_ERROR, "QM: Credit error register is not 0 (byte or credit overflow/underflow)");
	// Read register DORQ_REG_DQ_FULL_ST val and check if condition on val exist
	IDLE_CHK_1(0xF, DORQ_REG_DQ_FULL_ST, (val != 0), IDLE_CHK_ERROR, "DORQ: DORQ queue is full");
	// Read register MISC_REG_AEU_AFTER_INVERT_1_FUNC_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, MISC_REG_AEU_AFTER_INVERT_1_FUNC_0, ((val & ~0xCFFC) != 0), IDLE_CHK_WARNING, "AEU: P0 AFTER_INVERT_1 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_2_FUNC_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, MISC_REG_AEU_AFTER_INVERT_2_FUNC_0, (val != 0), IDLE_CHK_ERROR, "AEU: P0 AFTER_INVERT_2 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_3_FUNC_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, MISC_REG_AEU_AFTER_INVERT_3_FUNC_0, ((val & ~0xC21B0000) != 0), IDLE_CHK_ERROR, "AEU: P0 AFTER_INVERT_3 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_4_FUNC_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, MISC_REG_AEU_AFTER_INVERT_4_FUNC_0, ((val & ~0x801FFFFF) != 0), IDLE_CHK_ERROR, "AEU: P0 AFTER_INVERT_4 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_1_FUNC_1 val and check if condition on val exist
	IDLE_CHK_1(0x3, MISC_REG_AEU_AFTER_INVERT_1_FUNC_1, ((val & ~0xCFFC) != 0), IDLE_CHK_WARNING, "AEU: P1 AFTER_INVERT_1 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_2_FUNC_1 val and check if condition on val exist
	IDLE_CHK_1(0x3, MISC_REG_AEU_AFTER_INVERT_2_FUNC_1, (val != 0), IDLE_CHK_ERROR, "AEU: P1 AFTER_INVERT_2 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_3_FUNC_1 val and check if condition on val exist
	IDLE_CHK_1(0x3, MISC_REG_AEU_AFTER_INVERT_3_FUNC_1, ((val & ~0xC21B0000) != 0), IDLE_CHK_ERROR, "AEU: P1 AFTER_INVERT_3 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_4_FUNC_1 val and check if condition on val exist
	IDLE_CHK_1(0x3, MISC_REG_AEU_AFTER_INVERT_4_FUNC_1, ((val & ~0x801FFFFF) != 0), IDLE_CHK_ERROR, "AEU: P1 AFTER_INVERT_4 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_1_MCP val and check if condition on val exist
	IDLE_CHK_1(0xF, MISC_REG_AEU_AFTER_INVERT_1_MCP, ((val & ~0xCFFC) != 0), IDLE_CHK_WARNING, "AEU: MCP AFTER_INVERT_1 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_2_MCP val and check if condition on val exist
	IDLE_CHK_1(0xF, MISC_REG_AEU_AFTER_INVERT_2_MCP, (val != 0), IDLE_CHK_ERROR, "AEU: MCP AFTER_INVERT_2 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_3_MCP val and check if condition on val exist
	IDLE_CHK_1(0xF, MISC_REG_AEU_AFTER_INVERT_3_MCP, ((val & ~0xC21B0000) != 0), IDLE_CHK_ERROR, "AEU: MCP AFTER_INVERT_3 is not 0");
	// Read register MISC_REG_AEU_AFTER_INVERT_4_MCP val and check if condition on val exist
	IDLE_CHK_1(0xF, MISC_REG_AEU_AFTER_INVERT_4_MCP, ((val & ~0x801FFFFF) != 0), IDLE_CHK_ERROR, "AEU: MCP AFTER_INVERT_4 is not 0");
	// If register PBF_REG_DISABLE_NEW_TASK_PROC_P0 is valid, read PBF_REG_P0_CREDIT val1 and register PBF_REG_P0_INIT_CRD val2 and check if codition on val1,val2 exist
	IDLE_CHK_5(0xF, PBF_REG_DISABLE_NEW_TASK_PROC_P0, PBF_REG_P0_CREDIT, PBF_REG_P0_INIT_CRD, (val1 != val2), IDLE_CHK_ERROR_NO_TRAFFIC, "PBF: P0 credit is not equal to init_crd");
	// If register PBF_REG_DISABLE_NEW_TASK_PROC_P1 is valid, read PBF_REG_P1_CREDIT val1 and register PBF_REG_P1_INIT_CRD val2 and check if codition on val1,val2 exist
	IDLE_CHK_5(0xF, PBF_REG_DISABLE_NEW_TASK_PROC_P1, PBF_REG_P1_CREDIT, PBF_REG_P1_INIT_CRD, (val1 != val2), IDLE_CHK_ERROR_NO_TRAFFIC, "PBF: P1 credit is not equal to init_crd");
	// Read register PBF_REG_P4_CREDIT val1 and register PBF_REG_P4_INIT_CRD val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xF, PBF_REG_P4_CREDIT, PBF_REG_P4_INIT_CRD, (val1 != val2), IDLE_CHK_ERROR, "PBF: P4 credit is not equal to init_crd");
	// Read register PBF_REG_P0_TASK_CNT val and check if condition on val exist
	IDLE_CHK_1(0xF, PBF_REG_P0_TASK_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PBF: P0 task_cnt is not 0");
	// Read register PBF_REG_P1_TASK_CNT val and check if condition on val exist
	IDLE_CHK_1(0xF, PBF_REG_P1_TASK_CNT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PBF: P1 task_cnt is not 0");
	// Read register PBF_REG_P4_TASK_CNT val and check if condition on val exist
	IDLE_CHK_1(0xF, PBF_REG_P4_TASK_CNT, (val != 0), IDLE_CHK_ERROR, "PBF: P4 task_cnt is not 0");
	// Read register XCM_REG_CFC_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_CFC_INIT_CRD, (val != 1), IDLE_CHK_ERROR, "XCM: CFC_INIT_CRD is not 1");
	// Read register UCM_REG_CFC_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_CFC_INIT_CRD, (val != 1), IDLE_CHK_ERROR, "UCM: CFC_INIT_CRD is not 1");
	// Read register TCM_REG_CFC_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_CFC_INIT_CRD, (val != 1), IDLE_CHK_ERROR, "TCM: CFC_INIT_CRD is not 1");
	// Read register CCM_REG_CFC_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_CFC_INIT_CRD, (val != 1), IDLE_CHK_ERROR_NO_TRAFFIC, "CCM: CFC_INIT_CRD is not 1");
	// Read register XCM_REG_XQM_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_XQM_INIT_CRD, (val != 32), IDLE_CHK_ERROR, "XCM: XQM_INIT_CRD is not 32");
	// Read register UCM_REG_UQM_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_UQM_INIT_CRD, (val != 32), IDLE_CHK_ERROR, "UCM: UQM_INIT_CRD is not 32");
	// Read register TCM_REG_TQM_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_TQM_INIT_CRD, (val != 32), IDLE_CHK_ERROR, "TCM: TQM_INIT_CRD is not 32");
	// Read register CCM_REG_CQM_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_CQM_INIT_CRD, (val != 32), IDLE_CHK_ERROR_NO_TRAFFIC, "CCM: CQM_INIT_CRD is not 32");
	// Read register XCM_REG_TM_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_TM_INIT_CRD, (val != 4), IDLE_CHK_ERROR, "XCM: TM_INIT_CRD is not 4");
	// Read register UCM_REG_TM_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_TM_INIT_CRD, (val != 4), IDLE_CHK_ERROR, "UCM: TM_INIT_CRD is not 4");
	// Read register XCM_REG_FIC0_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_FIC0_INIT_CRD, (val != 64), IDLE_CHK_WARNING, "XCM: FIC0_INIT_CRD is not 64");
	// Read register UCM_REG_FIC0_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_FIC0_INIT_CRD, (val != 64), IDLE_CHK_ERROR_NO_TRAFFIC, "UCM: FIC0_INIT_CRD is not 64");
	// Read register TCM_REG_FIC0_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_FIC0_INIT_CRD, (val != 64), IDLE_CHK_ERROR_NO_TRAFFIC, "TCM: FIC0_INIT_CRD is not 64");
	// Read register CCM_REG_FIC0_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_FIC0_INIT_CRD, (val != 64), IDLE_CHK_ERROR_NO_TRAFFIC, "CCM: FIC0_INIT_CRD is not 64");
	// Read register XCM_REG_FIC1_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_FIC1_INIT_CRD, (val != 64), IDLE_CHK_ERROR, "XCM: FIC1_INIT_CRD is not 64");
	// Read register UCM_REG_FIC1_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_FIC1_INIT_CRD, (val != 64), IDLE_CHK_ERROR, "UCM: FIC1_INIT_CRD is not 64");
	// Read register TCM_REG_FIC1_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_FIC1_INIT_CRD, (val != 64), IDLE_CHK_ERROR, "TCM: FIC1_INIT_CRD is not 64");
	// Read register CCM_REG_FIC1_INIT_CRD val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_FIC1_INIT_CRD, (val != 64), IDLE_CHK_ERROR, "CCM: FIC1_INIT_CRD is not 64");
	// Read register XCM_REG_XX_FREE val and check if condition on val exist
	IDLE_CHK_1(0x1, XCM_REG_XX_FREE, (val != 31), IDLE_CHK_ERROR, "XCM: XX_FREE differs from expected 31");
	// Read register XCM_REG_XX_FREE val and check if condition on val exist
	IDLE_CHK_1(0xE, XCM_REG_XX_FREE, (val != 32), IDLE_CHK_ERROR, "XCM: XX_FREE differs from expected 32");
	// Read register UCM_REG_XX_FREE val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_XX_FREE, (val != 27), IDLE_CHK_ERROR_NO_TRAFFIC, "UCM: XX_FREE differs from expected 27");
	// Read register TCM_REG_XX_FREE val and check if condition on val exist
	IDLE_CHK_1(0x7, TCM_REG_XX_FREE, (val != 32), IDLE_CHK_ERROR, "TCM: XX_FREE differs from expected 32");
	// Read register TCM_REG_XX_FREE val and check if condition on val exist
	IDLE_CHK_1(0x8, TCM_REG_XX_FREE, (val != 29), IDLE_CHK_ERROR, "TCM: XX_FREE differs from expected 29");
	// Read register CCM_REG_XX_FREE val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_XX_FREE, (val != 24), IDLE_CHK_ERROR, "CCM: XX_FREE differs from expected 24");
	// Read register XSEM_REG_FAST_MEMORY + 0x18000 val and check if condition on val exist
	IDLE_CHK_1(0xF, XSEM_REG_FAST_MEMORY + 0x18000, (val !=  0), IDLE_CHK_ERROR_NO_TRAFFIC, "XSEM: FOC0 credit less than initial credit");
	// Read register XSEM_REG_FAST_MEMORY + 0x18040 val and check if condition on val exist
	IDLE_CHK_1(0xF, XSEM_REG_FAST_MEMORY + 0x18040, (val !=  24), IDLE_CHK_ERROR_NO_TRAFFIC, "XSEM: FOC1 credit less than initial credit");
	// Read register XSEM_REG_FAST_MEMORY + 0x18080 val and check if condition on val exist
	IDLE_CHK_1(0xF, XSEM_REG_FAST_MEMORY + 0x18080, (val !=  12), IDLE_CHK_ERROR_NO_TRAFFIC, "XSEM: FOC2 credit less than initial credit");
	// Read register USEM_REG_FAST_MEMORY + 0x18000 val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_FAST_MEMORY + 0x18000, (val != 26), IDLE_CHK_ERROR_NO_TRAFFIC, "USEM: FOC0 credit less than initial credit");
	// Read register USEM_REG_FAST_MEMORY + 0x18040 val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_FAST_MEMORY + 0x18040, (val != 78), IDLE_CHK_ERROR_NO_TRAFFIC, "USEM: FOC1 credit less than initial credit");
	// Read register USEM_REG_FAST_MEMORY + 0x18080 val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_FAST_MEMORY + 0x18080, (val != 16), IDLE_CHK_ERROR_NO_TRAFFIC, "USEM: FOC2 credit less than initial credit");
	// Read register USEM_REG_FAST_MEMORY + 0x180C0 val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_FAST_MEMORY + 0x180C0, (val != 32), IDLE_CHK_ERROR_NO_TRAFFIC, "USEM: FOC3 credit less than initial credit");
	// Read register TSEM_REG_FAST_MEMORY + 0x18000 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_FAST_MEMORY + 0x18000, (val != 52), IDLE_CHK_ERROR_NO_TRAFFIC, "TSEM: FOC0 credit less than initial credit");
	// Read register TSEM_REG_FAST_MEMORY + 0x18040 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_FAST_MEMORY + 0x18040, (val != 24), IDLE_CHK_ERROR_NO_TRAFFIC, "TSEM: FOC1 credit less than initial credit");
	// Read register TSEM_REG_FAST_MEMORY + 0x18080 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_FAST_MEMORY + 0x18080, (val != 12), IDLE_CHK_ERROR_NO_TRAFFIC, "TSEM: FOC2 credit less than initial credit");
	// Read register TSEM_REG_FAST_MEMORY + 0x180C0 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_FAST_MEMORY + 0x180C0, (val != 32), IDLE_CHK_ERROR_NO_TRAFFIC, "TSEM: FOC3 credit less than initial credit");
	// Read register CSEM_REG_FAST_MEMORY + 0x18000 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_FAST_MEMORY + 0x18000, (val != 16), IDLE_CHK_ERROR_NO_TRAFFIC, "CSEM: FOC0 credit less than initial credit");
	// Read register CSEM_REG_FAST_MEMORY + 0x18040 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_FAST_MEMORY + 0x18040, (val != 18), IDLE_CHK_ERROR_NO_TRAFFIC, "CSEM: FOC1 credit less than initial credit");
	// Read register CSEM_REG_FAST_MEMORY + 0x18080 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_FAST_MEMORY + 0x18080, (val != 48), IDLE_CHK_ERROR_NO_TRAFFIC, "CSEM: FOC2 credit less than initial credit");
	// Read register CSEM_REG_FAST_MEMORY + 0x180C0 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_FAST_MEMORY + 0x180C0, (val != 14), IDLE_CHK_ERROR_NO_TRAFFIC, "CSEM: FOC3 credit less than initial credit");
	// Read register PRS_REG_TSDM_CURRENT_CREDIT val and check if condition on val exist
	IDLE_CHK_1(0xF, PRS_REG_TSDM_CURRENT_CREDIT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PRS: TSDM current credit is not 0");
	// Read register PRS_REG_TCM_CURRENT_CREDIT val and check if condition on val exist
	IDLE_CHK_1(0xF, PRS_REG_TCM_CURRENT_CREDIT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PRS: TCM current credit is not 0");
	// Read register PRS_REG_CFC_LD_CURRENT_CREDIT val and check if condition on val exist
	IDLE_CHK_1(0xF, PRS_REG_CFC_LD_CURRENT_CREDIT, (val != 0), IDLE_CHK_ERROR, "PRS: CFC_LD current credit is not 0");
	// Read register PRS_REG_CFC_SEARCH_CURRENT_CREDIT val and check if condition on val exist
	IDLE_CHK_1(0xF, PRS_REG_CFC_SEARCH_CURRENT_CREDIT, (val != 0), IDLE_CHK_ERROR, "PRS: CFC_SEARCH current credit is not 0");
	// Read register PRS_REG_SRC_CURRENT_CREDIT val and check if condition on val exist
	IDLE_CHK_1(0xF, PRS_REG_SRC_CURRENT_CREDIT, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PRS: SRCH current credit is not 0");
	// Read register PRS_REG_PENDING_BRB_PRS_RQ val and check if condition on val exist
	IDLE_CHK_1(0xF, PRS_REG_PENDING_BRB_PRS_RQ, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PRS: PENDING_BRB_PRS_RQ is not 0");
	// Read register PRS_REG_PENDING_BRB_CAC0_RQ val in loop (incr by 4) and check if condition on val exist
	IDLE_CHK_2(0xF, PRS_REG_PENDING_BRB_CAC0_RQ, 5, 4, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PRS: PENDING_BRB_CAC_RQ is not 0");
	// Read register PRS_REG_SERIAL_NUM_STATUS_LSB val and check if condition on val exist
	IDLE_CHK_1(0xF, PRS_REG_SERIAL_NUM_STATUS_LSB, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PRS: SERIAL_NUM_STATUS_LSB is not 0");
	// Read register PRS_REG_SERIAL_NUM_STATUS_MSB val and check if condition on val exist
	IDLE_CHK_1(0xF, PRS_REG_SERIAL_NUM_STATUS_MSB, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "PRS: SERIAL_NUM_STATUS_MSB is not 0");
	// Read register CDU_REG_ERROR_DATA val and check if condition on val exist
	IDLE_CHK_1(0xF, CDU_REG_ERROR_DATA, (val != 0), IDLE_CHK_ERROR, "CDU: ERROR_DATA is not 0");
	// Read register CCM_REG_STORM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_STORM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "CCM: STORM declared message length unequal to actual");
	// Read register CCM_REG_CSDM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_CSDM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "CCM: CSDM declared message length unequal to actual");
	// Read register CCM_REG_TSEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_TSEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "CCM: TSEM declared message length unequal to actual");
	// Read register CCM_REG_XSEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_XSEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "CCM: XSEM declared message length unequal to actual");
	// Read register CCM_REG_USEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_USEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "CCM: USEM declared message length unequal to actual");
	// Read register CCM_REG_PBF_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, CCM_REG_PBF_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "CCM: PBF declared message length unequal to actual");
	// Read register TCM_REG_STORM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_STORM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "TCM: STORM declared message length unequal to actual");
	// Read register TCM_REG_TSDM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_TSDM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "TCM: TSDM declared message length unequal to actual");
	// Read register TCM_REG_PRS_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_PRS_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "TCM: PRS declared message length unequal to actual");
	// Read register TCM_REG_PBF_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_PBF_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "TCM: PBF declared message length unequal to actual");
	// Read register TCM_REG_USEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_USEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "TCM: USEM declared message length unequal to actual");
	// Read register TCM_REG_CSEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, TCM_REG_CSEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "TCM: CSEM declared message length unequal to actual");
	// Read register UCM_REG_STORM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_STORM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "UCM: STORM declared message length unequal to actual");
	// Read register UCM_REG_USDM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_USDM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "UCM: USDM declared message length unequal to actual");
	// Read register UCM_REG_TSEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_TSEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "UCM: TSEM declared message length unequal to actual");
	// Read register UCM_REG_CSEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_CSEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "UCM: CSEM declared message length unequal to actual");
	// Read register UCM_REG_XSEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_XSEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "UCM: XSEM declared message length unequal to actual");
	// Read register UCM_REG_DORQ_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, UCM_REG_DORQ_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "UCM: DORQ declared message length unequal to actual");
	// Read register XCM_REG_STORM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_STORM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "XCM: STORM declared message length unequal to actual");
	// Read register XCM_REG_XSDM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_XSDM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "XCM: XSDM declared message length unequal to actual");
	// Read register XCM_REG_TSEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_TSEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "XCM: TSEM declared message length unequal to actual");
	// Read register XCM_REG_CSEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_CSEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "XCM: CSEM declared message length unequal to actual");
	// Read register XCM_REG_USEM_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_USEM_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "XCM: USEM declared message length unequal to actual");
	// Read register XCM_REG_DORQ_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_DORQ_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "XCM: DORQ declared message length unequal to actual");
	// Read register XCM_REG_PBF_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_PBF_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "XCM: PBF declared message length unequal to actual");
	// Read register XCM_REG_NIG0_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_NIG0_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "XCM: NIG0 declared message length unequal to actual");
	// Read register XCM_REG_NIG1_LENGTH_MIS val and check if condition on val exist
	IDLE_CHK_1(0xF, XCM_REG_NIG1_LENGTH_MIS, (val != 0), IDLE_CHK_ERROR, "XCM: NIG1 declared message length unequal to actual");
	// Read register QM_REG_XQM_WRC_FIFOLVL val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_XQM_WRC_FIFOLVL, (val != 0), IDLE_CHK_ERROR, "QM: XQM wrc_fifolvl is not 0");
	// Read register QM_REG_UQM_WRC_FIFOLVL val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_UQM_WRC_FIFOLVL, (val != 0), IDLE_CHK_ERROR, "QM: UQM wrc_fifolvl is not 0");
	// Read register QM_REG_TQM_WRC_FIFOLVL val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_TQM_WRC_FIFOLVL, (val != 0), IDLE_CHK_ERROR, "QM: TQM wrc_fifolvl is not 0");
	// Read register QM_REG_CQM_WRC_FIFOLVL val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_CQM_WRC_FIFOLVL, (val != 0), IDLE_CHK_ERROR, "QM: CQM wrc_fifolvl is not 0");
	// Read register QM_REG_QSTATUS_LOW val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_QSTATUS_LOW, (val != 0), IDLE_CHK_ERROR, "QM: QSTATUS_LOW is not 0");
	// Read register QM_REG_QSTATUS_HIGH val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_QSTATUS_HIGH, (val != 0), IDLE_CHK_ERROR, "QM: QSTATUS_HIGH is not 0");
	// Read register QM_REG_PAUSESTATE0 val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_PAUSESTATE0, (val != 0), IDLE_CHK_ERROR, "QM: PAUSESTATE0 is not 0");
	// Read register QM_REG_PAUSESTATE1 val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_PAUSESTATE1, (val != 0), IDLE_CHK_ERROR, "QM: PAUSESTATE1 is not 0");
	// Read register QM_REG_OVFQNUM val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_OVFQNUM, (val != 0), IDLE_CHK_ERROR, "QM: OVFQNUM is not 0");
	// Read register QM_REG_OVFERROR val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_OVFERROR, (val != 0), IDLE_CHK_ERROR, "QM: OVFERROR is not 0");
	// Special check for QM PTRTBL
	IDLE_CHK_6(0xF, QM_REG_PTRTBL, 64, 8, IDLE_CHK_ERROR_NO_TRAFFIC);
	// Read register BRB1_REG_BRB1_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, BRB1_REG_BRB1_PRTY_STS, ((val & ~ 0x8) != 0), IDLE_CHK_WARNING, "BRB1: parity status is not 0");
	// Read register CDU_REG_CDU_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, CDU_REG_CDU_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "CDU: parity status is not 0");
	// Read register CFC_REG_CFC_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, CFC_REG_CFC_PRTY_STS, ((val  & ~0x2)!= 0), IDLE_CHK_WARNING, "CFC: parity status is not 0");
	// Read register CSDM_REG_CSDM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, CSDM_REG_CSDM_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "CSDM: parity status is not 0");
	// Read register DBG_REG_DBG_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0x3, DBG_REG_DBG_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "DBG: parity status is not 0");
	// Read register DMAE_REG_DMAE_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, DMAE_REG_DMAE_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "DMAE: parity status is not 0");
	// Read register DORQ_REG_DORQ_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, DORQ_REG_DORQ_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "DORQ: parity status is not 0");
	// Read register TCM_REG_TCM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0x1, TCM_REG_TCM_PRTY_STS, ((val & ~0x3ffc0) != 0), IDLE_CHK_WARNING, "TCM: parity status is not 0");
	// Read register TCM_REG_TCM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xE, TCM_REG_TCM_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "TCM: parity status is not 0");
	// Read register CCM_REG_CCM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0x1, CCM_REG_CCM_PRTY_STS, ((val & ~0x3ffc0) != 0), IDLE_CHK_WARNING, "CCM: parity status is not 0");
	// Read register CCM_REG_CCM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xE, CCM_REG_CCM_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "CCM: parity status is not 0");
	// Read register UCM_REG_UCM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0x1, UCM_REG_UCM_PRTY_STS, ((val & ~0x3ffc0) != 0), IDLE_CHK_WARNING, "UCM: parity status is not 0");
	// Read register UCM_REG_UCM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xE, UCM_REG_UCM_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "UCM: parity status is not 0");
	// Read register XCM_REG_XCM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0x1, XCM_REG_XCM_PRTY_STS, ((val & ~0x3ffc0) != 0), IDLE_CHK_WARNING, "XCM: parity status is not 0");
	// Read register XCM_REG_XCM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xE, XCM_REG_XCM_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "XCM: parity status is not 0");
	// Read register HC_REG_HC_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0x1, HC_REG_HC_PRTY_STS, ((val& ~0x1) != 0), IDLE_CHK_WARNING, "HC: parity status is not 0");
	// Read register MISC_REG_MISC_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0x1, MISC_REG_MISC_PRTY_STS, ((val& ~0x1) != 0), IDLE_CHK_WARNING, "MISC: parity status is not 0");
	// Read register PRS_REG_PRS_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, PRS_REG_PRS_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "PRS: parity status is not 0");
	// Read register PXP_REG_PXP_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, PXP_REG_PXP_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "PXP: parity status is not 0");
	// Read register QM_REG_QM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, QM_REG_QM_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "QM: parity status is not 0");
	// Read register SRC_REG_SRC_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0x1, SRC_REG_SRC_PRTY_STS, ((val & ~0x4) != 0), IDLE_CHK_WARNING, "SRCH: parity status is not 0");
	// Read register TSDM_REG_TSDM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, TSDM_REG_TSDM_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "TSDM: parity status is not 0");
	// Read register USDM_REG_USDM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, USDM_REG_USDM_PRTY_STS, ((val & ~0x20) != 0), IDLE_CHK_WARNING, "USDM: parity status is not 0");
	// Read register XSDM_REG_XSDM_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, XSDM_REG_XSDM_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "XSDM: parity status is not 0");
	// Read register GRCBASE_XPB + PB_REG_PB_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, GRCBASE_XPB + PB_REG_PB_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "XPB: parity status is not 0");
	// Read register GRCBASE_UPB + PB_REG_PB_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xF, GRCBASE_UPB + PB_REG_PB_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "UPB: parity status is not 0");
	// Read register CSEM_REG_CSEM_PRTY_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_CSEM_PRTY_STS_0, (val != 0), IDLE_CHK_WARNING, "CSEM: parity status 0 is not 0");
	// Read register PXP2_REG_PXP2_PRTY_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0x1, PXP2_REG_PXP2_PRTY_STS_0, ((val & ~0xfff40020) != 0) , IDLE_CHK_WARNING, "PXP2: parity status 0 is not 0");
	// Read register PXP2_REG_PXP2_PRTY_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xE, PXP2_REG_PXP2_PRTY_STS_0, ((val & ~0x20) != 0), IDLE_CHK_WARNING, "PXP2: parity status 0 is not 0");
	// Read register TSEM_REG_TSEM_PRTY_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_TSEM_PRTY_STS_0, (val != 0), IDLE_CHK_WARNING, "TSEM: parity status 0 is not 0");
	// Read register USEM_REG_USEM_PRTY_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_USEM_PRTY_STS_0, (val != 0), IDLE_CHK_WARNING, "USEM: parity status 0 is not 0");
	// Read register XSEM_REG_XSEM_PRTY_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, XSEM_REG_XSEM_PRTY_STS_0, (val != 0), IDLE_CHK_WARNING, "XSEM: parity status 0 is not 0");
	// Read register CSEM_REG_CSEM_PRTY_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_CSEM_PRTY_STS_1, (val != 0), IDLE_CHK_WARNING, "CSEM: parity status 1 is not 0");
	// Read register PXP2_REG_PXP2_PRTY_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0x1, PXP2_REG_PXP2_PRTY_STS_1, ((val & ~0x20) != 0), IDLE_CHK_WARNING, "PXP2: parity status 1 is not 0");
	// Read register PXP2_REG_PXP2_PRTY_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xE, PXP2_REG_PXP2_PRTY_STS_1, (val != 0), IDLE_CHK_WARNING, "PXP2: parity status 1 is not 0");
	// Read register TSEM_REG_TSEM_PRTY_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_TSEM_PRTY_STS_1, (val != 0), IDLE_CHK_WARNING, "TSEM: parity status 1 is not 0");
	// Read register USEM_REG_USEM_PRTY_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_USEM_PRTY_STS_1, (val != 0), IDLE_CHK_WARNING, "USEM: parity status 1 is not 0");
	// Read register XSEM_REG_XSEM_PRTY_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, XSEM_REG_XSEM_PRTY_STS_1, (val != 0), IDLE_CHK_WARNING, "XSEM: parity status 1 is not 0");
	// Read register PGLUE_B_REG_PGLUE_B_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_PGLUE_B_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "PGLUE_B: parity status is not 0");
	// Read register QM_REG_QTASKCTR_EXT_A_0 val in loop (incr by 4) and check if condition on val exist
	IDLE_CHK_2(0x2, QM_REG_QTASKCTR_EXT_A_0, 64, 4, (val != 0), IDLE_CHK_ERROR_NO_TRAFFIC, "QM: Q_EXT_A (upper 64 queues), Queue is not empty");
	// Read register QM_REG_QSTATUS_LOW_EXT_A val and check if condition on val exist
	IDLE_CHK_1(0x2, QM_REG_QSTATUS_LOW_EXT_A, (val != 0), IDLE_CHK_ERROR, "QM: QSTATUS_LOW_EXT_A is not 0");
	// Read register QM_REG_QSTATUS_HIGH_EXT_A val and check if condition on val exist
	IDLE_CHK_1(0x2, QM_REG_QSTATUS_HIGH_EXT_A, (val != 0), IDLE_CHK_ERROR, "QM: QSTATUS_HIGH_EXT_A is not 0");
	// Read register QM_REG_PAUSESTATE2 val and check if condition on val exist
	IDLE_CHK_1(0xE, QM_REG_PAUSESTATE2, (val != 0), IDLE_CHK_ERROR, "QM: PAUSESTATE2 is not 0");
	// Read register QM_REG_PAUSESTATE3 val and check if condition on val exist
	IDLE_CHK_1(0xE, QM_REG_PAUSESTATE3, (val != 0), IDLE_CHK_ERROR, "QM: PAUSESTATE3 is not 0");
	// Read register QM_REG_PAUSESTATE4 val and check if condition on val exist
	IDLE_CHK_1(0x2, QM_REG_PAUSESTATE4, (val != 0), IDLE_CHK_ERROR, "QM: PAUSESTATE4 is not 0");
	// Read register QM_REG_PAUSESTATE5 val and check if condition on val exist
	IDLE_CHK_1(0x2, QM_REG_PAUSESTATE5, (val != 0), IDLE_CHK_ERROR, "QM: PAUSESTATE5 is not 0");
	// Read register QM_REG_PAUSESTATE6 val and check if condition on val exist
	IDLE_CHK_1(0x2, QM_REG_PAUSESTATE6, (val != 0), IDLE_CHK_ERROR, "QM: PAUSESTATE6 is not 0");
	// Read register QM_REG_PAUSESTATE7 val and check if condition on val exist
	IDLE_CHK_1(0x2, QM_REG_PAUSESTATE7, (val != 0), IDLE_CHK_ERROR, "QM: PAUSESTATE7 is not 0");
	// Special check for QM PTRTBL
	IDLE_CHK_6(0x2, QM_REG_PTRTBL_EXT_A, 64, 8, IDLE_CHK_ERROR_NO_TRAFFIC);
	// Read register MISC_REG_AEU_SYS_KILL_OCCURRED val and check if condition on val exist
	IDLE_CHK_1(0xE, MISC_REG_AEU_SYS_KILL_OCCURRED, (val != 0), IDLE_CHK_ERROR, "MISC: system kill occurd;");
	// Read register MISC_REG_AEU_SYS_KILL_STATUS_0 val and check if condition on val exist
	IDLE_CHK_1(0xE, MISC_REG_AEU_SYS_KILL_STATUS_0, (val != 0), IDLE_CHK_ERROR, "MISC: system kill occurd; status_0 register");
	// Read register MISC_REG_AEU_SYS_KILL_STATUS_1 val and check if condition on val exist
	IDLE_CHK_1(0xE, MISC_REG_AEU_SYS_KILL_STATUS_1, (val != 0), IDLE_CHK_ERROR, "MISC: system kill occurd; status_1 register");
	// Read register MISC_REG_AEU_SYS_KILL_STATUS_2 val and check if condition on val exist
	IDLE_CHK_1(0xE, MISC_REG_AEU_SYS_KILL_STATUS_2, (val != 0), IDLE_CHK_ERROR, "MISC: system kill occurd; status_2 register");
	// Read register MISC_REG_AEU_SYS_KILL_STATUS_3 val and check if condition on val exist
	IDLE_CHK_1(0xE, MISC_REG_AEU_SYS_KILL_STATUS_3, (val != 0), IDLE_CHK_ERROR, "MISC: system kill occurd; status_3 register");
	// Read register MISC_REG_PCIE_HOT_RESET val and check if condition on val exist
	IDLE_CHK_1(0xE, MISC_REG_PCIE_HOT_RESET, (val != 0), IDLE_CHK_WARNING, "MISC: pcie_rst_b was asserted without perst assertion");
	// Read register NIG_REG_NIG_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, NIG_REG_NIG_INT_STS_0, ((val & ~0x300) != 0), IDLE_CHK_ERROR, "NIG: interrupt 0 is active");
	// Read register NIG_REG_NIG_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, NIG_REG_NIG_INT_STS_0, (val == 0x300), IDLE_CHK_WARNING, "NIG: Access to BMAC while not active. If tested on FPGA, ignore this warning.");
	// Read register NIG_REG_NIG_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, NIG_REG_NIG_INT_STS_1, (val != 0), IDLE_CHK_ERROR, "NIG: interrupt 1 is active");
	// Read register NIG_REG_NIG_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0x2, NIG_REG_NIG_PRTY_STS, ((val & ~0xFFC00000) != 0), IDLE_CHK_ERROR, "NIG: parity interrupt is active");
	// Read register NIG_REG_NIG_PRTY_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xC, NIG_REG_NIG_PRTY_STS_0, ((val & ~0xFFC00000) != 0), IDLE_CHK_ERROR, "NIG: parity 0 interrupt is active");
	// Read register NIG_REG_NIG_PRTY_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0x4, NIG_REG_NIG_PRTY_STS_1, ((val & 0xff) != 0), IDLE_CHK_ERROR, "NIG: parity 1 interrupt is active");
	// Read register NIG_REG_NIG_PRTY_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0x8, NIG_REG_NIG_PRTY_STS_1, (val != 0), IDLE_CHK_ERROR, "NIG: parity 1 interrupt is active");
	// Read register TSEM_REG_TSEM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_TSEM_INT_STS_0, ((val & ~0x10000000) != 0), IDLE_CHK_WARNING, "TSEM: interrupt 0 is active");
	// Read register TSEM_REG_TSEM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_TSEM_INT_STS_0, (val  == 0x10000000), IDLE_CHK_WARNING, "TSEM: interrupt 0 is active");
	// Read register TSEM_REG_TSEM_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSEM_REG_TSEM_INT_STS_1, (val != 0), IDLE_CHK_ERROR, "TSEM: interrupt 1 is active");
	// Read register CSEM_REG_CSEM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_CSEM_INT_STS_0, ((val & ~0x10000000) != 0), IDLE_CHK_WARNING, "CSEM: interrupt 0 is active");
	// Read register CSEM_REG_CSEM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_CSEM_INT_STS_0, (val  == 0x10000000), IDLE_CHK_WARNING, "CSEM: interrupt 0 is active");
	// Read register CSEM_REG_CSEM_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSEM_REG_CSEM_INT_STS_1, (val != 0), IDLE_CHK_ERROR, "CSEM: interrupt 1 is active");
	// Read register USEM_REG_USEM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_USEM_INT_STS_0, ((val & ~0x10000000) != 0), IDLE_CHK_WARNING, "USEM: interrupt 0 is active");
	// Read register USEM_REG_USEM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_USEM_INT_STS_0, (val  == 0x10000000), IDLE_CHK_WARNING, "USEM: interrupt 0 is active");
	// Read register USEM_REG_USEM_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, USEM_REG_USEM_INT_STS_1, (val != 0), IDLE_CHK_ERROR, "USEM: interrupt 1 is active");
	// Read register XSEM_REG_XSEM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, XSEM_REG_XSEM_INT_STS_0, ((val & ~0x10000000) != 0), IDLE_CHK_WARNING, "XSEM: interrupt 0 is active");
	// Read register XSEM_REG_XSEM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, XSEM_REG_XSEM_INT_STS_0, (val  == 0x10000000), IDLE_CHK_WARNING, "XSEM: interrupt 0 is active");
	// Read register XSEM_REG_XSEM_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, XSEM_REG_XSEM_INT_STS_1, (val != 0), IDLE_CHK_ERROR, "XSEM: interrupt 1 is active");
	// Read register TSDM_REG_TSDM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSDM_REG_TSDM_INT_STS_0, (val != 0), IDLE_CHK_ERROR, "TSDM: interrupt 0 is active");
	// Read register TSDM_REG_TSDM_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, TSDM_REG_TSDM_INT_STS_1, (val != 0), IDLE_CHK_ERROR, "TSDM: interrupt 0 is active");
	// Read register CSDM_REG_CSDM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSDM_REG_CSDM_INT_STS_0, (val != 0), IDLE_CHK_ERROR, "CSDM: interrupt 0 is active");
	// Read register CSDM_REG_CSDM_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, CSDM_REG_CSDM_INT_STS_1, (val != 0), IDLE_CHK_ERROR, "CSDM: interrupt 0 is active");
	// Read register USDM_REG_USDM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, USDM_REG_USDM_INT_STS_0, (val != 0), IDLE_CHK_ERROR, "USDM: interrupt 0 is active");
	// Read register USDM_REG_USDM_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, USDM_REG_USDM_INT_STS_1, (val != 0), IDLE_CHK_ERROR, "USDM: interrupt 0 is active");
	// Read register XSDM_REG_XSDM_INT_STS_0 val and check if condition on val exist
	IDLE_CHK_1(0xF, XSDM_REG_XSDM_INT_STS_0, (val != 0), IDLE_CHK_ERROR, "XSDM: interrupt 0 is active");
	// Read register XSDM_REG_XSDM_INT_STS_1 val and check if condition on val exist
	IDLE_CHK_1(0xF, XSDM_REG_XSDM_INT_STS_1, (val != 0), IDLE_CHK_ERROR, "XSDM: interrupt 0 is active");
	// Read register HC_REG_HC_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0x2, HC_REG_HC_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "HC: parity status is not 0");
	// Read register MISC_REG_MISC_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xE, MISC_REG_MISC_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "MISC: parity status is not 0");
	// Read register SRC_REG_SRC_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xE, SRC_REG_SRC_PRTY_STS, (val  != 0), IDLE_CHK_WARNING, "SRCH: parity status is not 0");
	// Read register QM_REG_BYTECRD0 val1 and register QM_REG_BYTECRDINITVAL val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xC, QM_REG_BYTECRD0, QM_REG_BYTECRDINITVAL, (val1 != val2), IDLE_CHK_ERROR_NO_TRAFFIC, "QM: Byte credit 0 is not equal to initial credit");
	// Read register QM_REG_BYTECRD1 val1 and register QM_REG_BYTECRDINITVAL val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xC, QM_REG_BYTECRD1, QM_REG_BYTECRDINITVAL, (val1 != val2), IDLE_CHK_ERROR_NO_TRAFFIC, "QM: Byte credit 1 is not equal to initial credit");
	// Read register QM_REG_BYTECRD2 val1 and register QM_REG_BYTECRDINITVAL val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xC, QM_REG_BYTECRD2, QM_REG_BYTECRDINITVAL, (val1 != val2), IDLE_CHK_ERROR_NO_TRAFFIC, "QM: Byte credit 2 is not equal to initial credit");
	// Read register QM_REG_VOQCRDERRREG val and check if condition on val exist
	IDLE_CHK_1(0xC, QM_REG_VOQCRDERRREG, ((val & 0xFFFF) != 0), IDLE_CHK_ERROR, "QM: VOQ credit error register is not 0 (VOQ credit overflow/underflow)");
	// Read register QM_REG_BYTECRDERRREG val and check if condition on val exist
	IDLE_CHK_1(0xC, QM_REG_BYTECRDERRREG, ((val & 0xFFF) != 0), IDLE_CHK_ERROR, "QM: Byte credit error register is not 0 (Byte credit overflow/underflow)");
	// Read register PGLUE_B_REG_FLR_REQUEST_VF_31_0 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_FLR_REQUEST_VF_31_0, (val  != 0), IDLE_CHK_WARNING, "PGL: FLR request is set for VF addresses 31-0");
	// Read register PGLUE_B_REG_FLR_REQUEST_VF_63_32 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_FLR_REQUEST_VF_63_32, (val  != 0), IDLE_CHK_WARNING, "PGL: FLR request is set for VF addresses 63-32");
	// Read register PGLUE_B_REG_FLR_REQUEST_VF_95_64 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_FLR_REQUEST_VF_95_64, (val  != 0), IDLE_CHK_WARNING, "PGL: FLR request is set for VF addresses 95-64");
	// Read register PGLUE_B_REG_FLR_REQUEST_VF_127_96 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_FLR_REQUEST_VF_127_96, (val  != 0), IDLE_CHK_WARNING, "PGL: FLR request is set for VF addresses 127-96");
	// Read register PGLUE_B_REG_FLR_REQUEST_PF_7_0 val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_FLR_REQUEST_PF_7_0, (val  != 0), IDLE_CHK_WARNING, "PGL: FLR request is set for PF addresses 7-0");
	// Read register PGLUE_B_REG_SR_IOV_DISABLED_REQUEST val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_SR_IOV_DISABLED_REQUEST, (val  != 0), IDLE_CHK_WARNING, "PGL: SR-IOV disable request is set ");
	// Read register PGLUE_B_REG_CFG_SPACE_A_REQUEST val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_CFG_SPACE_A_REQUEST, (val  != 0), IDLE_CHK_WARNING, "PGL: Cfg-Space A request is set");
	// Read register PGLUE_B_REG_CFG_SPACE_B_REQUEST val and check if condition on val exist
	IDLE_CHK_1(0xC, PGLUE_B_REG_CFG_SPACE_B_REQUEST, (val  != 0), IDLE_CHK_WARNING, "PGL: Cfg-Space B request is set");
	// Read register IGU_REG_ERROR_HANDLING_DATA_VALID val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_ERROR_HANDLING_DATA_VALID, (val  != 0), IDLE_CHK_ERROR, "IGU: some unauthorized commands arrived to the IGU. Read IGU_REG_ERROR_HANDLING_MEMORY for more details.");
	// Read register IGU_REG_ATTN_WRITE_DONE_PENDING val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_ATTN_WRITE_DONE_PENDING, (val  != 0), IDLE_CHK_WARNING, "IGU attention message write done pending is not empty");
	// Read register IGU_REG_WRITE_DONE_PENDING val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_WRITE_DONE_PENDING, (val  != 0), IDLE_CHK_WARNING, "IGU MSI/MSIX message write done pending is not empty");
	// Read register IGU_REG_IGU_PRTY_STS val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_IGU_PRTY_STS, (val != 0), IDLE_CHK_WARNING, "IGU: parity status is not 0");
	// Read register MISC_REG_GRC_TIMEOUT_ATTN val1 and register MISC_REG_AEU_AFTER_INVERT_4_FUNC_0 val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xE, MISC_REG_GRC_TIMEOUT_ATTN, MISC_REG_AEU_AFTER_INVERT_4_FUNC_0, ((val2 & 0x4000000) != 0), IDLE_CHK_ERROR, "MISC_REG_GRC_TIMEOUT_ATTN: GRC timeout attention parameters (FUNC_0).");
	// Read register MISC_REG_GRC_TIMEOUT_ATTN_FULL_FID val1 and register MISC_REG_AEU_AFTER_INVERT_4_FUNC_0 val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xC, MISC_REG_GRC_TIMEOUT_ATTN_FULL_FID, MISC_REG_AEU_AFTER_INVERT_4_FUNC_0, ((val2 & 0x4000000) != 0), IDLE_CHK_ERROR, "MISC_REG_GRC_TIMEOUT_ATTN_FULL_FID: GRC timeout attention FID (FUNC_0).");
	// Read register MISC_REG_GRC_TIMEOUT_ATTN val1 and register MISC_REG_AEU_AFTER_INVERT_4_FUNC_1 val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xE, MISC_REG_GRC_TIMEOUT_ATTN, MISC_REG_AEU_AFTER_INVERT_4_FUNC_1, ((val2 & 0x4000000) != 0), IDLE_CHK_ERROR, "MISC_REG_GRC_TIMEOUT_ATTN: GRC timeout attention parameters (FUNC_1).");
	// Read register MISC_REG_GRC_TIMEOUT_ATTN_FULL_FID val1 and register MISC_REG_AEU_AFTER_INVERT_4_FUNC_1 val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xC, MISC_REG_GRC_TIMEOUT_ATTN_FULL_FID, MISC_REG_AEU_AFTER_INVERT_4_FUNC_1, ((val2 & 0x4000000) != 0), IDLE_CHK_ERROR, "MISC_REG_GRC_TIMEOUT_ATTN_FULL_FID: GRC timeout attention FID (FUNC_1).");
	// Read register MISC_REG_GRC_TIMEOUT_ATTN val1 and register MISC_REG_AEU_AFTER_INVERT_4_MCP val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xE, MISC_REG_GRC_TIMEOUT_ATTN, MISC_REG_AEU_AFTER_INVERT_4_MCP, ((val2 & 0x4000000) != 0), IDLE_CHK_ERROR, "MISC_REG_GRC_TIMEOUT_ATTN: GRC timeout attention parameters (MCP).");
	// Read register MISC_REG_GRC_TIMEOUT_ATTN_FULL_FID val1 and register MISC_REG_AEU_AFTER_INVERT_4_MCP val2 and check if condition on val1,val2 exist
	IDLE_CHK_3(0xC, MISC_REG_GRC_TIMEOUT_ATTN_FULL_FID, MISC_REG_AEU_AFTER_INVERT_4_MCP, ((val2 & 0x4000000) != 0), IDLE_CHK_ERROR, "MISC_REG_GRC_TIMEOUT_ATTN_FULL_FID: GRC timeout attention FID (MCP).");
	// Read register IGU_REG_SILENT_DROP val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_SILENT_DROP, (val  != 0), IDLE_CHK_ERROR, "Some messages were not executed in the IGU.");
	// Read register PXP2_REG_PSWRQ_BW_CREDIT val and check if condition on val exist
	IDLE_CHK_1(0xC, PXP2_REG_PSWRQ_BW_CREDIT, (val != 0x2D), IDLE_CHK_ERROR, "PXP2: rq_read_credit and rq_write_credit are not 5");
	// Read register IGU_REG_SB_CTRL_FSM val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_SB_CTRL_FSM, (val  != 0), IDLE_CHK_WARNING, "IGU: block is not in idle. SB_CTRL_FSM should be zero in idle state");
	// Read register IGU_REG_INT_HANDLE_FSM val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_INT_HANDLE_FSM, (val  != 0), IDLE_CHK_WARNING, "IGU: block is not in idle. INT_HANDLE_FSM should be zero in idle state");
	// Read register IGU_REG_ATTN_FSM val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_ATTN_FSM, ((val & ~0x2)  != 0), IDLE_CHK_WARNING, "IGU: block is not in idle. SB_ATTN_FSMshould be zeroor two in idle state");
	// Read register IGU_REG_CTRL_FSM val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_CTRL_FSM, ((val & ~0x1)  != 0), IDLE_CHK_WARNING, "IGU: block is not in idle. SB_CTRL_FSM should be zero in idle state");
	// Read register IGU_REG_PXP_ARB_FSM val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_PXP_ARB_FSM, ((val & ~0x1)  != 0), IDLE_CHK_WARNING, "IGU: block is not in idle. SB_ARB_FSM should be zero in idle state");
	// Read register IGU_REG_PENDING_BITS_STATUS val and check if condition on val exist
	IDLE_CHK_1(0xC, IGU_REG_PENDING_BITS_STATUS, (val  != 0), IDLE_CHK_WARNING, "IGU: block is not in idle. There are pending write done");

	if (errors == 0) {
		BNX2X_DBG_ERR("completed successfully (with %d warnings)\n",
			      idle_chk_warnings);
	} else {
		BNX2X_DBG_ERR("failed (with %d errors, %d warnings)\n",
			      errors, idle_chk_warnings);
	}
	return errors;
}

