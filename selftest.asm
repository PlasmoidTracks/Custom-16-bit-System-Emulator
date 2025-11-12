.address $0000

; Test indirect register offset instruction
mov [.RESULT_indirect_register_offset], 0
mov [$8002], $8002
mov [$8004], $8004
mov [$8006], $8006
mov r1, 2
mov r2, 4
mov r3, 6
mov r0, [$8000 + r1]
cmp r0, $8002
jz .TEST_indirect_register_offset_2
or [.RESULT_indirect_register_offset], 0b0001
.TEST_indirect_register_offset_2
mov r0, [$8000 + r2]
cmp r0, $8004
jz .TEST_indirect_register_offset_3
or [.RESULT_indirect_register_offset], 0b0010
.TEST_indirect_register_offset_3
mov r0, [$8000 + r3]
cmp r0, $8006
jz .end_result_TEST_indirect_register_offset
or [.RESULT_indirect_register_offset], 0b0100
.end_result_TEST_indirect_register_offset
; move test restult into r0
mov r0, [.RESULT_indirect_register_offset]



hlt


.address 0x4000
.RESULT_indirect_register_offset


