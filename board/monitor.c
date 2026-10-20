/* SPDX-License-Identifier: GPL-2.0-only */

#define __SIMPLE_DEVICE__

#include <arch/io.h>
#include <console/console.h>
#include <device/pnp_ops.h>
#include <superio/nuvoton/common/nuvoton.h>
#include <superio/nuvoton/nct6779d/nct6779d.h>

#include "monitor.h"

bool p9di_monitor_init(void)
{
	const pnp_devfn_t dev = PNP_DEV(0x2e, NCT6779D_HWM_FPLED);
	uint16_t base = 0;

	nuvoton_pnp_enter_conf_state(dev);
	const uint8_t saved_ldn = pnp_read_config(dev, 7);
	const uint16_t id = pnp_read_config(dev, 0x20) << 8 | pnp_read_config(dev, 0x21);

	if ((id & 0xfff0) == 0xc560) {
		pnp_set_logical_device(dev);
		if (pnp_read_enable(dev))
			base = pnp_read_iobase(dev, 0x60);
		if (base == 0x290) {
			/* P9D-I stock CR1A=0: pins 75/76 serve HWM SMBus, not GPIO31/32. */
			pnp_unset_and_set_config(dev, 0x1a, 0x30, 0);
		}
	}
	pnp_write_config(dev, 7, saved_ldn);
	nuvoton_pnp_exit_conf_state(dev);
	if (base != 0x290) {
		printk(BIOS_WARNING, "P9D-I: HWM not enabled at expected base; skipping monitor init\n");
		return false;
	}

	const uint16_t index = base + 5;
	const uint16_t data = base + 6;
	const uint8_t saved_index = inb(index);

	outb(0x4e, index);
	const uint8_t saved_bank = inb(data);

	/* The LPC and SMBus interfaces have separate bank selectors. */
	outb(saved_bank & 0xf0, data);
	for (unsigned int reg = 0x5d; reg <= 0x5e; reg++) {
		outb(reg, index);
		const uint8_t value = inb(data);

		/* CPUTIN is a thermistor on this board; stock clears bit 2 in both. */
		uint8_t configured = value & ~0x04;

		/* NCT6779D VBAT monitoring enable, also used by Linux nct6775. */
		if (reg == 0x5d)
			configured |= 0x01;
		outb(configured, data);
	}
	outb(0x4e, index);
	outb(saved_bank, data);
	outb(saved_index, index);
	printk(BIOS_INFO, "P9D-I: HWM SMBus, CPUTIN thermistor and VBAT monitoring configured\n");
	return true;
}
