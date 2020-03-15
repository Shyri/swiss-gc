.extern dumpRam

.text

.set r0,0;   .set r1,1;   .set r2,2; .set r3,3;   .set r4,4
.set r5,5;   .set r6,6;   .set r7,7;   .set r8,8;   .set r9,9
.set r10,10; .set r11,11; .set r12,12; .set r13,13; .set r14,14
.set r15,15; .set r16,16; .set r17,17; .set r18,18; .set r19,19
.set r20,20; .set r21,21; .set r22,22; .set r23,23; .set r24,24
.set r25,25; .set r26,26; .set r27,27; .set r28,28; .set r29,29
.set r30,30; .set r31,31; .set f0,0; .set f2,2; .set f3,3

.globl _start

gameid:
.long	0,0
cheatdata:
.long	frozenvalue
.space 39*4

_start:
	stwu	r1,-168(r1)		# stores sp
	stw	r0,8(r1)		# stores r0

	mflr	r0
	stw	r0,172(r1)		# stores lr

	mfcr	r0
	stw	r0,12(r1)		# stores cr

	mfctr	r0
	stw	r0,16(r1)		# stores ctr

	mfxer	r0
	stw	r0,20(r1)		# stores xer

	stmw	r3,24(r1)		# saves r3-r31

	mfmsr	r25
	ori	r26,r25,0x2000		#enable floating point ?
	andi.	r26,r26,0xF9FF
	mtmsr	r26


	stfd	f2,152(r1)		# stores f2
	stfd	f3,160(r1)		# stores f3

    lis	r31,_start@h		#0x8000

    lis r20, 0xCC00
    lhz r28, 0x4010(r20)
    ori r21, r28, 0xFF
    sth r21, 0x4010(r20) # disable MP3 memory protection

	mflr	r29
	lis r15, codelist@h
	ori	r15, r15, codelist@l

	ori	r7, 31, cheatdata@l	# set pointer for storing data (before the codelist)

	lis	r6,0x8000		# default base address = 0x80000000 (code handler)

	mr	r16,r6			# default pointer = 0x80000000 (code handler)

	li	r8,0			# code execution status set to true (code handler)

	lis	r3,0x00D0
        ori	r3,r3,0xC0DE

	li	r3,0x4D
    bl exisendbyte
	li	r3,0x59
    bl exisendbyte
	li	r3,0x20
    bl exisendbyte
	li	r3,0x41
    bl exisendbyte
	li	r3,0x53
    bl exisendbyte
	li	r3,0x53
    bl exisendbyte
	li	r3,0x13
    bl exisendbyte

	bl _exitcodehandler

exisendbyte:				# r3, send value
	mflr	r30
	slwi	r3, r3, 20		# (sendbyte<<20);
	oris	r3, r3, 0xA000 #0xB000		# 0xB0000000 | (OR)
	li	r22,0x19
	li	r23,0x250   #li	r23,0xD0
	lis	r24,0xCC00

	bl checkexisend

	extrwi.  r3, r16, 1,5		# returns either 0 or 1, one for byte received ok
	mtlr	r30
    blr
checkexisend:
	stw	r23, 0x6800(r24)		# 32mhz Port B
	stw	r3, 0x6810(r24)         # 0xCC006810
	stw	r22, 0x680C(r24)        # 0xCC006820 (Channel 0 Control Register)
exicheckreceivewait:
 	lwz	r5, 0x680C(r24)
	andi.	r5, r5, 1
	bne	exicheckreceivewait	# while((exi_chan1cr)&1);

	lwz	r16, 0x6810(r24)
	stw	0, 0x6800(r24) #stw	r5, 0x6800(r24)

	blr

_exitcodehandler:
	mtlr	r29
resumegame:

    sth r28,0x4010(r20) # restore memory protection value

	lfd	f2,152(r1)		# loads f2
	lfd	f3,160(r1)		# loads f3

	mfmsr	r25

	lwz	r0,172(r1)
	mtlr	r0			# restores lr

	lwz	r0,12(r1)
	mtcr	r0			# restores cr

	lwz	r0,16(r1)
	mtctr	r0			# restores ctr

	lwz	r0,20(r1)
	mtxer	r0			# restores xer

	lmw	r3,24(r1)		# restores r3-r31

	lwz	r0,8(r1)		# loads r0

	addi	r1,r1,168

	isync

        blr				# return back to game
frozenvalue:	#frozen value, then LR
.long        0,0
codelist:
.space 2*4
.end
