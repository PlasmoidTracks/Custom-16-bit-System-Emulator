; Since memory from 0xa000 to 0xefff is not mapped to any device, reading from 0xbcde will read from open bus. 
; This means, whatever value was last written to the bus during a read, 
; will be the value the CPU fetches when trying to read from unmapped memory
; in this case, during decoding of the instruction, we can see that the last byte to decode is byte 0xbc, 
; which is the high byte of the operant of the indirection absolute address read. 
; What we expect to see is the CPU to read 0xbc twice, once for the low byte and once for the high byte, 
; and write the final value 0xbcbc into r0


; 0x81 0x41 0xde 0xbc 
%mov r0, [$BCDE]
; 0x74 0x00 
hlt



