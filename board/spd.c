/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/dram/ddr3.h>
#include <device/smbus_host.h>
#include <halt.h>
#include <northbridge/intel/haswell/raminit.h>
#include <spd.h>
#include <string.h>

#include "diag.h"
#include "smbus_owner.h"

void mb_read_spd(const struct spd_info *spdi, uint8_t data[4][SPD_SIZE_MAX_DDR3])
{
	const uint8_t expected_addresses[] = { 0x50, 0, 0x52, 0 };

	if (memcmp(spdi->addresses, expected_addresses, sizeof(expected_addresses)))
		die("P9D-I: unexpected SPD address map.\n");
	if (!p9di_claim_spd_bus())
		die("P9D-I: cannot acquire SPD bus from ASMB7; stopping.\n");

	/* This port requires ECC UDIMMs in both physical slots. */
	for (unsigned int ch = 0; ch < 2; ch++) {
		p9di_diag_mark("SPD_CHANNEL_ENTER", ch);
		const unsigned int slot = ch * 2;
		uint8_t *spd = data[slot];

		for (unsigned int offset = 0; offset < SPD_SIZE_MAX_DDR3; offset++) {
			if (!(offset & 31))
				p9di_diag_mark("SPD_PROGRESS", ch << 16 | offset);
			const int value = smbus_read_byte(spdi->addresses[slot], offset);

			if (value < 0) {
				p9di_smbus_read_error(value);
				p9di_diag_mark("SPD_READ_FAIL", ch << 16 | offset);
				printk(BIOS_ERR, "P9D-I: SPD %02x byte %u read failed (%d)\n",
				       spdi->addresses[slot], offset, value);
				die("P9D-I: cannot validate live SPD; stopping.\n");
			}
			spd[offset] = value;
		}

		const uint16_t stored_crc = spd[126] | spd[127] << 8;

		if (spd[2] != SPD_MEMORY_TYPE_SDRAM_DDR3 ||
		    spd_ddr3_calc_crc(spd, SPD_SIZE_MAX_DDR3) != stored_crc) {
			p9di_diag_mark("SPD_CRC_FAIL", ch);
			die("P9D-I: invalid DDR3 SPD or CRC; stopping.\n");
		}
		if ((spd[3] & SPD_DDR3_DIMM_TYPE_MASK) != SPD_DDR3_DIMM_TYPE_UDIMM ||
		    (spd[8] & 7) != 3) {
			p9di_diag_mark("SPD_TYPE_FAIL", ch);
			die("P9D-I: requires 64-bit DDR3 UDIMMs.\n");
		}
		if (CONFIG(P9DI_REQUIRE_ECC) && ((spd[8] >> 3) & 3) != 1) {
			p9di_diag_mark("SPD_ECC_FAIL", ch);
			die("P9D-I: DIMM lacks 8-bit ECC extension; stopping.\n");
		}

		p9di_diag_mark("SPD_CHANNEL_OK", ch);
		printk(BIOS_INFO, "P9D-I: channel %u SPD %02x CRC OK, %.18s "
		       "serial %02x%02x%02x%02x, width 64+%u\n", ch,
		       spdi->addresses[slot], (char *)&spd[128], spd[122], spd[123],
		       spd[124], spd[125], ((spd[8] >> 3) & 3) * 8);
	}
}
