/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <device/pci_ops.h>
#include <device/pnp_ops.h>
#include <northbridge/intel/haswell/raminit.h>
#include <southbridge/intel/lynxpoint/pch.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <string.h>
#if ENV_RAMSTAGE
#include <bootstate.h>
#endif

#include "diag.h"

static const uint16_t ports[] = { 0x3f8, 0x2f8 };

static void enable_decodes(void)
{
	/* early_pch_init() resets this to COM A only; restore B before probing it. */
	pci_s_write_config16(PCH_LPC_DEV, LPC_IO_DEC,
		(pci_s_read_config16(PCH_LPC_DEV, LPC_IO_DEC) & ~0x77) | 0x10);
	pci_s_write_config16(PCH_LPC_DEV, LPC_EN,
		pci_s_read_config16(PCH_LPC_DEV, LPC_EN) | COMA_LPC_EN | COMB_LPC_EN);
}

static void init_uart(uint16_t base)
{
	/* 1.8432 MHz / 16 / 2 = 57600, 8N1, no hardware flow control. */
	outb(3, base + 3);
	outb(0, base + 1);
	outb(1, base + 2);
	outb(3, base + 4);
	outb(0x83, base + 3);
	outb(2, base);
	outb(0, base + 1);
	outb(3, base + 3);
}

static bool send_byte(uint16_t base, unsigned char byte)
{
	/* Bound every wait; skip this port's remaining marker on failure. */
	for (unsigned int n = 0; n < 10000; n++) {
		const uint8_t status = inb(base + 5);

		if (status == 0xff)
			return false;
		if (status & 0x20) {
			outb(byte, base);
			return true;
		}
	}
	return false;
}

static bool send_string(uint16_t base, const char *s)
{
	while (*s) {
		if (!send_byte(base, *s++))
			return false;
	}
	return true;
}

#if ENV_BOOTBLOCK || ENV_SEPARATE_ROMSTAGE
void p9di_diag_init(void)
{
	const pnp_devfn_t sio = PNP_DEV(0x2e, 0);

	enable_decodes();
	nuvoton_pnp_enter_conf_state(sio);
	const uint8_t old_ldn = pnp_read_config(sio, 7);
	const uint16_t chip_id = pnp_read_config(sio, 0x20) << 8 |
		pnp_read_config(sio, 0x21);

	if ((chip_id & 0xfff0) == 0xc560) {
		/* Preserve other pinmux bits; matches the stock COM A setup. */
		pnp_unset_and_set_config(sio, 0x2a, 0x80, 0);
		for (unsigned int i = 0; i < ARRAY_SIZE(ports); i++) {
			const pnp_devfn_t dev = PNP_DEV(0x2e, 2 + i);

			pnp_set_logical_device(dev);
			pnp_set_enable(dev, 0);
			pnp_set_iobase(dev, PNP_IDX_IO0, ports[i]);
			pnp_set_irq(dev, 0x70, i ? 3 : 4);
			pnp_set_enable(dev, 1);
		}
	}
	pnp_write_config(sio, 7, old_ldn);
	nuvoton_pnp_exit_conf_state(sio);
	p9di_diag_mark("SIO_ID", chip_id);
}
#endif

void p9di_diag_mark(const char *event, uint32_t value)
{
	static const char hex[] = "0123456789abcdef";

	p9di_post_mark(event, value);

	enable_decodes();

	for (unsigned int i = 0; i < ARRAY_SIZE(ports); i++) {
		const uint16_t base = ports[i];

		init_uart(base);
		if (!send_string(base, i ? "\r\n[P9DI-D8 COM2@2f8] " :
					  "\r\n[P9DI-D8 COM1@3f8] ") ||
		    !send_string(base, event) || !send_string(base, " 0x"))
			continue;
		bool ok = true;

		for (int bit = 28; bit >= 0; bit -= 4) {
			if (!send_byte(base, hex[(value >> bit) & 0xf])) {
				ok = false;
				break;
			}
		}
		if (ok)
			send_string(base, "\r\n");
	}
}

#if ENV_SEPARATE_ROMSTAGE
void mb_raminit_status(const char *event, uint32_t value)
{
	if (!strcmp(event, "PCH_READY"))
		p9di_diag_init();
	p9di_diag_mark(event, value);
	if (!strcmp(event, "ROMSTAGE_ENTRY"))
		p9di_smbus_probe(true);
	if (!strcmp(event, "PCH_READY"))
		p9di_smbus_probe(false);
	if (!strcmp(event, "NORTHBRIDGE_READY"))
		p9di_post_snapshot(2);
}
#endif

#if ENV_RAMSTAGE
static void diag_bootstate(void *event)
{
	p9di_diag_mark(event, 0);
}
BOOT_STATE_INIT_ENTRY(BS_PRE_DEVICE, BS_ON_ENTRY, diag_bootstate, (void *)"RAMSTAGE_ENTRY");
BOOT_STATE_INIT_ENTRY(BS_DEV_ENUMERATE, BS_ON_ENTRY, diag_bootstate, (void *)"DEVICE_ENUM_ENTER");
BOOT_STATE_INIT_ENTRY(BS_DEV_ENUMERATE, BS_ON_EXIT, diag_bootstate, (void *)"DEVICE_ENUM_DONE");
BOOT_STATE_INIT_ENTRY(BS_DEV_INIT, BS_ON_ENTRY, diag_bootstate, (void *)"DEVICE_INIT_ENTER");
BOOT_STATE_INIT_ENTRY(BS_DEV_INIT, BS_ON_EXIT, diag_bootstate, (void *)"DEVICE_INIT_DONE");
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_ENTRY, diag_bootstate, (void *)"PAYLOAD_LOAD");
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_BOOT, BS_ON_ENTRY, diag_bootstate, (void *)"EDK2_HANDOFF");
#endif
