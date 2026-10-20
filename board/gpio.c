/* SPDX-License-Identifier: GPL-2.0-only */

#include <southbridge/intel/common/gpio.h>

/*
 * Modes/directions/output levels match the measured stock UEFI settings.
 * Select reset domains explicitly: GPIO24 (BMC_INIT#) uses PWROK and
 * GPIO73 (VDDQ_SEL_GPIO) uses RSMRST#, matching stock GP_RST_SEL=0/0/0x200.
 * Input/native pin readback levels can differ after device initialization.
 */

static const struct pch_gpio_set1 set1_reset = { 0 };
static const struct pch_gpio_set2 set2_reset = { 0 };
static const struct pch_gpio_set3 set3_reset = {
	.gpio73 = GPIO_RESET_RSMRST,
};

static const struct pch_gpio_set1 set1_mode = {
	.gpio0 = 1,
	.gpio6 = 1,
	.gpio8 = 1,
	.gpio12 = 1,
	.gpio13 = 1,
	.gpio14 = 1,
	.gpio15 = 1,
	.gpio16 = 1,
	.gpio17 = 1,
	.gpio21 = 1,
	.gpio22 = 1,
	.gpio24 = 1,
	.gpio27 = 1,
	.gpio28 = 1,
	.gpio29 = 1,
};

static const struct pch_gpio_set1 set1_direction = {
	.gpio0 = 1,
	.gpio1 = 1,
	.gpio6 = 1,
	.gpio7 = 1,
	.gpio9 = 1,
	.gpio10 = 1,
	.gpio11 = 1,
	.gpio12 = 1,
	.gpio13 = 1,
	.gpio14 = 1,
	.gpio16 = 1,
	.gpio17 = 1,
	.gpio18 = 1,
	.gpio19 = 1,
	.gpio20 = 1,
	.gpio21 = 1,
	.gpio23 = 1,
	.gpio25 = 1,
	.gpio26 = 1,
	.gpio28 = 1,
	.gpio31 = 1,
};

static const struct pch_gpio_set1 set1_level = {
	.gpio1 = 1,
	.gpio2 = 1,
	.gpio3 = 1,
	.gpio4 = 1,
	.gpio5 = 1,
	.gpio7 = 1,
	.gpio8 = 1,
	.gpio9 = 1,
	.gpio10 = 1,
	.gpio11 = 1,
	.gpio13 = 1,
	.gpio14 = 1,
	.gpio16 = 1,
	.gpio17 = 1,
	.gpio18 = 1,
	.gpio19 = 1,
	.gpio21 = 1,
	.gpio22 = 1,
	.gpio23 = 1,
	.gpio25 = 1,
	.gpio26 = 1,
	.gpio28 = 1,
	.gpio29 = 1,
	.gpio31 = 1,
};

static const struct pch_gpio_set1 set1_invert = {
	.gpio13 = 1,
};

static const struct pch_gpio_set1 set1_blink = {
	.gpio18 = 1,
};

static const struct pch_gpio_set2 set2_mode = {
	.gpio32 = 1,
	.gpio33 = 1,
	.gpio34 = 1,
	.gpio44 = 1,
	.gpio45 = 1,
	.gpio49 = 1,
	.gpio50 = 1,
	.gpio51 = 1,
	.gpio52 = 1,
	.gpio53 = 1,
	.gpio54 = 1,
	.gpio55 = 1,
};

static const struct pch_gpio_set2 set2_direction = {
	.gpio34 = 1,
	.gpio36 = 1,
	.gpio37 = 1,
	.gpio40 = 1,
	.gpio41 = 1,
	.gpio42 = 1,
	.gpio43 = 1,
	.gpio44 = 1,
	.gpio45 = 1,
	.gpio46 = 1,
	.gpio47 = 1,
	.gpio49 = 1,
	.gpio54 = 1,
	.gpio56 = 1,
	.gpio57 = 1,
	.gpio58 = 1,
	.gpio59 = 1,
	.gpio60 = 1,
};

static const struct pch_gpio_set2 set2_level = {
	.gpio32 = 1,
	.gpio36 = 1,
	.gpio37 = 1,
	.gpio38 = 1,
	.gpio40 = 1,
	.gpio41 = 1,
	.gpio42 = 1,
	.gpio43 = 1,
	.gpio45 = 1,
	.gpio48 = 1,
	.gpio51 = 1,
	.gpio53 = 1,
	.gpio54 = 1,
	.gpio55 = 1,
	.gpio57 = 1,
	.gpio58 = 1,
	.gpio59 = 1,
	.gpio60 = 1,
	.gpio61 = 1,
	.gpio63 = 1,
};

static const struct pch_gpio_set3 set3_mode = {
	.gpio68 = 1,
	.gpio69 = 1,
	.gpio70 = 1,
	.gpio72 = 1,
	.gpio73 = 1,
};

static const struct pch_gpio_set3 set3_direction = {
	.gpio68 = 1,
	.gpio69 = 1,
	.gpio70 = 1,
	.gpio71 = 1,
	.gpio72 = 1,
	.gpio75 = 1,
};

static const struct pch_gpio_set3 set3_level = {
	.gpio64 = 1,
	.gpio65 = 1,
	.gpio66 = 1,
	.gpio67 = 1,
	.gpio68 = 1,
	.gpio69 = 1,
	.gpio70 = 1,
	.gpio71 = 1,
	.gpio72 = 1,
	.gpio73 = 1,
	.gpio75 = 1,
};

const struct pch_gpio_map mainboard_gpio_map = {
	.set1 = {
		.mode = &set1_mode,
		.direction = &set1_direction,
		.level = &set1_level,
		.reset = &set1_reset,
		.invert = &set1_invert,
		.blink = &set1_blink,
	},
	.set2 = {
		.mode = &set2_mode,
		.direction = &set2_direction,
		.level = &set2_level,
		.reset = &set2_reset,
	},
	.set3 = {
		.mode = &set3_mode,
		.direction = &set3_direction,
		.level = &set3_level,
		.reset = &set3_reset,
	},
};
