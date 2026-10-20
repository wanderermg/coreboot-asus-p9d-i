## SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c
bootblock-$(CONFIG_P9DI_DUAL_UART_DIAGNOSTICS) += diag.c
romstage-$(CONFIG_P9DI_DUAL_UART_DIAGNOSTICS) += diag.c
ramstage-$(CONFIG_P9DI_DUAL_UART_DIAGNOSTICS) += diag.c
bootblock-$(CONFIG_P9DI_POST_DIAGNOSTICS) += post.c
romstage-$(CONFIG_P9DI_POST_DIAGNOSTICS) += post.c smbus_diag.c
ramstage-$(CONFIG_P9DI_POST_DIAGNOSTICS) += post.c
romstage-y += gpio.c ecc.c spd.c smbus_owner.c
romstage-$(CONFIG_P9DI_SIMPLE_POST) += simple_post.c
ramstage-y += bmc.c monitor.c
smm-y += bmc.c
smm-srcs += src/drivers/ipmi/ipmi_kcs.c
