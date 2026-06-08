.address $0000
.code
call .main
hlt
.main 
push r3
mov r3, sp
sub sp, $0008
lea r0, [$FFFE + r3]
mov [r0], $7F02
lea r0, [$FFFC + r3]
mov [r0], .irq_handler
mov r0, [$FFFE + r3]
mov [r0], [$FFFC + r3]
mov [.__static_data], $0010
lea r0, [$FFFA + r3]
mov [r0], $FFFF
lea r0, [$FFF8 + r3]
lea [r0], [$FFFA + r3]
push [$FFF8 + r3]
push [.__static_data]
call .fib
add sp, $0004
mov r0, [$FFFA + r3]
mov r1, [.__static_data + $0004]
mov r2, [.__static_data + $0002]
mov sp, r3
pop r3
ret
.fib 
push r3
mov r3, sp
sub sp, $0018
inc [.__static_data + $0002]
lea r1, [$0006 + r3]
lea r0, [$FFFE + r3]
mov [r0], r1
lea r0, [$FFFC + r3]
mov [r0], [r1]
lea r1, [$0004 + r3]
lea r0, [$FFFA + r3]
mov [r0], r1
lea r0, [$FFF8 + r3]
mov [r0], [r1]
mov r0, [$FFF8 + r3]
cmp r0, $0001
xor r1, r1
cmovnl r1, $0001
cmovz r1, $0000
lea r0, [$FFF6 + r3]
mov [r0], r1
tst [$FFF6 + r3]
jnz .valid
mov r0, [$FFFC + r3]
mov [r0], [$FFF8 + r3]
mov sp, r3
pop r3
ret
.valid 
lea r0, [$FFF2 + r3]
lea [r0], [$FFF4 + r3]
mov r1, [$FFF8 + r3]
sub r1, $0002
lea r0, [$FFF0 + r3]
mov [r0], r1
push [$FFF2 + r3]
push [$FFF0 + r3]
call .fib
add sp, $0004
lea r0, [$FFEC + r3]
lea [r0], [$FFEE + r3]
mov r1, [$FFF8 + r3]
dec r1
lea r0, [$FFEA + r3]
mov [r0], r1
push [$FFEC + r3]
push [$FFEA + r3]
call .fib
add sp, $0004
mov r1, [$FFF4 + r3]
add r1, [$FFEE + r3]
lea r0, [$FFE8 + r3]
mov [r0], r1
mov r0, [$FFFC + r3]
mov [r0], [$FFE8 + r3]
mov sp, r3
pop r3
ret
.irq_handler 
semi
inc [.__static_data + $0004]
clmi
ret
.data
.__static_data 
.dw $0000
.dw $0000
.dw $0000
