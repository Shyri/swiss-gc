#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure1080i60
VIConfigure1080i60:
	li			%r0, 0
	lwz			%r6, 0 (%r3)
	lis			%r4, VAR_AREA
	lwz			%r5, VAR_TVMODE (%r4)
	cmpwi		%r5, 5
	beq			1f
	srwi		%r5, %r6, 2
	cmpwi		%r5, 5
	bne			2f
	stw			%r5, VAR_TVMODE (%r4)
1:	insrwi		%r0, %r5, 30, 0
2:	stw			%r3, VAR_RMODE (%r4)
	li			%r8, 1
	li			%r7, 0
	lhz			%r6, 8 (%r3)
	slwi		%r6, %r6, 1
	cmpwi		%r6, 480
	ble			6f
	cmpwi		%r6, 574
	ble			4f
	ori			%r0, %r0, 2
	lhz			%r6, 8 (%r3)
	cmpwi		%r6, 480
	ble			3f
	lhz			%r6, 6 (%r3)
	clrrwi		%r6, %r6, 1
	sth			%r6, 8 (%r3)
3:	subfic		%r5, %r6, 540
	srawi		%r5, %r5, 1
	b			7f
4:	lhz			%r6, 6 (%r3)
	cmpwi		%r6, 240
	ble			5f
	subi		%r6, %r6, 264
	slwi		%r6, %r6, 1
	addi		%r6, %r6, 240
	sth			%r6, 6 (%r3)
5:	sth			%r6, 8 (%r3)
	slwi		%r6, %r6, 1
6:	subfic		%r5, %r6, 480
	srwi		%r5, %r5, 1
7:	sth			%r5, 12 (%r3)
	sth			%r6, 16 (%r3)
	sth			%r7, 20 (%r3)
	stb			%r8, 24 (%r3)
	sth			%r0, 0 (%r3)

.globl VIConfigure1080i60_length
VIConfigure1080i60_length:
.long (VIConfigure1080i60_length - VIConfigure1080i60)