/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/pnp_ops.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <superio/nuvoton/nct6779d/nct6779d.h>
#include <southbridge/intel/lynxpoint/pch.h>
#include <bootblock_common.h>

#include "diag.h"

void mainboard_config_superio(void)
{
	if (CONFIG(P9DI_DUAL_UART_DIAGNOSTICS)) {
		p9di_post_snapshot(0);
		p9di_diag_init();
		p9di_post_snapshot(1);
		p9di_diag_mark("BOOTBLOCK_SIO_READY", 0);
		return;
	}
	/* Pin multiplexing beyond COM A needs a board-specific register dump. */
	nuvoton_enable_serial(PNP_DEV(0x2e, NCT6779D_SP1), CONFIG_TTYS0_BASE);
}

void bootblock_mainboard_init(void)
{
	p9di_diag_mark("BOOTBLOCK_DONE", 0);
}
