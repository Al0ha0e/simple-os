#ifndef COMMON_H
#define COMMON_H

// console.c
void consoleinit(void);
void consoleintr(int);
void consputc(int);

// uart.c
void uartinit(void);
void uartintr(void);
void uartputc(int);
void uartputc_sync(int);
int uartgetc(void);

#endif