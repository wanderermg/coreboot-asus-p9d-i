/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <delay.h>
#include <device/pci_ops.h>
#include <device/pnp_ops.h>
#include <southbridge/intel/lynxpoint/pch.h>
#include <string.h>
#include <superio/nuvoton/common/nuvoton.h>

#include "diag.h"
#include "post_ids.h"

static void post_frame(uint8_t id, uint32_t value)
{
	static uint8_t sequence;
	uint8_t frame[] = { 0xd4, 0x52, sequence++, id, id ^ 0xff,
		value, value >> 8, value >> 16, value >> 24, 0, 0xd5 };
	uint8_t crc = 0;

	for (unsigned int i = 0; i < 9; i++) {
		crc ^= frame[i];
		for (unsigned int bit = 0; bit < 8; bit++)
			crc = (crc << 1) ^ ((crc & 0x80) ? 7 : 0);
	}
	frame[9] = crc;
	for (unsigned int i = 0; i < sizeof(frame); i++) {
		outb(frame[i], 0x80);
		/* Do not outrun the AST2300 snoop interrupt handler. */
		mdelay(5);
	}
}

void p9di_post_mark(const char *event, uint32_t value)
{
	static const struct {
		const char *name;
		uint8_t id;
	} events[] = {
		{ "SIO_ID", P9DI_POST_SIO_ID },
		{ "BOOTBLOCK_SIO_READY", P9DI_POST_BOOTBLOCK_SIO_READY },
		{ "BOOTBLOCK_DONE", P9DI_POST_BOOTBLOCK_DONE },
		{ "ROMSTAGE_ENTRY", P9DI_POST_ROMSTAGE_ENTRY },
		{ "PCH_READY", P9DI_POST_PCH_READY },
		{ "NORTHBRIDGE_READY", P9DI_POST_NORTHBRIDGE_READY },
		{ "PLATFORM_INFO_ENTER", P9DI_POST_PLATFORM_INFO_ENTER },
		{ "PLATFORM_INFO_DONE", P9DI_POST_PLATFORM_INFO_DONE },
		{ "RAMINIT_ENTER", P9DI_POST_RAMINIT_ENTER },
		{ "SPD_ENTER", P9DI_POST_SPD_ENTER },
		{ "SPD_CHANNEL_ENTER", P9DI_POST_SPD_CHANNEL_ENTER },
		{ "SPD_READ_FAIL", P9DI_POST_SPD_READ_FAIL },
		{ "SPD_CRC_FAIL", P9DI_POST_SPD_CRC_FAIL },
		{ "SPD_TYPE_FAIL", P9DI_POST_SPD_TYPE_FAIL },
		{ "SPD_ECC_FAIL", P9DI_POST_SPD_ECC_FAIL },
		{ "SPD_REF_ENTER", P9DI_POST_SPD_REF_ENTER },
		{ "SPD_REF_FAIL", P9DI_POST_SPD_REF_FAIL },
		{ "SPD_CHANNEL_OK", P9DI_POST_SPD_CHANNEL_OK },
		{ "SPD_READY", P9DI_POST_SPD_READY },
		{ "MRC_CACHE_ENTER", P9DI_POST_MRC_CACHE_ENTER },
		{ "MRC_CACHE_READY", P9DI_POST_MRC_CACHE_READY },
		{ "MRC_MAP", P9DI_POST_MRC_MAP },
		{ "MRC_ENTER", P9DI_POST_MRC_ENTER },
		{ "MRC_RETURN", P9DI_POST_MRC_RETURN },
		{ "MRC_VERSION", P9DI_POST_MRC_VERSION },
		{ "MRC_INIT_STATE", P9DI_POST_MRC_INIT_STATE },
		{ "CBMEM_ENTER", P9DI_POST_CBMEM_ENTER },
		{ "CBMEM_READY", P9DI_POST_CBMEM_READY },
		{ "RAMINIT_COMPLETE", P9DI_POST_RAMINIT_COMPLETE },
		{ "ECC_CHECK_ENTER", P9DI_POST_ECC_CHECK_ENTER },
		{ "ECC_CH0", P9DI_POST_ECC_CH0 },
		{ "ECC_CH1", P9DI_POST_ECC_CH1 },
		{ "ECC_CHECK_FAIL", P9DI_POST_ECC_CHECK_FAIL },
		{ "ECC_CHECK_OK", P9DI_POST_ECC_CHECK_OK },
		{ "RAMSTAGE_ENTRY", P9DI_POST_RAMSTAGE_ENTRY },
		{ "DEVICE_ENUM_ENTER", P9DI_POST_DEVICE_ENUM_ENTER },
		{ "DEVICE_ENUM_DONE", P9DI_POST_DEVICE_ENUM_DONE },
		{ "DEVICE_INIT_ENTER", P9DI_POST_DEVICE_INIT_ENTER },
		{ "DEVICE_INIT_DONE", P9DI_POST_DEVICE_INIT_DONE },
		{ "PAYLOAD_LOAD", P9DI_POST_PAYLOAD_LOAD },
		{ "EDK2_HANDOFF", P9DI_POST_EDK2_HANDOFF },
		{ "SPD_PROGRESS", P9DI_POST_SPD_PROGRESS },
		{ "SNAPSHOT_ENTER", P9DI_POST_SNAPSHOT_ENTER },
		{ "SNAPSHOT_DONE", P9DI_POST_SNAPSHOT_DONE },
		{ "AST_UNAVAILABLE", P9DI_POST_AST_UNAVAILABLE },
		{ "AST_READ_ENTER", P9DI_POST_AST_READ_ENTER },
		{ "AST_READ_DONE", P9DI_POST_AST_READ_DONE },
		{ "SMBUS_PHASE", P9DI_POST_SMBUS_PHASE },
		{ "SMBUS_PCI_ID", P9DI_POST_SMBUS_PCI_ID },
		{ "SMBUS_PCI_COMMAND", P9DI_POST_SMBUS_PCI_COMMAND },
		{ "SMBUS_PCI_BAR", P9DI_POST_SMBUS_PCI_BAR },
		{ "SMBUS_STATIC_BASE", P9DI_POST_SMBUS_STATIC_BASE },
		{ "SMBUS_HOSTC", P9DI_POST_SMBUS_HOSTC },
		{ "SMBUS_IO_VALID", P9DI_POST_SMBUS_IO_VALID },
		{ "SMBUS_HOST_REGS", P9DI_POST_SMBUS_HOST_REGS },
		{ "SMBUS_DATA_AUX", P9DI_POST_SMBUS_DATA_AUX },
		{ "SMBUS_PIN_LEVELS", P9DI_POST_SMBUS_PIN_LEVELS },
		{ "SMBUS_RCBA_FD", P9DI_POST_SMBUS_RCBA_FD },
		{ "SMBUS_RCBA_CG", P9DI_POST_SMBUS_RCBA_CG },
		{ "SMBUS_GPIOBASE", P9DI_POST_SMBUS_GPIOBASE },
		{ "SMBUS_GPIO_MODE1", P9DI_POST_SMBUS_GPIO_MODE1 },
		{ "SMBUS_GPIO_MODE2", P9DI_POST_SMBUS_GPIO_MODE2 },
		{ "SMBUS_GPIO_MODE3", P9DI_POST_SMBUS_GPIO_MODE3 },
		{ "SMBUS_GPIO_LEVEL1", P9DI_POST_SMBUS_GPIO_LEVEL1 },
		{ "SMBUS_GPIO_LEVEL2", P9DI_POST_SMBUS_GPIO_LEVEL2 },
		{ "SMBUS_GPIO_LEVEL3", P9DI_POST_SMBUS_GPIO_LEVEL3 },
		{ "SMBUS_ENABLE_RESULT", P9DI_POST_SMBUS_ENABLE_RESULT },
		{ "SMBUS_PROBE_SKIPPED", P9DI_POST_SMBUS_PROBE_SKIPPED },
		{ "SMBUS_PROBE_ADDR", P9DI_POST_SMBUS_PROBE_ADDR },
		{ "SMBUS_PROBE_RESULT", P9DI_POST_SMBUS_PROBE_RESULT },
		{ "SPD_READ_STATUS", P9DI_POST_SPD_READ_STATUS },
	};

	for (unsigned int i = 0; i < ARRAY_SIZE(events); i++) {
		if (!strcmp(event, events[i].name)) {
			post_frame(events[i].id, value);
			return;
		}
	}
	/* Unknown events remain visible without silently dropping their values. */
	uint32_t hash = 2166136261U;

	for (const char *s = event; *s; s++)
		hash = (hash ^ (uint8_t)*s) * 16777619U;
	post_frame(0x7e, hash);
	post_frame(0x7f, value);
}

#if ENV_BOOTBLOCK || ENV_SEPARATE_ROMSTAGE
static uint32_t read_packed(pnp_devfn_t dev, const uint8_t regs[4])
{
	uint32_t value = 0;

	for (unsigned int i = 0; i < 4; i++)
		value |= (uint32_t)pnp_read_config(dev, regs[i]) << (i * 8);
	return value;
}

static void snapshot_nct(void)
{
	const pnp_devfn_t dev = PNP_DEV(0x2e, 0);
	const uint8_t globals[] = { 0x1b, 0x2a, 0x2b, 0x2c };
	const uint8_t config[] = { 0x30, 0x60, 0x61, 0x70 };
	const uint8_t clocks[] = { 0xf0, 0xf1, 0xf2, 0x2f };
	uint32_t uart[2], clock[2];

	nuvoton_pnp_enter_conf_state(dev);
	const uint8_t old_ldn = pnp_read_config(dev, 7);
	const uint16_t id = pnp_read_config(dev, 0x20) << 8 |
		pnp_read_config(dev, 0x21);
	const uint32_t global = read_packed(dev, globals);

	if ((id & 0xfff0) == 0xc560) {
		for (unsigned int i = 0; i < 2; i++) {
			pnp_write_config(dev, 7, 2 + i);
			uart[i] = read_packed(dev, config);
			clock[i] = read_packed(dev, clocks);
		}
	}
	pnp_write_config(dev, 7, old_ldn);
	nuvoton_pnp_exit_conf_state(dev);
	post_frame(P9DI_POST_NCT_ID, id);
	post_frame(P9DI_POST_NCT_GLOBAL, global);
	if ((id & 0xfff0) != 0xc560)
		return;
	for (unsigned int i = 0; i < 2; i++) {
		post_frame(i ? P9DI_POST_NCT_UART_B : P9DI_POST_NCT_UART_A, uart[i]);
		/* Keep the complete register bytes for each UART. */
		post_frame(i ? P9DI_POST_NCT_UART_B_CLOCK : P9DI_POST_NCT_UART_A_CLOCK,
			clock[i]);
	}
}

static uint32_t ast_read(pnp_devfn_t dev, uint32_t address)
{
	uint32_t value = 0;

	for (unsigned int i = 0; i < 4; i++)
		pnp_write_config(dev, 0xf0 + i, address >> (24 - i * 8));
	/* Reading FE triggers a read. Never write FE: that would write AHB. */
	(void)pnp_read_config(dev, 0xfe);
	for (unsigned int i = 0; i < 4; i++)
		value = value << 8 | pnp_read_config(dev, 0xf4 + i);
	return value;
}

static void snapshot_ast(void)
{
	const pnp_devfn_t dev = PNP_DEV(0x4e, 0);
	const uint8_t config[] = { 0x30, 0x60, 0x61, 0x70 };
	const uint8_t scratch_regs[] = { 0x20, 0x21, 0x22, 0x23 };
	const uint8_t ldns[] = { 2, 3, 0x0b, 0x0c };
	static const struct {
		uint32_t address;
		uint8_t id;
	} regs[] = {
		{ 0x1e789000, P9DI_POST_AST_HICR0 },
		{ 0x1e789008, P9DI_POST_AST_HICR2 },
		{ 0x1e789010, P9DI_POST_AST_HICR4 },
		{ 0x1e789080, P9DI_POST_AST_HICR5 },
		{ 0x1e789098, P9DI_POST_AST_HICR9 },
		{ 0x1e78909c, P9DI_POST_AST_HICRA },
		{ 0x1e6e200c, P9DI_POST_AST_SCU_CLKSTOP },
		{ 0x1e6e202c, P9DI_POST_AST_SCU_MISC },
		{ 0x1e6e207c, P9DI_POST_AST_SCU_REV },
		{ 0x1e6e2080, P9DI_POST_AST_SCU_PIN80 },
		{ 0x1e6e2084, P9DI_POST_AST_SCU_PIN84 },
		{ 0x1e6e2088, P9DI_POST_AST_SCU_PIN88 },
		{ 0x1e6e2090, P9DI_POST_AST_SCU_PIN90 },
		{ 0x1e78f00c, P9DI_POST_AST_UART4_LCR },
		{ 0x1e78f010, P9DI_POST_AST_UART4_MCR },
	};
	uint8_t saved[9];
	bool present = false;

	outb(0xa5, 0x4e);
	outb(0xa5, 0x4e);
	const uint8_t old_ldn = pnp_read_config(dev, 7);
	const uint32_t scratch = read_packed(dev, scratch_regs);

	/* ASPEED has scratch registers, not a conventional Super I/O ID. */
	for (unsigned int reg = 0x21; reg < 0x30; reg++) {
		if (pnp_read_config(dev, reg) != 0xff)
			present = true;
	}
	post_frame(P9DI_POST_AST_SCRATCH, scratch);
	if (!present)
		goto unavailable;

	uint32_t f0 = 0;

	for (unsigned int i = 0; i < ARRAY_SIZE(ldns); i++) {
		pnp_write_config(dev, 7, ldns[i]);
		if (pnp_read_config(dev, 7) != ldns[i])
			goto unavailable;
		post_frame(P9DI_POST_AST_UART1 + i, read_packed(dev, config));
		f0 |= (uint32_t)pnp_read_config(dev, 0xf0) << (i * 8);
	}
	post_frame(P9DI_POST_AST_UART_F0, f0);
	pnp_write_config(dev, 7, 0x0d);
	if (pnp_read_config(dev, 7) != 0x0d)
		goto unavailable;
	const uint8_t active = pnp_read_config(dev, 0x30);

	if (active == 0xff)
		goto unavailable;
	for (unsigned int i = 0; i < sizeof(saved); i++)
		saved[i] = pnp_read_config(dev, 0xf0 + i);
	post_frame(P9DI_POST_AST_BRIDGE_STATE, old_ldn | active << 8 | saved[8] << 16);
	pnp_write_config(dev, 0x30, active | 1);
	pnp_write_config(dev, 0xf8, (saved[8] & 0xfc) | 2);
	for (unsigned int i = 0; i < ARRAY_SIZE(regs); i++) {
		post_frame(P9DI_POST_AST_READ_ENTER, regs[i].address);
		post_frame(regs[i].id, ast_read(dev, regs[i].address));
		post_frame(P9DI_POST_AST_READ_DONE, regs[i].address);
	}
	for (unsigned int i = 0; i < sizeof(saved); i++)
		pnp_write_config(dev, 0xf0 + i, saved[i]);
	pnp_write_config(dev, 0x30, active);
	post_frame(P9DI_POST_AST_BRIDGE_RESTORED,
		pnp_read_config(dev, 0x30) | pnp_read_config(dev, 0xf8) << 8);
	pnp_write_config(dev, 7, old_ldn);
	outb(0xaa, 0x4e);
	return;

unavailable:
	pnp_write_config(dev, 7, old_ldn);
	outb(0xaa, 0x4e);
	post_frame(P9DI_POST_AST_UNAVAILABLE, scratch);
}

void p9di_post_snapshot(uint32_t phase)
{
	post_frame(P9DI_POST_SNAPSHOT_ENTER, phase);
	post_frame(P9DI_POST_PCH_LPC_80, pci_s_read_config16(PCH_LPC_DEV, LPC_IO_DEC));
	post_frame(P9DI_POST_PCH_LPC_82, pci_s_read_config16(PCH_LPC_DEV, LPC_EN));
	snapshot_nct();
	for (unsigned int i = 0; i < 2; i++) {
		const uint16_t port = i ? 0x2f8 : 0x3f8;
		const uint32_t status = inb(port + 3) | inb(port + 4) << 8 |
			inb(port + 5) << 16;

		/* LSR read acknowledges latched UART errors; never read RX data. */
		post_frame(i ? P9DI_POST_COM2_STATUS : P9DI_POST_COM1_STATUS, status);
	}
	if (phase != 1)
		snapshot_ast();
	post_frame(P9DI_POST_SNAPSHOT_DONE, phase);
}
#endif
