/* SPDX-License-Identifier: GPL-2.0-only */

#include <southbridge/intel/lynxpoint/pch.h>

/* Provisional USB mapping: only the ASMB7 hub on USB2 port 3 is measured. */
const struct usb2_port_config mainboard_usb2_ports[MAX_USB2_PORTS] = {
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_INTERNAL },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
	{ 0x0040, 1, USB_OC_PIN_SKIP, USB_PORT_FLEX },
};

const struct usb3_port_config mainboard_usb3_ports[MAX_USB3_PORTS] = {
	{ 1, USB_OC_PIN_SKIP },
	{ 1, USB_OC_PIN_SKIP },
	{ 0, USB_OC_PIN_SKIP },
	{ 0, USB_OC_PIN_SKIP },
	{ 0, USB_OC_PIN_SKIP },
	{ 0, USB_OC_PIN_SKIP },
};
