/*
 * MiniOS ARM64 PL011 Direct UART Access
 * For early boot logging before the driver framework is active.
 */

#ifndef PL011_H
#define PL011_H

void pl011_direct_init(void);
void pl011_direct_putc(char c);

#endif // PL011_H
