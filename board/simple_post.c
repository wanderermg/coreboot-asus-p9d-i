/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <northbridge/intel/haswell/raminit.h>
#include <string.h>

#include "diag.h"

void p9di_diag_mark(const char *event, uint32_t value)
{
	static const struct {
		const char *event;
		uint8_t code;
	} checkpoints[] = {
		{ "SPD_BUS_CLAIM_ENTER", 0xd0 },
		{ "SPD_BUS_CLAIM_OK", 0xd1 },
		{ "SPD_BUS_CLAIM_FAIL", 0xd2 },
		{ "ROMSTAGE_ENTRY", 0xe0 },
		{ "PCH_READY", 0xe1 },
		{ "NORTHBRIDGE_READY", 0xe2 },
		{ "RAMINIT_ENTER", 0xe3 },
		{ "SPD_ENTER", 0xe4 },
		{ "SPD_READY", 0xe9 },
		{ "MRC_MAP", 0xea },
		{ "MRC_ENTER", 0xeb },
		{ "CBMEM_ENTER", 0xf0 },
		{ "CBMEM_READY", 0xf1 },
		{ "RAMINIT_COMPLETE", 0xf2 },
		{ "ECC_CHECK_ENTER", 0xf3 },
		{ "ECC_CH0", 0xf4 },
		{ "ECC_CH1", 0xf5 },
		{ "ECC_CHECK_OK", 0xf6 },
	};
	static const struct {
		const char *event;
		uint8_t code;
	} spd_errors[] = {
		{ "SPD_CRC_FAIL", 0xc2 },
		{ "SPD_TYPE_FAIL", 0xc4 },
		{ "SPD_ECC_FAIL", 0xc6 },
		{ "SPD_REF_FAIL", 0xc8 },
	};

	for (unsigned int i = 0; i < ARRAY_SIZE(checkpoints); i++) {
		if (!strcmp(event, checkpoints[i].event)) {
			outb(checkpoints[i].code, 0x80);
			return;
		}
	}
	if (!strcmp(event, "SPD_CHANNEL_ENTER") && value < 2)
		outb(value ? 0xe7 : 0xe5, 0x80);
	else if (!strcmp(event, "SPD_CHANNEL_OK") && value < 2)
		outb(value ? 0xe8 : 0xe6, 0x80);
	else if (!strcmp(event, "SPD_READ_FAIL") && (value >> 16) < 2)
		outb(0xc0 + (value >> 16), 0x80);
	else if (!strcmp(event, "MRC_RETURN"))
		outb(value ? 0xed : 0xec, 0x80);
	else if (!strcmp(event, "MRC_INIT_STATE"))
		outb(value & (1 << 5) ? 0xee : 0xef, 0x80);
	else if (!strcmp(event, "ECC_CHECK_FAIL") && value < 2)
		outb(value ? 0xf8 : 0xf7, 0x80);
	else if (value < 2) {
		for (unsigned int i = 0; i < ARRAY_SIZE(spd_errors); i++) {
			if (!strcmp(event, spd_errors[i].event)) {
				outb(spd_errors[i].code + value, 0x80);
				return;
			}
		}
	}
}

void mb_raminit_status(const char *event, uint32_t value)
{
	p9di_diag_mark(event, value);
}
