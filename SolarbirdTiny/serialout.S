;25mmHg Kazoosh, 2021
; must be in the same directory with .ino!!!!!!!!!!!!!!!!!!

/* half-duplex 81N serial uart in hand-tuned assembler
 * 1%/2% Tx/Rx timing error for 115.2kbps@8Mhz
 * 2%/1% Tx/Rx timing error for 230.4kbps@8Mhz
 * optimized for no jitter vs AVR305 with 1 cycle/bit jitter
 * @author: Ralph Doncaster
 * @orginalversion: BasicSerial3.S
 * link: https://nerdralph.blogspot.com/2014/01/avr-half-duplex-software-uart.html
 * shrink to TX only by 25mmHg
 */

#define delayCount r18
#include <avr/io.h>
; correct for avr/io.h 0x20 port offset for io instructions
#define UART_Port (PORTB-0x20)
#define UART_Tx 4 // DISCONNECT PIEZO !!!!!!!!!!!!!!!!!!!!!!!!

.global TxTimedByte
; transmit byte in r24 with bit delay in r22 - 15 instructions
; calling code must set Tx line to idle state (high) or 1st byte may be lost
; i.e. PORTB |= (1<<UART_Tx)
TxTimedByte:
	cli
	sbi UART_Port-1, UART_Tx			; set Tx line to output
	cbi UART_Port, UART_Tx				; start bit
	in r0, UART_Port
	ldi r25, 3							; stop bit + idle state
TxLoop:
; 8 cycle loop + delay - total = 7 + 3*r22
	mov delayCount, r22
TxDelay:
; delay (3 cycle * delayCount) -1
	dec delayCount
	brne TxDelay
	bst r24, 0							; store lsb in T
	bld r0, UART_Tx
	lsr r25
	ror r24
	out UART_Port, r0
	brne TxLoop
	reti								; return and enable interrupts
