; Since memory from 0xc000 to 0xefff is not mapped to any device, reading from 0xbcde will read from open bus. 
; This means, whatever value was last written to the bus during a read, 
; will be the value the CPU fetches when trying to read from unmapped memory
; in this case, during decoding of the instruction, we can see that the last byte to decode is byte 0xcd, 
; which is the high byte of the operant of the indirection absolute address read. 
; What we expect to see is the CPU to read 0xcd twice, once for the low byte and once for the high byte, 
; and write the final value 0xcdcd into r0


; 0x01 0x41 0xef 0xcd 
mov r0, [$CDEF]
; 0x74 0x00 
hlt



