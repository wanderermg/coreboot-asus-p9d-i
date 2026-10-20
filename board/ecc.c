/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <halt.h>
#include <northbridge/intel/haswell/haswell.h>

#include "diag.h"

void mb_late_romstage_setup(void)
{
	p9di_diag_mark("ECC_CHECK_ENTER", 0);
	for (unsigned int ch = 0; ch < 2; ch++) {
		const uint32_t cfg = mchbar_read32(MAD_DIMM(ch));

		p9di_diag_mark(ch ? "ECC_CH1" : "ECC_CH0", cfg);
		printk(BIOS_INFO, "P9D-I: channel %u MAD_DIMM=%08x ECC=%u\n",
		       ch, cfg, (cfg >> 24) & 3);
		if (CONFIG(P9DI_REQUIRE_ECC) && (cfg & 0xffff) &&
		    ((cfg >> 24) & 3) != 3) {
			p9di_diag_mark("ECC_CHECK_FAIL", ch);
			die("P9D-I: populated channel without full ECC; stopping.\n");
		}
	}
	p9di_diag_mark("ECC_CHECK_OK", 0);
}
