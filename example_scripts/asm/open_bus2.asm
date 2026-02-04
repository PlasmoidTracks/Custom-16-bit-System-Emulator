; 0x07 0x08 0xf8 0x7f 
jmp 0x7ff8

.address $7ff8
; 0x4d 0x08 0x00 0x00 
tst $0000

; 0x01 0x09 0x00 0x08 
mov r0, $0800
; The last instruction is set at 0x7ffc - 0x7fff, and the segment from 0x8000 - 0x9fff is not mapped. 
; So the next read is gonna be from open bus. 
; here the last byte that was fetched was 0x08, 
; so the next instruction that is decoded will therefore be 0x0808, which is jz
; and the operand is gonna be decoded as 0x0808. 
; So the next instruction will be:
; jz 0x0808
; and because we did tst $0000, the Z flag will be set to 1 and the jump will be executed
; hence why we land at:

.address $0808
; 0x01 0x0b 0xff 0xff 
mov r2, $FFFF
; 0x74 0x00 
hlt


