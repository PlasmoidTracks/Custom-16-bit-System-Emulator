#include <stdint.h>
#include <stdio.h>

static uint8_t ram[65536] = {0};
static uint16_t r0, r1, r2, r3, sp;

static struct {
	union {
		uint16_t value;
		struct {
			uint16_t Z : 1;
			uint16_t FZ : 1;
			uint16_t L : 1;
			uint16_t UL : 1;
			uint16_t FL : 1;
			uint16_t DL : 1;
			uint16_t LL : 1;
			uint16_t AO : 1;
			uint16_t MI : 1;
		};
	};
} sr = {0};

void write16(uint16_t address, uint16_t data) {
	ram[address] = data & 0x00ff;
	ram[address + 1] = data >> 8;
}
uint16_t read16(uint16_t address) {
	return ram[address] | (ram[address + 1] << 8);
}

void push16(uint16_t data) {
	sp -= 0x0002;
	write16(sp, data);
}
uint16_t pop16(void) {
	uint16_t data = read16(sp);
	sp += 0x0002;
	return data;
}

// Todo: write the actual binary data into ram for authenticity

void _jump_label0(void);
void _jump_label1(void);

// This function contains the code that is run at the beginning of the execution
void preamble(void) {
	// call .jump_label0 
	push16(0xFFFF); // stub push of pc
	_jump_label0();
	// hlt 
	return;
}

void _jump_label0(void) {
	// push r3 
	push16(r3);
	// mov r3 sp 
	r3 = sp;
	// sub sp $0008 
	sp -= 0x0008;
	// lea r0 [ $FFFE + r3 ] 
	r0 = 0xFFFE + r3;
	// mov [ r0 ] $7F02 
	write16(r0, 0x7F02);
	// lea r0 [ $FFFC + r3 ] 
	r0 = 0xFFFC + r3;
	// mov [ r0 ] $0140 
	write16(r0, 0x0140);
	// mov r0 [ $FFFE + r3 ] 
	r0 = read16(0xFFFE + r3);
	// mov [ r0 ] [ $FFFC + r3 ] 
	write16(r0, read16(0xFFFC + r3));
	// mov [ $0169 ] $0010 
	write16(0x0169, 0x0010);
	// lea r0 [ $FFFA + r3 ] 
	r0 = 0xFFFA + r3;
	// mov [ r0 ] $FFFF 
	write16(r0, 0xFFFF);
	// lea r1 [ $FFFA + r3 ] 
	r1 = 0xFFFA + r3;
	// lea r0 [ $FFF8 + r3 ] 
	r0 = 0xFFF8 + r3;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// push [ $FFF8 + r3 ] 
	push16(read16(0xFFF8 + r3));
	// push [ $0169 ] 
	push16(read16(0x0169));
	// call .jump_label1 
	push16(0xFFFF); // stub push of pc
	_jump_label1();
	// add sp $0004 
	sp += 0x0004;
	// mov r0 [ $FFFA + r3 ] 
	r0 = read16(0xFFFA + r3);
	// mov r1 [ $016D ] 
	r1 = read16(0x016D);
	// mov r2 [ $016B ] 
	r2 = read16(0x016B);
	// mov sp r3 
	sp = r3;
	// pop r3 
	r3 = pop16();
	// ret 
	(void) pop16();
	return;
}

void _jump_label1(void) {
	// push r3 
	push16(r3);
	// mov r3 sp 
	r3 = sp;
	// sub sp $0018 
	sp -= 0x0018;
	// mov r0 [ $016B ] 
	r0 = read16(0x016B);
	// add r0 $0001 
	r0 += 0x0001;
	// mov r1 r0 
	r1 = r0;
	// lea r0 [ $016B ] 
	r0 = 0x016B;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// lea r1 [ $0006 + r3 ] 
	r1 = 0x0006 + r3;
	// lea r0 [ $FFFE + r3 ] 
	r0 = 0xFFFE + r3;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// mov r1 [ $FFFE + r3 ] 
	r1 = read16(0xFFFE + r3);
	// lea r0 [ $FFFC + r3 ] 
	r0 = 0xFFFC + r3;
	// mov [ r0 ] [ r1 ] 
	write16(r0, read16(r1));
	// lea r1 [ $0004 + r3 ] 
	r1 = 0x0004 + r3;
	// lea r0 [ $FFFA + r3 ] 
	r0 = 0xFFFA + r3;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// mov r1 [ $FFFA + r3 ] 
	r1 = read16(0xFFFA + r3);
	// lea r0 [ $FFF8 + r3 ] 
	r0 = 0xFFF8 + r3;
	// mov [ r0 ] [ r1 ] 
	write16(r0, read16(r1));
	// mov r0 [ $FFF8 + r3 ] 
	r0 = read16(0xFFF8 + r3);
	// cmp r0 $0001 
	sr.Z = r0 == 0x0001;
	sr.L = (int16_t) r0 < (int16_t) 0x0001;
	sr.UL = r0 < 0x0001;
	// xor r1 r1 
	r1 ^= r1;
	// cmovnl r1 $0001 
	r1 = sr.L ? r1 : 0x0001;
	// cmovz r1 $0000 
	r1 = !sr.Z ? r1 : 0x0000;
	// lea r0 [ $FFF6 + r3 ] 
	r0 = 0xFFF6 + r3;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// mov r1 [ $FFF6 + r3 ] 
	r1 = read16(0xFFF6 + r3);
	// tst r1 
	sr.Z = r1 == 0x0000;
	sr.L = (int16_t) r1 < 0x0000;
	sr.UL = 0;
	// jnz .jump_label2 
	if (!sr.Z) goto _jump_label2;
	// mov r1 [ $FFF8 + r3 ] 
	r1 = read16(0xFFF8 + r3);
	// mov r0 [ $FFFC + r3 ] 
	r0 = read16(0xFFFC + r3);
	// mov [ r0 ] r1 
	write16(r0, r1);
	// mov sp r3 
	sp = r3;
	// pop r3 
	r3 = pop16();
	// ret 
	(void) pop16();
	return;
	// .jump_label2 
_jump_label2:
	// lea r1 [ $FFF4 + r3 ] 
	r1 = 0xFFF4 + r3;
	// lea r0 [ $FFF2 + r3 ] 
	r0 = 0xFFF2 + r3;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// mov r0 [ $FFF8 + r3 ] 
	r0 = read16(0xFFF8 + r3);
	// sub r0 $0002 
	r0 -= 0x0002;
	// mov r1 r0 
	r1 = r0;
	// lea r0 [ $FFF0 + r3 ] 
	r0 = 0xFFF0 + r3;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// push [ $FFF2 + r3 ] 
	push16(read16(0xFFF2 + r3));
	// push [ $FFF0 + r3 ] 
	push16(read16(0xFFF0 + r3));
	// call .jump_label1 
	push16(0xFFFF); // stub push of pc
	_jump_label1();
	// add sp $0004 
	sp += 0x0004;
	// lea r1 [ $FFEE + r3 ] 
	r1 = 0xFFEE + r3;
	// lea r0 [ $FFEC + r3 ] 
	r0 = 0xFFEC + r3;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// mov r0 [ $FFF8 + r3 ] 
	r0 = read16(0xFFF8 + r3);
	// sub r0 $0001 
	r0 -= 0x0001;
	// mov r1 r0 
	r1 = r0;
	// lea r0 [ $FFEA + r3 ] 
	r0 = 0xFFEA + r3;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// push [ $FFEC + r3 ] 
	push16(read16(0xFFEC + r3));
	// push [ $FFEA + r3 ] 
	push16(read16(0xFFEA + r3));
	// call .jump_label1 
	push16(0xFFFF); // stub push of pc
	_jump_label1();
	// add sp $0004 
	sp += 0x0004;
	// mov r0 [ $FFF4 + r3 ] 
	r0 = read16(0xFFF4 + r3);
	// add r0 [ $FFEE + r3 ] 
	r0 += read16(0xFFEE + r3);
	// mov r1 r0 
	r1 = r0;
	// lea r0 [ $FFE8 + r3 ] 
	r0 = 0xFFE8 + r3;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// mov r0 [ $FFFC + r3 ] 
	r0 = read16(0xFFFC + r3);
	// mov [ r0 ] [ $FFE8 + r3 ] 
	write16(r0, read16(0xFFE8 + r3));
	// mov sp r3 
	sp = r3;
	// pop r3 
	r3 = pop16();
	// ret 
	(void) pop16();
	return;
	// semi 
	sr.MI = 0x0001;
	// push r3 
	push16(r3);
	// mov r3 sp 
	r3 = sp;
	// push r2 
	push16(r2);
	// push r1 
	push16(r1);
	// push r0 
	push16(r0);
	// pushsr 
	push16(sr.value);
	// mov r0 [ $016D ] 
	r0 = read16(0x016D);
	// add r0 $0001 
	r0 += 0x0001;
	// mov r1 r0 
	r1 = r0;
	// lea r0 [ $016D ] 
	r0 = 0x016D;
	// mov [ r0 ] r1 
	write16(r0, r1);
	// popsr 
	sr.value = pop16();
	// pop r0 
	r0 = pop16();
	// pop r1 
	r1 = pop16();
	// pop r2 
	r2 = pop16();
	// clmi 
	sr.MI = 0x0000;
	// mov sp r3 
	sp = r3;
	// pop r3 
	r3 = pop16();
	// ret 
	(void) pop16();
	return;
}

int main(void) {
	r0 = 0x0000;
	r1 = 0x0000;
	r2 = 0x0000;
	r3 = 0x0000;
	sp = 0x7F00;

	preamble();

	printf("r0: %.4x\n", r0);
	printf("r1: %.4x\n", r1);
	printf("r2: %.4x\n", r2);
	printf("r3: %.4x\n", r3);
	printf("sp: %.4x\n", sp);

	return 0;
}
