/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <device/pci_ops.h>
#include <device/smbus_host.h>
#include <southbridge/intel/lynxpoint/pch.h>

#include "diag.h"

struct smbus_snapshot {
	uint32_t id, command, bar, hostc, fd, cg;
	uint32_t host, data, pins, gpio_base, gpio_mode[3], gpio_level[3];
	bool valid_io;
};

static struct smbus_snapshot read_snapshot(void)
{
	struct smbus_snapshot s = {
		.id = pci_s_read_config32(PCH_SMBUS_DEV, 0),
		.command = pci_s_read_config16(PCH_SMBUS_DEV, 4),
		.bar = pci_s_read_config32(PCH_SMBUS_DEV, 0x20),
		.hostc = pci_s_read_config8(PCH_SMBUS_DEV, 0x40),
		.fd = RCBA32(FD),
		.cg = RCBA32(0x341c),
		.host = 0xffffffff,
		.data = 0xffffffff,
		.pins = 0xffffffff,
		.gpio_base = pci_s_read_config32(PCH_LPC_DEV, GPIOBASE),
	};
	const uint16_t io = s.bar & 0xffe0;

	s.valid_io = (s.id & 0xffff) == 0x8086 && (s.command & 1) &&
		(s.hostc & 1) && io && io != 0xffe0;
	if (s.valid_io) {
		/* Sample immediately, before slow POST/UART output can change timing. */
		s.host = inb(io) | inb(io + 2) << 8 | inb(io + 3) << 16 |
			(uint32_t)inb(io + 4) << 24;
		s.data = inb(io + 5) | inb(io + 6) << 8 | inb(io + 0x0c) << 16 |
			(uint32_t)inb(io + 0x0d) << 24;
		s.pins = inb(io + 0x0e) | inb(io + 0x0f) << 8;
	}
	const uint16_t gpio = s.gpio_base & 0xff80;
	const uint8_t gpio_control = pci_s_read_config8(PCH_LPC_DEV, GPIO_CNTL);

	for (unsigned int i = 0; i < 3; i++) {
		static const uint8_t mode[] = { 0x00, 0x30, 0x40 };
		static const uint8_t level[] = { 0x0c, 0x38, 0x48 };

		s.gpio_mode[i] = s.gpio_level[i] = 0xffffffff;
		if ((gpio_control & 0x10) && gpio && gpio != 0xff80) {
			s.gpio_mode[i] = inl(gpio + mode[i]);
			s.gpio_level[i] = inl(gpio + level[i]);
		}
	}
	return s;
}

static void emit_snapshot(unsigned int phase, const struct smbus_snapshot *s)
{
	p9di_diag_mark("SMBUS_PHASE", phase);
	p9di_diag_mark("SMBUS_PCI_ID", s->id);
	p9di_diag_mark("SMBUS_PCI_COMMAND", s->command);
	p9di_diag_mark("SMBUS_PCI_BAR", s->bar);
	p9di_diag_mark("SMBUS_STATIC_BASE", smbus_base());
	p9di_diag_mark("SMBUS_HOSTC", s->hostc);
	p9di_diag_mark("SMBUS_IO_VALID", s->valid_io);
	p9di_diag_mark("SMBUS_HOST_REGS", s->host);
	p9di_diag_mark("SMBUS_DATA_AUX", s->data);
	p9di_diag_mark("SMBUS_PIN_LEVELS", s->pins);
	p9di_diag_mark("SMBUS_RCBA_FD", s->fd);
	p9di_diag_mark("SMBUS_RCBA_CG", s->cg);
	p9di_diag_mark("SMBUS_GPIOBASE", s->gpio_base);
	for (unsigned int i = 0; i < 3; i++) {
		static const char *const modes[] = {
			"SMBUS_GPIO_MODE1", "SMBUS_GPIO_MODE2", "SMBUS_GPIO_MODE3",
		};
		static const char *const levels[] = {
			"SMBUS_GPIO_LEVEL1", "SMBUS_GPIO_LEVEL2", "SMBUS_GPIO_LEVEL3",
		};

		p9di_diag_mark(modes[i], s->gpio_mode[i]);
		p9di_diag_mark(levels[i], s->gpio_level[i]);
	}
}

void p9di_smbus_probe(bool before_gpio)
{
	if (before_gpio) {
		const struct smbus_snapshot initial = read_snapshot();

		emit_snapshot(0, &initial);
		/* Same initialization as early_pch_init(), only moved before GPIO for probing. */
		const int ret = smbus_enable_iobar(smbus_base());

		p9di_diag_mark("SMBUS_ENABLE_RESULT", ret);
		if (ret < 0)
			return;
		smbus_host_reset(smbus_base());
	}
	const struct smbus_snapshot before = read_snapshot();

	emit_snapshot(before_gpio ? 1 : 2, &before);
	if (!before.valid_io || (before.bar & 0xffe0) != smbus_base()) {
		p9di_diag_mark("SMBUS_PROBE_SKIPPED", before.bar);
		return;
	}
	/* Known SPD EEPROMs only. No scanning, EEPROM writes, or recovery toggles. */
	const uint8_t addresses[] = { 0x50, 0x52 };
	const uint8_t offsets[] = { 0, 2 };

	for (unsigned int i = 0; i < ARRAY_SIZE(addresses); i++) {
		for (unsigned int j = 0; j < ARRAY_SIZE(offsets); j++) {
			p9di_diag_mark("SMBUS_PROBE_ADDR", addresses[i] << 8 | offsets[j]);
			const int ret = smbus_read_byte(addresses[i], offsets[j]);
			const struct smbus_snapshot after = read_snapshot();

			p9di_diag_mark("SMBUS_PROBE_RESULT", ret);
			emit_snapshot(before_gpio ? 4 : 5, &after);
		}
	}
}

void p9di_smbus_read_error(int error)
{
	/* Preserve original controller status before emitting any diagnostic bytes. */
	const struct smbus_snapshot failed = read_snapshot();

	p9di_diag_mark("SPD_READ_STATUS", error);
	emit_snapshot(3, &failed);
}
