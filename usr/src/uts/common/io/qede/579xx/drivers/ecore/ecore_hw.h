/*
* CDDL HEADER START
*
* The contents of this file are subject to the terms of the
* Common Development and Distribution License, v.1,  (the "License").
* You may not use this file except in compliance with the License.
*
* You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
* or http://opensource.org/licenses/CDDL-1.0.
* See the License for the specific language governing permissions
* and limitations under the License.
*
* When distributing Covered Code, include this CDDL HEADER in each
* file and include the License file at usr/src/OPENSOLARIS.LICENSE.
* If applicable, add the following below this CDDL HEADER, with the
* fields enclosed by brackets "[]" replaced with your own identifying
* information: Portions Copyright [yyyy] [name of copyright owner]
*
* CDDL HEADER END
*/

/*
* Copyright 2014-2017 Cavium, Inc. 
* The contents of this file are subject to the terms of the Common Development 
* and Distribution License, v.1,  (the "License").

* You may not use this file except in compliance with the License.

* You can obtain a copy of the License at available 
* at http://opensource.org/licenses/CDDL-1.0

* See the License for the specific language governing permissions and 
* limitations under the License.
*/

#ifndef __ECORE_HW_H__
#define __ECORE_HW_H__

#include "ecore.h"
#include "ecore_dev_api.h"

/* Forward decleration */
struct ecore_ptt;

enum reserved_ptts {
	RESERVED_PTT_EDIAG,
	RESERVED_PTT_USER_SPACE,
	RESERVED_PTT_MAIN,
	RESERVED_PTT_DPC,
	RESERVED_PTT_MAX
};

/* @@@TMP - in earlier versions of the emulation, the HW lock started from 1
 * instead of 0, this should be fixed in later HW versions.
 */
#ifndef MISC_REG_DRIVER_CONTROL_0
#define MISC_REG_DRIVER_CONTROL_0	MISC_REG_DRIVER_CONTROL_1
#endif
#ifndef MISC_REG_DRIVER_CONTROL_0_SIZE
#define MISC_REG_DRIVER_CONTROL_0_SIZE	MISC_REG_DRIVER_CONTROL_1_SIZE
#endif

enum _dmae_cmd_dst_mask {
	DMAE_CMD_DST_MASK_NONE = 0,
	DMAE_CMD_DST_MASK_PCIE = 1,
	DMAE_CMD_DST_MASK_GRC = 2
};

enum _dmae_cmd_src_mask {
	DMAE_CMD_SRC_MASK_PCIE = 0,
	DMAE_CMD_SRC_MASK_GRC = 1
};

enum _dmae_cmd_crc_mask {
	DMAE_CMD_COMP_CRC_EN_MASK_NONE = 0,
	DMAE_CMD_COMP_CRC_EN_MASK_SET = 1
};

/* definitions for DMA constants */
#define DMAE_GO_VALUE	0x1

#ifdef __BIG_ENDIAN
#define DMAE_COMPLETION_VAL	0xAED10000
#define DMAE_CMD_ENDIANITY	0x3
#else
#define DMAE_COMPLETION_VAL	0xD1AE
#define DMAE_CMD_ENDIANITY	0x2
#endif

#define DMAE_CMD_SIZE	14
/* size of DMAE command structure to fill.. DMAE_CMD_SIZE-5 */
#define DMAE_CMD_SIZE_TO_FILL	(DMAE_CMD_SIZE - 5)
/* Minimum wait for dmae opertaion to complete 2 milliseconds */
#define DMAE_MIN_WAIT_TIME	0x2
#define DMAE_MAX_CLIENTS	32

/**
* @brief ecore_gtt_init - Initialize GTT windows
*
* @param p_hwfn
*/
void ecore_gtt_init(struct ecore_hwfn *p_hwfn);

/**
 * @brief ecore_ptt_invalidate - Forces all ptt entries to be re-configured
 *
 * @param p_hwfn
 */
void ecore_ptt_invalidate(struct ecore_hwfn *p_hwfn);

/**
 * @brief ecore_ptt_pool_alloc - Allocate and initialize PTT pool
 *
 * @param p_hwfn
 *
 * @return _ecore_status_t - success (0), negative - error.
 */
enum _ecore_status_t ecore_ptt_pool_alloc(struct ecore_hwfn *p_hwfn);

/**
 * @brief ecore_ptt_pool_free -
 *
 * @param p_hwfn
 */
void ecore_ptt_pool_free(struct ecore_hwfn *p_hwfn);

/**
 * @brief ecore_ptt_get_hw_addr - Get PTT's GRC/HW address
 *
 * @param p_hwfn
 * @param p_ptt
 *
 * @return u32
 */
u32 ecore_ptt_get_hw_addr(struct ecore_hwfn	*p_hwfn,
			  struct ecore_ptt	*p_ptt);

/**
 * @brief ecore_ptt_get_bar_addr - Get PPT's external BAR address
 *
 * @param p_hwfn
 * @param p_ptt
 *
 * @return u32
 */
u32 ecore_ptt_get_bar_addr(struct ecore_ptt	*p_ptt);

/**
 * @brief ecore_ptt_set_win - Set PTT Window's GRC BAR address
 *
 * @param p_hwfn
 * @param new_hw_addr
 * @param p_ptt
 */
void ecore_ptt_set_win(struct ecore_hwfn	*p_hwfn,
		       struct ecore_ptt		*p_ptt,
		       u32			new_hw_addr);

/**
 * @brief ecore_get_reserved_ptt - Get a specific reserved PTT
 *
 * @param p_hwfn
 * @param ptt_idx
 *
 * @return struct ecore_ptt *
 */
struct ecore_ptt *ecore_get_reserved_ptt(struct ecore_hwfn	*p_hwfn,
					 enum reserved_ptts	ptt_idx);

/**
 * @brief ecore_wr - Write value to BAR using the given ptt
 *
 * @param p_hwfn
 * @param p_ptt
 * @param val
 * @param hw_addr
 */
void ecore_wr(struct ecore_hwfn	*p_hwfn,
	      struct ecore_ptt	*p_ptt,
	      u32		hw_addr,
	      u32		val);

/**
 * @brief ecore_rd - Read value from BAR using the given ptt
 *
 * @param p_hwfn
 * @param p_ptt
 * @param val
 * @param hw_addr
 */
u32 ecore_rd(struct ecore_hwfn	*p_hwfn,
	     struct ecore_ptt	*p_ptt,
	     u32		hw_addr);

/**
 * @brief ecore_memcpy_from - copy n bytes from BAR using the given
 *        ptt
 *
 * @param p_hwfn
 * @param p_ptt
 * @param dest
 * @param hw_addr
 * @param n
 */
void ecore_memcpy_from(struct ecore_hwfn	*p_hwfn,
		       struct ecore_ptt		*p_ptt,
		       void			*dest,
		       u32			hw_addr,
		       osal_size_t		n);

/**
 * @brief ecore_memcpy_to - copy n bytes to BAR using the given
 *        ptt
 *
 * @param p_hwfn
 * @param p_ptt
 * @param hw_addr
 * @param src
 * @param n
 */
void ecore_memcpy_to(struct ecore_hwfn	*p_hwfn,
		     struct ecore_ptt	*p_ptt,
		     u32		hw_addr,
		     void		*src,
		     osal_size_t	n);
/**
 * @brief ecore_fid_pretend - pretend to another function when
 *        accessing the ptt window. There is no way to unpretend
 *        a function. The only way to cancel a pretend is to
 *        pretend back to the original function.
 *
 * @param p_hwfn
 * @param p_ptt
 * @param fid - fid field of pxp_pretend structure. Can contain
 *            either pf / vf, port/path fields are don't care.
 */
void ecore_fid_pretend(struct ecore_hwfn	*p_hwfn,
		       struct ecore_ptt		*p_ptt,
		       u16			fid);

/**
 * @brief ecore_port_pretend - pretend to another port when
 *        accessing the ptt window
 *
 * @param p_hwfn
 * @param p_ptt
 * @param port_id - the port to pretend to
 */
void ecore_port_pretend(struct ecore_hwfn	*p_hwfn,
			struct ecore_ptt	*p_ptt,
			u8			port_id);

/**
 * @brief ecore_port_unpretend - cancel any previously set port
 *        pretend
 *
 * @param p_hwfn
 * @param p_ptt
 */
void ecore_port_unpretend(struct ecore_hwfn	*p_hwfn,
			  struct ecore_ptt	*p_ptt);

/**
 * @brief ecore_vfid_to_concrete - build a concrete FID for a
 *        given VF ID
 *
 * @param p_hwfn
 * @param p_ptt
 * @param vfid
 */
u32 ecore_vfid_to_concrete(struct ecore_hwfn *p_hwfn, u8 vfid);

/**
* @brief ecore_dmae_info_alloc - Init the dmae_info structure
* which is part of p_hwfn.
* @param p_hwfn
*/
enum _ecore_status_t ecore_dmae_info_alloc(struct ecore_hwfn	*p_hwfn);

/**
* @brief ecore_dmae_info_free - Free the dmae_info structure
* which is part of p_hwfn
*
* @param p_hwfn
*/
void ecore_dmae_info_free(struct ecore_hwfn	*p_hwfn);

union ecore_qm_pq_params {
	struct {
		u8 q_idx;
	} iscsi;

	struct {
		u8 tc;
	} core;

	struct {
		u8 is_vf;
		u8 vf_id;
		u8 tc;
	} eth;

	struct {
		u8 dcqcn;
		u8 qpid; /* roce relative */
	} roce;

	struct {
		u8 qidx;
	} iwarp;
};

u16 ecore_get_qm_pq(struct ecore_hwfn	*p_hwfn,
		    enum protocol_type	proto,
		    union ecore_qm_pq_params *params);

enum _ecore_status_t ecore_init_fw_data(struct ecore_dev *p_dev,
					const u8 *fw_data);

void ecore_hw_err_notify(struct ecore_hwfn *p_hwfn,
			 enum ecore_hw_err_type err_type);

#endif /* __ECORE_HW_H__ */