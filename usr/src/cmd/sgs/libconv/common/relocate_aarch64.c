/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

#include <sys/elf_AARCH64.h>
#include "_conv.h"
#include "relocate_aarch64_msg.h"

static const Msg rels[R_AARCH64_NUM] = {
	[R_AARCH64_NONE] =				MSG_R_AARCH64_NONE,
	[R_AARCH64_NONE_ALT] =				MSG_R_AARCH64_NONE_ALT,
	[R_AARCH64_ABS64] =				MSG_R_AARCH64_ABS64,
	[R_AARCH64_ABS32] =				MSG_R_AARCH64_ABS32,
	[R_AARCH64_ABS16] =				MSG_R_AARCH64_ABS16,
	[R_AARCH64_PREL64] =				MSG_R_AARCH64_PREL64,
	[R_AARCH64_PREL32] =				MSG_R_AARCH64_PREL32,
	[R_AARCH64_PREL16] =				MSG_R_AARCH64_PREL16,
	[R_AARCH64_MOVW_UABS_G0] =			MSG_R_AARCH64_MOVW_UABS_G0,
	[R_AARCH64_MOVW_UABS_G0_NC] =			MSG_R_AARCH64_MOVW_UABS_G0_NC,
	[R_AARCH64_MOVW_UABS_G1] =			MSG_R_AARCH64_MOVW_UABS_G1,
	[R_AARCH64_MOVW_UABS_G1_NC] =			MSG_R_AARCH64_MOVW_UABS_G1_NC,
	[R_AARCH64_MOVW_UABS_G2] =			MSG_R_AARCH64_MOVW_UABS_G2,
	[R_AARCH64_MOVW_UABS_G2_NC] =			MSG_R_AARCH64_MOVW_UABS_G2_NC,
	[R_AARCH64_MOVW_UABS_G3] =			MSG_R_AARCH64_MOVW_UABS_G3,
	[R_AARCH64_MOVW_SABS_G0] =			MSG_R_AARCH64_MOVW_SABS_G0,
	[R_AARCH64_MOVW_SABS_G1] =			MSG_R_AARCH64_MOVW_SABS_G1,
	[R_AARCH64_MOVW_SABS_G2] =			MSG_R_AARCH64_MOVW_SABS_G2,
	[R_AARCH64_LD_PREL_LO19] =			MSG_R_AARCH64_LD_PREL_LO19,
	[R_AARCH64_ADR_PREL_LO21] =			MSG_R_AARCH64_ADR_PREL_LO21,
	[R_AARCH64_ADR_PREL_PG_HI21] =			MSG_R_AARCH64_ADR_PREL_PG_HI21,
	[R_AARCH64_ADR_PREL_PG_HI21_NC] =		MSG_R_AARCH64_ADR_PREL_PG_HI21_NC,
	[R_AARCH64_ADD_ABS_LO12_NC] =			MSG_R_AARCH64_ADD_ABS_LO12_NC,
	[R_AARCH64_LDST8_ABS_LO12_NC] =			MSG_R_AARCH64_LDST8_ABS_LO12_NC,
	[R_AARCH64_LDST16_ABS_LO12_NC] =		MSG_R_AARCH64_LDST16_ABS_LO12_NC,
	[R_AARCH64_LDST32_ABS_LO12_NC] =		MSG_R_AARCH64_LDST32_ABS_LO12_NC,
	[R_AARCH64_LDST64_ABS_LO12_NC] =		MSG_R_AARCH64_LDST64_ABS_LO12_NC,
	[R_AARCH64_LDST128_ABS_LO12_NC] =		MSG_R_AARCH64_LDST128_ABS_LO12_NC,
	[R_AARCH64_TSTBR14] =				MSG_R_AARCH64_TSTBR14,
	[R_AARCH64_CONDBR19] =				MSG_R_AARCH64_CONDBR19,
	[R_AARCH64_JUMP26] =				MSG_R_AARCH64_JUMP26,
	[R_AARCH64_CALL26] =				MSG_R_AARCH64_CALL26,
	[R_AARCH64_MOVW_PREL_G0] =			MSG_R_AARCH64_MOVW_PREL_G0,
	[R_AARCH64_MOVW_PREL_G0_NC] =			MSG_R_AARCH64_MOVW_PREL_G0_NC,
	[R_AARCH64_MOVW_PREL_G1] =			MSG_R_AARCH64_MOVW_PREL_G1,
	[R_AARCH64_MOVW_PREL_G1_NC] =			MSG_R_AARCH64_MOVW_PREL_G1_NC,
	[R_AARCH64_MOVW_PREL_G2] =			MSG_R_AARCH64_MOVW_PREL_G2,
	[R_AARCH64_MOVW_PREL_G2_NC] =			MSG_R_AARCH64_MOVW_PREL_G2_NC,
	[R_AARCH64_MOVW_PREL_G3] =			MSG_R_AARCH64_MOVW_PREL_G3,
	[R_AARCH64_MOVW_GOTOFF_G0] =			MSG_R_AARCH64_MOVW_GOTOFF_G0,
	[R_AARCH64_MOVW_GOTOFF_G0_NC] =			MSG_R_AARCH64_MOVW_GOTOFF_G0_NC,
	[R_AARCH64_MOVW_GOTOFF_G1] =			MSG_R_AARCH64_MOVW_GOTOFF_G1,
	[R_AARCH64_MOVW_GOTOFF_G1_NC] =			MSG_R_AARCH64_MOVW_GOTOFF_G1_NC,
	[R_AARCH64_MOVW_GOTOFF_G2] =			MSG_R_AARCH64_MOVW_GOTOFF_G2,
	[R_AARCH64_MOVW_GOTOFF_G2_NC] =			MSG_R_AARCH64_MOVW_GOTOFF_G2_NC,
	[R_AARCH64_MOVW_GOTOFF_G3] =			MSG_R_AARCH64_MOVW_GOTOFF_G3,
	[R_AARCH64_MOVW_GOTREL64] =			MSG_R_AARCH64_MOVW_GOTREL64,
	[R_AARCH64_MOVW_GOTREL32] =			MSG_R_AARCH64_MOVW_GOTREL32,
	[R_AARCH64_GOT_LD_PREL19] =			MSG_R_AARCH64_GOT_LD_PREL19,
	[R_AARCH64_LD64_GOTOFF_LO15] =			MSG_R_AARCH64_LD64_GOTOFF_LO15,
	[R_AARCH64_ADR_GOT_PAGE] =			MSG_R_AARCH64_ADR_GOT_PAGE,
	[R_AARCH64_LD64_GOT_LO12_NC] =			MSG_R_AARCH64_LD64_GOT_LO12_NC,
	[R_AARCH64_LD64_GOTPAGE_LO15] =			MSG_R_AARCH64_LD64_GOTPAGE_LO15,
	[R_AARCH64_TLSGD_ADR_PREL21] =			MSG_R_AARCH64_TLSGD_ADR_PREL21,
	[R_AARCH64_TLSGD_ADR_PAGE21] =			MSG_R_AARCH64_TLSGD_ADR_PAGE21,
	[R_AARCH64_TLSGD_ADD_LO12_NC] =			MSG_R_AARCH64_TLSGD_ADD_LO12_NC,
	[R_AARCH64_TLSGD_MOVW_G1] =			MSG_R_AARCH64_TLSGD_MOVW_G1,
	[R_AARCH64_TLSGD_MOVW_G0_NC] =			MSG_R_AARCH64_TLSGD_MOVW_G0_NC,
	[R_AARCH64_TLSLD_ADR_PREL21] =			MSG_R_AARCH64_TLSLD_ADR_PREL21,
	[R_AARCH64_TLSLD_ADR_PAGE21] =			MSG_R_AARCH64_TLSLD_ADR_PAGE21,
	[R_AARCH64_TLSLD_ADD_LO12_NC] =			MSG_R_AARCH64_TLSLD_ADD_LO12_NC,
	[R_AARCH64_TLLGD_MOVW_G1] =			MSG_R_AARCH64_TLLGD_MOVW_G1,
	[R_AARCH64_TLSLD_MOVW_G0_NC] =			MSG_R_AARCH64_TLSLD_MOVW_G0_NC,
	[R_AARCH64_TLSLD_LD_PREL19] =			MSG_R_AARCH64_TLSLD_LD_PREL19,
	[R_AARCH64_TLSLD_MOVW_DTPREL_G2] =		MSG_R_AARCH64_TLSLD_MOVW_DTPREL_G2,
	[R_AARCH64_TLSLD_MOVW_DTPREL_G1] =		MSG_R_AARCH64_TLSLD_MOVW_DTPREL_G1,
	[R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC] =		MSG_R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC,
	[R_AARCH64_TLSLD_MOVW_DTPREL_G0] =		MSG_R_AARCH64_TLSLD_MOVW_DTPREL_G0,
	[R_AARCH64_TLSLD_MOVW_DTPREL_G0_NC] =		MSG_R_AARCH64_TLSLD_MOVW_DTPREL_G0_NC,
	[R_AARCH64_TLSLD_ADD_DTPREL_HI12] =		MSG_R_AARCH64_TLSLD_ADD_DTPREL_HI12,
	[R_AARCH64_TLSLD_ADD_DTPREL_LO12] =		MSG_R_AARCH64_TLSLD_ADD_DTPREL_LO12,
	[R_AARCH64_TLSLD_ADD_DTPREL_LO12_NC] =		MSG_R_AARCH64_TLSLD_ADD_DTPREL_LO12_NC,
	[R_AARCH64_TLSLD_LDST8_DTPREL_LO12] =		MSG_R_AARCH64_TLSLD_LDST8_DTPREL_LO12,
	[R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC] =	MSG_R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC,
	[R_AARCH64_TLSLD_LDST16_DTPREL_LO12] =		MSG_R_AARCH64_TLSLD_LDST16_DTPREL_LO12,
	[R_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC] =	MSG_R_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC,
	[R_AARCH64_TLSLD_LDST32_DTPREL_LO12] =		MSG_R_AARCH64_TLSLD_LDST32_DTPREL_LO12,
	[R_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC] =	MSG_R_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC,
	[R_AARCH64_TLSLD_LDST64_DTPREL_LO12] =		MSG_R_AARCH64_TLSLD_LDST64_DTPREL_LO12,
	[R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC] =	MSG_R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC,
	[R_AARCH64_TLSLD_LDST128_DTPREL_LO12] =		MSG_R_AARCH64_TLSLD_LDST128_DTPREL_LO12,
	[R_AARCH64_TLSLD_LDST128_DTPREL_LO12_NC] =	MSG_R_AARCH64_TLSLD_LDST128_DTPREL_LO12_NC,
	[R_AARCH64_TLSIE_MOVW_GOTTPREL_G1] =		MSG_R_AARCH64_TLSIE_MOVW_GOTTPREL_G1,
	[R_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC] =		MSG_R_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC,
	[R_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21] =		MSG_R_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21,
	[R_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC] =	MSG_R_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC,
	[R_AARCH64_TLSIE_LD_GOTTPREL_PREL19] =		MSG_R_AARCH64_TLSIE_LD_GOTTPREL_PREL19,
	[R_AARCH64_TLSLE_MOVW_TPREL_G2] =		MSG_R_AARCH64_TLSLE_MOVW_TPREL_G2,
	[R_AARCH64_TLSLE_MOVW_TPREL_G1] =		MSG_R_AARCH64_TLSLE_MOVW_TPREL_G1,
	[R_AARCH64_TLSLE_MOVW_TPREL_G1_NC] =		MSG_R_AARCH64_TLSLE_MOVW_TPREL_G1_NC,
	[R_AARCH64_TLSLE_MOVW_TPREL_G0] =		MSG_R_AARCH64_TLSLE_MOVW_TPREL_G0,
	[R_AARCH64_TLSLE_MOVW_TPREL_G0_NC] =		MSG_R_AARCH64_TLSLE_MOVW_TPREL_G0_NC,
	[R_AARCH64_TLSLE_ADD_TPREL_HI12] =		MSG_R_AARCH64_TLSLE_ADD_TPREL_HI12,
	[R_AARCH64_TLSLE_ADD_TPREL_LO12] =		MSG_R_AARCH64_TLSLE_ADD_TPREL_LO12,
	[R_AARCH64_TLSLE_ADD_TPREL_LO12_NC] =		MSG_R_AARCH64_TLSLE_ADD_TPREL_LO12_NC,
	[R_AARCH64_TLSLE_LDST8_TPREL_LO12] =		MSG_R_AARCH64_TLSLE_LDST8_TPREL_LO12,
	[R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC] =		MSG_R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC,
	[R_AARCH64_TLSLE_LDST16_TPREL_LO12] =		MSG_R_AARCH64_TLSLE_LDST16_TPREL_LO12,
	[R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC] =	MSG_R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC,
	[R_AARCH64_TLSLE_LDST32_TPREL_LO12] =		MSG_R_AARCH64_TLSLE_LDST32_TPREL_LO12,
	[R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC] =	MSG_R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC,
	[R_AARCH64_TLSLE_LDST64_TPREL_LO12] =		MSG_R_AARCH64_TLSLE_LDST64_TPREL_LO12,
	[R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC] =	MSG_R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC,
	[R_AARCH64_TLSLE_LDST128_TPREL_LO12] =		MSG_R_AARCH64_TLSLE_LDST128_TPREL_LO12,
	[R_AARCH64_TLSLE_LDST128_TPREL_LO12_NC] =	MSG_R_AARCH64_TLSLE_LDST128_TPREL_LO12_NC,
	[R_AARCH64_TLSDESC_LD_PREL19] =			MSG_R_AARCH64_TLSDESC_LD_PREL19,
	[R_AARCH64_TLSDESC_ADR_PREL21] =		MSG_R_AARCH64_TLSDESC_ADR_PREL21,
	[R_AARCH64_TLSDESC_ADR_PAGE21] =		MSG_R_AARCH64_TLSDESC_ADR_PAGE21,
	[R_AARCH64_TLSDESC_LD64_LO12] =			MSG_R_AARCH64_TLSDESC_LD64_LO12,
	[R_AARCH64_TLSDESC_ADD_LO12] =			MSG_R_AARCH64_TLSDESC_ADD_LO12,
	[R_AARCH64_TLSDESC_OFF_G1] =			MSG_R_AARCH64_TLSDESC_OFF_G1,
	[R_AARCH64_TLSDESC_OFF_G0_NC] =			MSG_R_AARCH64_TLSDESC_OFF_G0_NC,
	[R_AARCH64_TLSDESC_LDR] =			MSG_R_AARCH64_TLSDESC_LDR,
	[R_AARCH64_TLSDESC_ADD] =			MSG_R_AARCH64_TLSDESC_ADD,
	[R_AARCH64_TLSDESC_CALL] =			MSG_R_AARCH64_TLSDESC_CALL,
	[R_AARCH64_COPY] =				MSG_R_AARCH64_COPY,
	[R_AARCH64_GLOB_DAT] =				MSG_R_AARCH64_GLOB_DAT,
	[R_AARCH64_JUMP_SLOT] =				MSG_R_AARCH64_JUMP_SLOT,
	[R_AARCH64_RELATIVE] =				MSG_R_AARCH64_RELATIVE,
	[R_AARCH64_TLS_DTPREL64] =			MSG_R_AARCH64_TLS_DTPREL64,
	[R_AARCH64_TLS_DTPMOD64] =			MSG_R_AARCH64_TLS_DTPMOD64,
	[R_AARCH64_TLS_TPREL64] =			MSG_R_AARCH64_TLS_TPREL64,
	[R_AARCH64_TLSDESC] =				MSG_R_AARCH64_TLSDESC,
	[R_AARCH64_TLS_IRELATIVE] =			MSG_R_AARCH64_TLS_IRELATIVE,
};

#if (R_AARCH64_NUM != 1033)
#error "R_AARCH64_NUM has grown"
#endif

const char *
conv_reloc_aarch64_type(Word type, Conv_fmt_flags_t fmt_flags,
    Conv_inv_buf_t *inv_buf)
{
	if ((type >= R_AARCH64_NUM) || (rels[type] == NULL))
		return (conv_invalid_val(inv_buf, type, fmt_flags));

	return (MSG_ORIG(rels[type]));
}
