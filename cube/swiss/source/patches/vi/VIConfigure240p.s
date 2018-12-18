#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure240p
VIConfigure240p:
	li			%r0, 1
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
	li			%r7, 0
	li			%r6, 0
	lhz			%r5, 8 (%r3)
	slwi		%r5, %r5, 1
	cmpwi		%r5, 480
	ble			6f
	cmpwi		%r5, 574
	ble			4f
	li			%r6, 1
	lhz			%r5, 8 (%r3)
	cmpwi		%r5, 480
	ble			6f
	lhz			%r5, 6 (%r3)
	clrrwi		%r5, %r5, 1
	cmpwi		%r5, 480
	ble			3f
	li			%r5, 480
	sth			%r5, 6 (%r3)
3:	sth			%r5, 8 (%r3)
	b			6f
4:	lhz			%r5, 6 (%r3)
	cmpwi		%r5, 240
	ble			5f
	li			%r5, 240
	sth			%r5, 6 (%r3)
5:	sth			%r5, 8 (%r3)
	slwi		%r5, %r5, 1
6:	subfic		%r4, %r5, 480
	srwi		%r4, %r4, 1
	sth			%r4, 12 (%r3)
	sth			%r5, 16 (%r3)
	sth			%r6, 20 (%r3)
	stb			%r7, 24 (%r3)
	sth			%r0, 0 (%r3)

.globl VIConfigure240p_length
VIConfigure240p_length:
.long (VIConfigure240p_length - VIConfigure240p)