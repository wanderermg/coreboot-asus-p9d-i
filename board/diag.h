/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef P9DI_DIAG_H
#define P9DI_DIAG_H

#include <types.h>

#if CONFIG(P9DI_DUAL_UART_DIAGNOSTICS)
void p9di_diag_init(void);
void p9di_diag_mark(const char *event, uint32_t value);
#else
static inline void p9di_diag_init(void) { }
#if CONFIG(P9DI_SIMPLE_POST) && ENV_SEPARATE_ROMSTAGE
void p9di_diag_mark(const char *event, uint32_t value);
#else
static inline void p9di_diag_mark(const char *event, uint32_t value) { }
#endif
#endif

#if CONFIG(P9DI_POST_DIAGNOSTICS)
void p9di_post_mark(const char *event, uint32_t value);
void p9di_post_snapshot(uint32_t phase);
void p9di_smbus_probe(bool before_gpio);
void p9di_smbus_read_error(int error);
#else
static inline void p9di_post_mark(const char *event, uint32_t value) { }
static inline void p9di_post_snapshot(uint32_t phase) { }
static inline void p9di_smbus_probe(bool before_gpio) { }
static inline void p9di_smbus_read_error(int error) { }
#endif

#endif
