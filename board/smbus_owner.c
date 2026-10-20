/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/smbus_host.h>
#include <drivers/ipmi/ipmi_if.h>

#include "diag.h"
#include "smbus_owner.h"

bool p9di_claim_spd_bus(void)
{
	p9di_diag_mark("SPD_BUS_CLAIM_ENTER", 0);
	if (ipmi_premem_init(CONFIG_BMC_KCS_BASE, 0) != CB_SUCCESS)
		goto fail;

	/*
	 * ASMB7 GPIOE6 drives QU9 through Q34/Q35. Sensor scanning selects
	 * the BMC's I2C2 instead of the PCH's SMBus (I2C6) for the shared
	 * SPD/HWM bus. OemEnDisScanI2c(0) stops scanning and selects the PCH;
	 * its handler switches GPIOE6 before returning the completion code.
	 * Keep ownership through MRC. bmc.c returns it after HWM setup.
	 */
	const unsigned char disable = 0;
	struct ipmi_rsp response = {0};
	const int ret = ipmi_message(CONFIG_BMC_KCS_BASE, 0x30, 0, 0x02,
		&disable, sizeof(disable), (unsigned char *)&response, sizeof(response));

	if (ret != sizeof(response) || response.completion_code ||
	    response.cmd != 0x02 || response.lun != (0x31 << 2)) {
		printk(BIOS_ERR, "ASMB7: SPD bus handoff failed (%d/%02x)\n",
		       ret, response.completion_code);
		goto fail;
	}

	/* Discard status from transactions attempted while the mux selected BMC. */
	smbus_host_reset(smbus_base());
	p9di_diag_mark("SPD_BUS_CLAIM_OK", 0);
	return true;

fail:
	p9di_diag_mark("SPD_BUS_CLAIM_FAIL", 0);
	return false;
}
