;Lab1.S - toggles blink control
.global Toggling
.global blink

.section .data
blink:
    .byte 1       ; 1 = ON, 0 = OFF

.section .text
Toggling:
    lds r16, blink   ; load blink
    ldi r17, 1
    eor r16, r17           ; toggle flash_state
    sts blink, r16   ; store back
    ret                                                                                                                                