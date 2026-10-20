/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <console/console.h>
#include <cpu/x86/smm.h>
#include <drivers/ipmi/ipmi_if.h>
#if ENV_RAMSTAGE
#include <bootstate.h>
#include "monitor.h"
#endif

/* Recovered from the stock BmcAcpi SMI handler: netfn 06, command 06. */
static void asmb7_power_state(uint8_t system, uint8_t device)
{
	const unsigned char request[] = { system, device };
	struct ipmi_rsp response = {0};
	const int ret = ipmi_message(CONFIG_BMC_KCS_BASE, IPMI_NETFN_APPLICATION,
		0, IPMI_CMD_ACPI_POWERON, request, sizeof(request),
		(unsigned char *)&response, sizeof(response));

	if (ret != sizeof(response) || response.completion_code)
		printk(BIOS_WARNING, "ASMB7: ACPI power state failed (%d/%02x)\n",
		       ret, response.completion_code);
}

#if ENV_SMM
void mainboard_smi_sleep(u8 slp_typ)
{
	switch (slp_typ) {
	case ACPI_S1:
		asmb7_power_state(0x81, 0x80);
		break;
	case ACPI_S3:
		asmb7_power_state(0x83, 0x83);
		break;
	case ACPI_S4:
		asmb7_power_state(0x84, 0x83);
		break;
	case ACPI_S5:
		asmb7_power_state(0x85, 0x83);
		break;
	}
}
#else
static void asmb7_s0(void *unused)
{
	const bool monitor_ready = p9di_monitor_init();

	asmb7_power_state(0x80, 0x80);
	if (!monitor_ready)
		return;

	/* Stock ASUS_IPMI_DXE: OemEnDisScanI2c in ASMB7 1.09, payload 1 enables. */
	const unsigned char enable = 1;
	struct ipmi_rsp response = {0};
	const int ret = ipmi_message(CONFIG_BMC_KCS_BASE, 0x30, 0, 0x02,
		&enable, sizeof(enable), (unsigned char *)&response, sizeof(response));

	if (ret != sizeof(response) || response.completion_code)
		printk(BIOS_WARNING, "ASMB7: sensor scan enable failed (%d/%02x)\n",
		       ret, response.completion_code);
}
BOOT_STATE_INIT_ENTRY(BS_DEV_INIT, BS_ON_EXIT, asmb7_s0, NULL);
#endif
