	.arch armv8-a
	.file	"Timer.cpp"
	.text
	.section	.text.startup,"ax",@progbits
	.align	2
	.p2align 4,,11
	.type	_ZStplIcSt11char_traitsIcESaIcEENSt7__cxx1112basic_stringIT_T0_T1_EERKS8_OS8_.isra.0, %function
_ZStplIcSt11char_traitsIcESaIcEENSt7__cxx1112basic_stringIT_T0_T1_EERKS8_OS8_.isra.0:
.LFB2574:
	.cfi_startproc
	stp	x29, x30, [sp, -32]!
	.cfi_def_cfa_offset 32
	.cfi_offset 29, -32
	.cfi_offset 30, -24
	mov	x3, x0
	mov	x4, x1
	mov	x29, sp
	mov	x0, x2
	mov	x2, 0
	mov	x1, 0
	str	x19, [sp, 16]
	.cfi_offset 19, -16
	mov	x19, x8
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE7replaceEmmPKcm
	mov	x1, x0
	mov	x0, x19
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1EOS4_
	mov	x0, x19
	ldr	x19, [sp, 16]
	ldp	x29, x30, [sp], 32
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 19
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE2574:
	.size	_ZStplIcSt11char_traitsIcESaIcEENSt7__cxx1112basic_stringIT_T0_T1_EERKS8_OS8_.isra.0, .-_ZStplIcSt11char_traitsIcESaIcEENSt7__cxx1112basic_stringIT_T0_T1_EERKS8_OS8_.isra.0
	.text
	.align	2
	.p2align 4,,11
	.global	_ZN5Timer3runEv
	.type	_ZN5Timer3runEv, %function
_ZN5Timer3runEv:
.LFB1992:
	.cfi_startproc
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	mov	x29, sp
.L5:
	bl	_ZN5Timer11getInstanceEv
	bl	_ZN5Timer11getInstanceEv
	b	.L5
	.cfi_endproc
.LFE1992:
	.size	_ZN5Timer3runEv, .-_ZN5Timer3runEv
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align	3
.LC0:
	.string	"Timer "
	.align	3
.LC1:
	.string	" fired.\n"
	.text
	.align	2
	.p2align 4,,11
	.global	_ZN5Timer4_runEv
	.type	_ZN5Timer4_runEv, %function
_ZN5Timer4_runEv:
.LFB1993:
	.cfi_startproc
	stp	x29, x30, [sp, -80]!
	.cfi_def_cfa_offset 80
	.cfi_offset 29, -80
	.cfi_offset 30, -72
	mov	x1, 53376
	mov	x29, sp
	stp	x25, x26, [sp, 64]
	.cfi_offset 25, -16
	.cfi_offset 26, -8
	mov	x26, x0
	adrp	x25, .LC0
	mov	x0, 4232
	add	x25, x25, :lo12:.LC0
	stp	x19, x20, [sp, 16]
	.cfi_offset 19, -64
	.cfi_offset 20, -56
	add	x20, x26, x0
	mov	x19, 0
	stp	x21, x22, [sp, 32]
	.cfi_offset 21, -48
	.cfi_offset 22, -40
	add	x22, x26, 128
	add	x21, x26, x1
	stp	x23, x24, [sp, 48]
	.cfi_offset 23, -32
	.cfi_offset 24, -24
	adrp	x24, .LC1
	add	x24, x24, :lo12:.LC1
	adrp	x23, :got:_ZSt4cout
	ldr	x23, [x23, :got_lo12:_ZSt4cout]
	b	.L11
	.p2align 2,,3
.L9:
	add	x19, x19, 1
	add	x20, x20, 48
	cmp	x19, 1024
	beq	.L14
.L11:
	ldr	w0, [x22, x19, lsl 2]
	ldr	w1, [x21, x19, lsl 2]
	sub	w1, w1, w0
	str	w1, [x21, x19, lsl 2]
	ldr	w0, [x22, x19, lsl 2]
	cmp	w0, 0
	ccmp	w1, 0, 0, ne
	bne	.L9
	ldr	w0, [x20, 36]
	mov	x1, x25
	str	w0, [x21, x19, lsl 2]
	mov	x2, 6
	mov	x0, x23
	bl	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l
	ldp	x1, x2, [x20]
	mov	x0, x23
	bl	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l
	mov	x1, x24
	mov	x2, 8
	bl	_ZSt16__ostream_insertIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_PKS3_l
	ldr	w0, [x20, 32]
	cmp	w0, 0
	cset	w1, gt
	sub	w0, w0, w1
	str	w0, [x20, 32]
	cbnz	w0, .L9
	lsr	x2, x19, 6
	mov	x1, 1
	str	wzr, [x22, x19, lsl 2]
	lsl	x1, x1, x19
	add	x19, x19, 1
	add	x20, x20, 48
	ldr	x0, [x26, x2, lsl 3]
	orr	x0, x0, x1
	str	x0, [x26, x2, lsl 3]
	cmp	x19, 1024
	bne	.L11
.L14:
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x23, x24, [sp, 48]
	ldp	x25, x26, [sp, 64]
	ldp	x29, x30, [sp], 80
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 25
	.cfi_restore 26
	.cfi_restore 23
	.cfi_restore 24
	.cfi_restore 21
	.cfi_restore 22
	.cfi_restore 19
	.cfi_restore 20
	.cfi_def_cfa_offset 0
	ret
	.cfi_endproc
.LFE1993:
	.size	_ZN5Timer4_runEv, .-_ZN5Timer4_runEv
	.section	.rodata.str1.8
	.align	3
.LC2:
	.string	"array::at: __n (which is %zu) >= _Nm (which is %zu)"
	.text
	.align	2
	.p2align 4,,11
	.global	_ZN5Timer5pauseEj
	.type	_ZN5Timer5pauseEj, %function
_ZN5Timer5pauseEj:
.LFB1994:
	.cfi_startproc
	uxtw	x2, w1
	cmp	w1, 1023
	bhi	.L20
	add	x0, x0, 128
	str	wzr, [x0, x2, lsl 2]
	ret
.L20:
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	mov	x1, x2
	adrp	x0, .LC2
	mov	x29, sp
	add	x0, x0, :lo12:.LC2
	mov	x2, 1024
	bl	_ZSt24__throw_out_of_range_fmtPKcz
	.cfi_endproc
.LFE1994:
	.size	_ZN5Timer5pauseEj, .-_ZN5Timer5pauseEj
	.align	2
	.p2align 4,,11
	.global	_ZN5Timer7unpauseEj
	.type	_ZN5Timer7unpauseEj, %function
_ZN5Timer7unpauseEj:
.LFB1995:
	.cfi_startproc
	uxtw	x2, w1
	cmp	w1, 1023
	bhi	.L26
	add	x0, x0, 128
	mov	w1, 1
	str	w1, [x0, x2, lsl 2]
	ret
.L26:
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	mov	x1, x2
	adrp	x0, .LC2
	mov	x29, sp
	add	x0, x0, :lo12:.LC2
	mov	x2, 1024
	bl	_ZSt24__throw_out_of_range_fmtPKcz
	.cfi_endproc
.LFE1995:
	.size	_ZN5Timer7unpauseEj, .-_ZN5Timer7unpauseEj
	.section	.rodata.str1.8
	.align	3
.LC3:
	.string	"bitset::set"
	.align	3
.LC4:
	.string	"%s: __position (which is %zu) >= _Nb (which is %zu)"
	.text
	.align	2
	.p2align 4,,11
	.global	_ZN5Timer5startEjjRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEi
	.type	_ZN5Timer5startEjjRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEi, %function
_ZN5Timer5startEjjRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEi:
.LFB1996:
	.cfi_startproc
	stp	x29, x30, [sp, -144]!
	.cfi_def_cfa_offset 144
	.cfi_offset 29, -144
	.cfi_offset 30, -136
	mov	x29, sp
	ldr	x5, [x0]
	stp	x19, x20, [sp, 16]
	.cfi_offset 19, -128
	.cfi_offset 20, -120
	mov	x19, x0
	stp	x21, x22, [sp, 32]
	stp	x23, x24, [sp, 48]
	.cfi_offset 21, -112
	.cfi_offset 22, -104
	.cfi_offset 23, -96
	.cfi_offset 24, -88
	mov	w24, w4
	str	x25, [sp, 64]
	.cfi_offset 25, -80
	cbnz	x5, .L43
	ldr	x5, [x0, 8]
	cbnz	x5, .L44
	ldr	x5, [x0, 16]
	cbnz	x5, .L45
	ldr	x5, [x0, 24]
	cbnz	x5, .L46
	ldr	x5, [x0, 32]
	cbnz	x5, .L47
	ldr	x5, [x0, 40]
	cbnz	x5, .L48
	ldr	x5, [x0, 48]
	cbnz	x5, .L49
	ldr	x5, [x0, 56]
	cbnz	x5, .L50
	ldr	x5, [x0, 64]
	cbnz	x5, .L51
	ldr	x5, [x0, 72]
	cbnz	x5, .L52
	ldr	x5, [x0, 80]
	cbnz	x5, .L53
	ldr	x5, [x0, 88]
	cbnz	x5, .L54
	ldr	x5, [x0, 96]
	cbnz	x5, .L55
	ldr	x5, [x0, 104]
	cbnz	x5, .L56
	ldr	x5, [x0, 112]
	cbnz	x5, .L57
	ldr	x5, [x0, 120]
	mov	x21, 15
	cbnz	x5, .L28
	adrp	x1, .LC3
	adrp	x0, .LC4
	mov	x3, 1024
	add	x1, x1, :lo12:.LC3
	add	x0, x0, :lo12:.LC4
	mov	x2, x3
	bl	_ZSt24__throw_out_of_range_fmtPKcz
.L43:
	mov	x21, 0
	.p2align 3,,7
.L28:
	rbit	x5, x5
	lsl	w20, w1, 4
	clz	x5, x5
	sub	w20, w20, w1
	add	x21, x5, x21, lsl 6
	mov	x4, 1
	lsl	w20, w20, 1
	mov	w8, 19923
	lsr	x5, x21, 6
	lsl	x0, x4, x21
	movk	w8, 0x1062, lsl 16
	add	x1, x19, x21, lsl 2
	add	x6, x1, 49152
	add	x25, sp, 120
	ldr	x7, [x19, x5, lsl 3]
	umull	x20, w20, w8
	str	w2, [sp, 96]
	bic	x0, x7, x0
	str	x0, [x19, x5, lsl 3]
	lsr	x20, x20, 38
	str	x25, [sp, 104]
	ldp	x23, x22, [x3]
	str	w20, [x6, 4224]
	str	x22, [sp, 88]
	str	w4, [x1, 128]
	cmp	x22, 15
	bhi	.L69
	cmp	x22, 1
	beq	.L70
	cbnz	x22, .L71
.L35:
	mov	x0, x25
.L34:
	lsl	x23, x21, 1
	str	x22, [sp, 112]
	add	x1, x23, x21
	strb	wzr, [x0, x22]
	stp	w24, w20, [sp, 136]
	lsl	x1, x1, 4
	ldr	w0, [sp, 96]
	add	x22, x19, x1
	ldr	x3, [sp, 104]
	str	w0, [x22, 4224]
	ldr	x0, [x22, 4232]
	cmp	x3, x25
	beq	.L72
	mov	x2, 4248
	add	x2, x1, x2
	add	x2, x19, x2
	cmp	x0, x2
	beq	.L73
	add	x2, x19, x1
	ldr	q0, [sp, 112]
	mov	x1, x2
	ldr	x2, [x2, 4248]
	str	x3, [x22, 4232]
	str	q0, [x1, 4240]
	cbz	x0, .L41
	str	x0, [sp, 104]
	str	x2, [sp, 120]
.L39:
	add	x23, x23, x21
	str	xzr, [sp, 112]
	strb	wzr, [x0]
	add	x23, x19, x23, lsl 4
	ldr	x0, [sp, 104]
	ldr	x1, [sp, 136]
	str	x1, [x23, 4264]
	cmp	x0, x25
	beq	.L27
	ldr	x1, [sp, 120]
	add	x1, x1, 1
	bl	_ZdlPvm
.L27:
	ldp	x19, x20, [sp, 16]
	mov	w0, w21
	ldp	x21, x22, [sp, 32]
	ldp	x23, x24, [sp, 48]
	ldr	x25, [sp, 64]
	ldp	x29, x30, [sp], 144
	.cfi_remember_state
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 25
	.cfi_restore 23
	.cfi_restore 24
	.cfi_restore 21
	.cfi_restore 22
	.cfi_restore 19
	.cfi_restore 20
	.cfi_def_cfa_offset 0
	ret
.L70:
	.cfi_restore_state
	ldrb	w0, [x23]
	strb	w0, [sp, 120]
	b	.L35
	.p2align 2,,3
.L69:
	add	x1, sp, 88
	add	x0, sp, 104
	mov	x2, 0
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_createERmm
	ldr	x1, [sp, 88]
	str	x0, [sp, 104]
	str	x1, [sp, 120]
.L32:
	mov	x2, x22
	mov	x1, x23
	bl	memcpy
	ldr	x22, [sp, 88]
	ldr	x0, [sp, 104]
	b	.L34
	.p2align 2,,3
.L72:
	ldr	x2, [sp, 112]
	cbz	x2, .L37
	cmp	x2, 1
	beq	.L74
	mov	x1, x25
	bl	memcpy
	ldr	x2, [sp, 112]
	ldr	x0, [x22, 4232]
.L37:
	str	x2, [x22, 4240]
	strb	wzr, [x0, x2]
	ldr	x0, [sp, 104]
	b	.L39
.L73:
	ldr	q0, [sp, 112]
	add	x1, x19, x1
	str	x3, [x22, 4232]
	str	q0, [x1, 4240]
.L41:
	str	x25, [sp, 104]
	add	x25, sp, 120
	mov	x0, x25
	b	.L39
.L44:
	mov	x21, 1
	b	.L28
.L55:
	mov	x21, 12
	b	.L28
.L71:
	mov	x0, x25
	b	.L32
.L45:
	mov	x21, 2
	b	.L28
.L46:
	mov	x21, 3
	b	.L28
.L47:
	mov	x21, 4
	b	.L28
.L48:
	mov	x21, 5
	b	.L28
.L49:
	mov	x21, 6
	b	.L28
.L50:
	mov	x21, 7
	b	.L28
.L51:
	mov	x21, 8
	b	.L28
.L52:
	mov	x21, 9
	b	.L28
.L53:
	mov	x21, 10
	b	.L28
.L54:
	mov	x21, 11
	b	.L28
.L56:
	mov	x21, 13
	b	.L28
.L57:
	mov	x21, 14
	b	.L28
.L74:
	ldrb	w1, [sp, 120]
	strb	w1, [x0]
	ldr	x2, [sp, 112]
	ldr	x0, [x22, 4232]
	b	.L37
	.cfi_endproc
.LFE1996:
	.size	_ZN5Timer5startEjjRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEi, .-_ZN5Timer5startEjjRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEi
	.align	2
	.p2align 4,,11
	.global	_ZN5Timer4stopEj
	.type	_ZN5Timer4stopEj, %function
_ZN5Timer4stopEj:
.LFB2001:
	.cfi_startproc
	uxtw	x4, w1
	cmp	w1, 1023
	bhi	.L80
	lsr	x5, x4, 6
	add	x6, x0, 128
	mov	x3, 1
	lsl	x3, x3, x1
	ldr	x2, [x0, x5, lsl 3]
	str	wzr, [x6, x4, lsl 2]
	orr	x2, x2, x3
	str	x2, [x0, x5, lsl 3]
	ret
.L80:
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	adrp	x0, .LC2
	mov	x1, x4
	mov	x29, sp
	add	x0, x0, :lo12:.LC2
	mov	x2, 1024
	bl	_ZSt24__throw_out_of_range_fmtPKcz
	.cfi_endproc
.LFE2001:
	.size	_ZN5Timer4stopEj, .-_ZN5Timer4stopEj
	.align	2
	.p2align 4,,11
	.global	_ZN5Timer11setDurationEjj
	.type	_ZN5Timer11setDurationEjj, %function
_ZN5Timer11setDurationEjj:
.LFB2002:
	.cfi_startproc
	lsl	w3, w2, 4
	mov	w4, 19923
	sub	w3, w3, w2
	movk	w4, 0x1062, lsl 16
	uxtw	x2, w1
	lsl	w3, w3, 1
	umull	x3, w3, w4
	cmp	w1, 1023
	bhi	.L86
	add	x2, x2, x2, lsl 1
	lsr	x3, x3, 38
	add	x0, x0, x2, lsl 4
	str	w3, [x0, 4268]
	ret
.L86:
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	mov	x1, x2
	adrp	x0, .LC2
	mov	x29, sp
	add	x0, x0, :lo12:.LC2
	mov	x2, 1024
	bl	_ZSt24__throw_out_of_range_fmtPKcz
	.cfi_endproc
.LFE2002:
	.size	_ZN5Timer11setDurationEjj, .-_ZN5Timer11setDurationEjj
	.align	2
	.p2align 4,,11
	.global	_ZN5Timer18findAvailableTimerEv
	.type	_ZN5Timer18findAvailableTimerEv, %function
_ZN5Timer18findAvailableTimerEv:
.LFB2003:
	.cfi_startproc
	ldr	x1, [x0]
	mov	x2, x0
	cbnz	x1, .L90
	ldr	x1, [x0, 8]
	cbnz	x1, .L91
	ldr	x1, [x0, 16]
	cbnz	x1, .L92
	ldr	x1, [x0, 24]
	cbnz	x1, .L93
	ldr	x1, [x0, 32]
	cbnz	x1, .L94
	ldr	x1, [x0, 40]
	cbnz	x1, .L95
	ldr	x1, [x0, 48]
	cbnz	x1, .L96
	ldr	x1, [x0, 56]
	cbnz	x1, .L97
	ldr	x1, [x0, 64]
	cbnz	x1, .L98
	ldr	x1, [x0, 72]
	cbnz	x1, .L99
	ldr	x1, [x0, 80]
	cbnz	x1, .L100
	ldr	x1, [x0, 88]
	cbnz	x1, .L101
	ldr	x1, [x0, 96]
	cbnz	x1, .L102
	ldr	x1, [x0, 104]
	cbnz	x1, .L103
	ldr	x1, [x0, 112]
	cbnz	x1, .L104
	ldr	x1, [x0, 120]
	mov	x0, 15
	cbnz	x1, .L88
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	adrp	x1, .LC3
	adrp	x0, .LC4
	mov	x29, sp
	mov	x3, 1024
	add	x1, x1, :lo12:.LC3
	add	x0, x0, :lo12:.LC4
	mov	x2, x3
	bl	_ZSt24__throw_out_of_range_fmtPKcz
.L90:
	.cfi_def_cfa_offset 0
	.cfi_restore 29
	.cfi_restore 30
	mov	x0, 0
.L88:
	rbit	x1, x1
	clz	x1, x1
	add	x0, x1, x0, lsl 6
	mov	x1, 1
	lsr	x3, x0, 6
	lsl	x1, x1, x0
	ldr	x4, [x2, x3, lsl 3]
	bic	x1, x4, x1
	str	x1, [x2, x3, lsl 3]
	ret
.L91:
	mov	x0, 1
	b	.L88
.L102:
	mov	x0, 12
	b	.L88
.L92:
	mov	x0, 2
	b	.L88
.L93:
	mov	x0, 3
	b	.L88
.L94:
	mov	x0, 4
	b	.L88
.L95:
	mov	x0, 5
	b	.L88
.L96:
	mov	x0, 6
	b	.L88
.L97:
	mov	x0, 7
	b	.L88
.L98:
	mov	x0, 8
	b	.L88
.L99:
	mov	x0, 9
	b	.L88
.L100:
	mov	x0, 10
	b	.L88
.L101:
	mov	x0, 11
	b	.L88
.L103:
	mov	x0, 13
	b	.L88
.L104:
	mov	x0, 14
	b	.L88
	.cfi_endproc
.LFE2003:
	.size	_ZN5Timer18findAvailableTimerEv, .-_ZN5Timer18findAvailableTimerEv
	.section	.rodata._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2IS3_EEPKcRKS3_.str1.8,"aMS",@progbits,1
	.align	3
.LC5:
	.string	"basic_string: construction from null is not valid"
	.section	.text._ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2IS3_EEPKcRKS3_,"axG",@progbits,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC5IS3_EEPKcRKS3_,comdat
	.align	2
	.p2align 4,,11
	.weak	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2IS3_EEPKcRKS3_
	.type	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2IS3_EEPKcRKS3_, %function
_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2IS3_EEPKcRKS3_:
.LFB2267:
	.cfi_startproc
	stp	x29, x30, [sp, -64]!
	.cfi_def_cfa_offset 64
	.cfi_offset 29, -64
	.cfi_offset 30, -56
	mov	x29, sp
	stp	x19, x20, [sp, 16]
	stp	x21, x22, [sp, 32]
	.cfi_offset 19, -48
	.cfi_offset 20, -40
	.cfi_offset 21, -32
	.cfi_offset 22, -24
	add	x22, x0, 16
	str	x22, [x0]
	cbz	x1, .L119
	mov	x20, x0
	mov	x21, x1
	mov	x0, x1
	bl	strlen
	str	x0, [sp, 56]
	mov	x19, x0
	cmp	x0, 15
	bhi	.L120
	cmp	x0, 1
	bne	.L112
	ldrb	w0, [x21]
	str	x19, [x20, 8]
	strb	w0, [x20, 16]
	strb	wzr, [x22, x19]
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x29, x30, [sp], 64
	.cfi_remember_state
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 21
	.cfi_restore 22
	.cfi_restore 19
	.cfi_restore 20
	.cfi_def_cfa_offset 0
	ret
	.p2align 2,,3
.L112:
	.cfi_restore_state
	cbnz	x0, .L111
	str	x19, [x20, 8]
	strb	wzr, [x22, x19]
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x29, x30, [sp], 64
	.cfi_remember_state
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 21
	.cfi_restore 22
	.cfi_restore 19
	.cfi_restore 20
	.cfi_def_cfa_offset 0
	ret
	.p2align 2,,3
.L120:
	.cfi_restore_state
	add	x1, sp, 56
	mov	x0, x20
	mov	x2, 0
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE9_M_createERmm
	ldr	x1, [sp, 56]
	mov	x22, x0
	str	x0, [x20]
	str	x1, [x20, 16]
.L111:
	mov	x2, x19
	mov	x1, x21
	mov	x0, x22
	bl	memcpy
	ldr	x22, [x20]
	ldr	x19, [sp, 56]
	str	x19, [x20, 8]
	strb	wzr, [x22, x19]
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x29, x30, [sp], 64
	.cfi_remember_state
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 21
	.cfi_restore 22
	.cfi_restore 19
	.cfi_restore 20
	.cfi_def_cfa_offset 0
	ret
.L119:
	.cfi_restore_state
	adrp	x0, .LC5
	add	x0, x0, :lo12:.LC5
	bl	_ZSt19__throw_logic_errorPKc
	.cfi_endproc
.LFE2267:
	.size	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2IS3_EEPKcRKS3_, .-_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2IS3_EEPKcRKS3_
	.weak	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_
	.set	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_,_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2IS3_EEPKcRKS3_
	.section	.rodata.str1.8
	.align	3
.LC6:
	.string	"./config/"
	.align	3
.LC7:
	.string	"menu/"
	.align	3
.LC8:
	.string	"dialogue/"
	.align	3
.LC9:
	.string	"grid/"
	.align	3
.LC10:
	.string	"node/"
	.align	3
.LC11:
	.string	"bg/"
	.align	3
.LC12:
	.string	"entity/"
	.align	3
.LC13:
	.string	"body/"
	.align	3
.LC14:
	.string	"personality/"
	.align	3
.LC15:
	.string	"tree/"
	.section	.text.startup
	.align	2
	.p2align 4,,11
	.type	_Z41__static_initialization_and_destruction_0ii.constprop.0, %function
_Z41__static_initialization_and_destruction_0ii.constprop.0:
.LFB2582:
	.cfi_startproc
	.cfi_personality 0x9b,DW.ref.__gxx_personality_v0
	.cfi_lsda 0x1b,.LLSDA2582
	stp	x29, x30, [sp, -128]!
	.cfi_def_cfa_offset 128
	.cfi_offset 29, -128
	.cfi_offset 30, -120
	mov	x0, 9
	adrp	x1, .LC7
	mov	x29, sp
	stp	x19, x20, [sp, 16]
	.cfi_offset 19, -112
	.cfi_offset 20, -104
	adrp	x19, .LANCHOR0
	add	x3, x19, :lo12:.LANCHOR0
	stp	x21, x22, [sp, 32]
	.cfi_offset 21, -96
	.cfi_offset 22, -88
	add	x21, sp, 96
	add	x1, x1, :lo12:.LC7
	stp	x23, x24, [sp, 48]
	.cfi_offset 23, -80
	.cfi_offset 24, -72
	add	x23, sp, 88
	mov	x2, x23
	str	x25, [sp, 64]
	.cfi_offset 25, -64
	adrp	x20, .LC6
	str	x0, [x19, #:lo12:.LANCHOR0]
	add	x20, x20, :lo12:.LC6
	mov	x0, x21
	str	x20, [x3, 8]
.LEHB0:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_
.LEHE0:
	mov	x2, x20
	mov	x0, x21
	mov	x1, 0
.LEHB1:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6insertEmPKc
.LEHE1:
	add	x2, x19, :lo12:.LANCHOR0
	mov	x1, x0
	add	x0, x2, 16
	add	x22, sp, 112
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1EOS4_
	ldr	x0, [sp, 96]
	cmp	x0, x22
	beq	.L122
	ldr	x1, [sp, 112]
	add	x1, x1, 1
	bl	_ZdlPvm
.L122:
	adrp	x0, :got:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	ldr	x0, [x0, :got_lo12:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev]
	add	x24, x19, :lo12:.LANCHOR0
	add	x1, x24, 16
	adrp	x20, __dso_handle
	add	x2, x20, :lo12:__dso_handle
	bl	__cxa_atexit
	ldr	x24, [x24, 8]
	mov	x2, x23
	mov	x0, x21
	adrp	x1, .LC8
	add	x1, x1, :lo12:.LC8
.LEHB2:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_
.LEHE2:
	mov	x2, x24
	mov	x0, x21
	mov	x1, 0
.LEHB3:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6insertEmPKc
.LEHE3:
	add	x2, x19, :lo12:.LANCHOR0
	mov	x1, x0
	add	x0, x2, 48
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1EOS4_
	ldr	x0, [sp, 96]
	cmp	x0, x22
	beq	.L123
	ldr	x1, [sp, 112]
	add	x1, x1, 1
	bl	_ZdlPvm
.L123:
	adrp	x0, :got:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	ldr	x0, [x0, :got_lo12:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev]
	add	x24, x19, :lo12:.LANCHOR0
	add	x1, x24, 48
	add	x2, x20, :lo12:__dso_handle
	bl	__cxa_atexit
	ldr	x24, [x24, 8]
	mov	x2, x23
	mov	x0, x21
	adrp	x1, .LC9
	add	x1, x1, :lo12:.LC9
.LEHB4:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_
.LEHE4:
	mov	x2, x24
	mov	x0, x21
	mov	x1, 0
.LEHB5:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6insertEmPKc
.LEHE5:
	add	x2, x19, :lo12:.LANCHOR0
	mov	x1, x0
	add	x0, x2, 80
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1EOS4_
	ldr	x0, [sp, 96]
	cmp	x0, x22
	beq	.L124
	ldr	x1, [sp, 112]
	add	x1, x1, 1
	bl	_ZdlPvm
.L124:
	adrp	x0, :got:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	ldr	x0, [x0, :got_lo12:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev]
	add	x24, x19, :lo12:.LANCHOR0
	add	x1, x24, 80
	add	x2, x20, :lo12:__dso_handle
	bl	__cxa_atexit
	ldr	x24, [x24, 8]
	mov	x2, x23
	mov	x0, x21
	adrp	x1, .LC10
	add	x1, x1, :lo12:.LC10
.LEHB6:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_
.LEHE6:
	mov	x2, x24
	mov	x0, x21
	mov	x1, 0
.LEHB7:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE6insertEmPKc
.LEHE7:
	add	x2, x19, :lo12:.LANCHOR0
	mov	x1, x0
	add	x0, x2, 112
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1EOS4_
	ldr	x0, [sp, 96]
	cmp	x0, x22
	beq	.L125
	ldr	x1, [sp, 112]
	add	x1, x1, 1
	bl	_ZdlPvm
.L125:
	adrp	x0, :got:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	ldr	x0, [x0, :got_lo12:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev]
	add	x24, x19, :lo12:.LANCHOR0
	add	x1, x24, 112
	add	x2, x20, :lo12:__dso_handle
	bl	__cxa_atexit
	mov	x2, x23
	mov	x0, x21
	adrp	x1, .LC11
	add	x1, x1, :lo12:.LC11
.LEHB8:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_
.LEHE8:
	ldp	x0, x1, [x24, 80]
	add	x8, x24, 144
	mov	x2, x21
.LEHB9:
	bl	_ZStplIcSt11char_traitsIcESaIcEENSt7__cxx1112basic_stringIT_T0_T1_EERKS8_OS8_.isra.0
.LEHE9:
	ldr	x0, [sp, 96]
	cmp	x0, x22
	beq	.L126
	ldr	x1, [sp, 112]
	add	x1, x1, 1
	bl	_ZdlPvm
.L126:
	adrp	x0, :got:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	ldr	x0, [x0, :got_lo12:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev]
	add	x24, x19, :lo12:.LANCHOR0
	add	x1, x24, 144
	add	x2, x20, :lo12:__dso_handle
	bl	__cxa_atexit
	mov	x2, x23
	mov	x0, x21
	adrp	x1, .LC12
	add	x1, x1, :lo12:.LC12
.LEHB10:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_
.LEHE10:
	ldp	x0, x1, [x24, 112]
	add	x8, x24, 176
	mov	x2, x21
.LEHB11:
	bl	_ZStplIcSt11char_traitsIcESaIcEENSt7__cxx1112basic_stringIT_T0_T1_EERKS8_OS8_.isra.0
.LEHE11:
	ldr	x0, [sp, 96]
	cmp	x0, x22
	beq	.L127
	ldr	x1, [sp, 112]
	add	x1, x1, 1
	bl	_ZdlPvm
.L127:
	adrp	x0, :got:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	ldr	x0, [x0, :got_lo12:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev]
	add	x24, x19, :lo12:.LANCHOR0
	add	x25, x24, 176
	add	x2, x20, :lo12:__dso_handle
	mov	x1, x25
	bl	__cxa_atexit
	mov	x2, x23
	mov	x0, x21
	adrp	x1, .LC13
	add	x1, x1, :lo12:.LC13
.LEHB12:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_
.LEHE12:
	ldp	x0, x1, [x24, 176]
	add	x8, x24, 208
	mov	x2, x21
.LEHB13:
	bl	_ZStplIcSt11char_traitsIcESaIcEENSt7__cxx1112basic_stringIT_T0_T1_EERKS8_OS8_.isra.0
.LEHE13:
	ldr	x0, [sp, 96]
	cmp	x0, x22
	beq	.L128
	ldr	x1, [sp, 112]
	add	x1, x1, 1
	bl	_ZdlPvm
.L128:
	adrp	x0, :got:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	ldr	x0, [x0, :got_lo12:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev]
	add	x24, x19, :lo12:.LANCHOR0
	add	x1, x24, 208
	add	x2, x20, :lo12:__dso_handle
	bl	__cxa_atexit
	mov	x2, x23
	mov	x0, x21
	adrp	x1, .LC14
	add	x1, x1, :lo12:.LC14
.LEHB14:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_
.LEHE14:
	ldp	x0, x1, [x24, 176]
	add	x8, x24, 240
	mov	x2, x21
.LEHB15:
	bl	_ZStplIcSt11char_traitsIcESaIcEENSt7__cxx1112basic_stringIT_T0_T1_EERKS8_OS8_.isra.0
.LEHE15:
	ldr	x0, [sp, 96]
	cmp	x0, x22
	beq	.L129
	ldr	x1, [sp, 112]
	add	x1, x1, 1
	bl	_ZdlPvm
.L129:
	adrp	x0, :got:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	ldr	x0, [x0, :got_lo12:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev]
	add	x24, x19, :lo12:.LANCHOR0
	add	x25, x24, 240
	add	x2, x20, :lo12:__dso_handle
	mov	x1, x25
	bl	__cxa_atexit
	mov	x2, x23
	mov	x0, x21
	adrp	x1, .LC15
	add	x1, x1, :lo12:.LC15
.LEHB16:
	bl	_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC1IS3_EEPKcRKS3_
.LEHE16:
	ldp	x0, x1, [x24, 240]
	mov	x2, x21
	add	x8, x24, 272
.LEHB17:
	bl	_ZStplIcSt11char_traitsIcESaIcEENSt7__cxx1112basic_stringIT_T0_T1_EERKS8_OS8_.isra.0
.LEHE17:
	ldr	x0, [sp, 96]
	cmp	x0, x22
	beq	.L130
	ldr	x1, [sp, 112]
	add	x1, x1, 1
	bl	_ZdlPvm
.L130:
	adrp	x0, :got:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev
	ldr	x0, [x0, :got_lo12:_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEED1Ev]
	add	x20, x20, :lo12:__dso_handle
	add	x19, x19, :lo12:.LANCHOR0
	mov	x2, x20
	add	x1, x19, 272
	add	x19, x19, 304
	bl	__cxa_atexit
	mov	x0, x19
.LEHB18:
	bl	_ZNSt8ios_base4InitC1Ev
	ldp	x21, x22, [sp, 32]
	mov	x2, x20
	ldp	x23, x24, [sp, 48]
	mov	x1, x19
	ldp	x19, x20, [sp, 16]
	ldr	x25, [sp, 64]
	ldp	x29, x30, [sp], 128
	.cfi_remember_state
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 25
	.cfi_restore 23
	.cfi_restore 24
	.cfi_restore 21
	.cfi_restore 22
	.cfi_restore 19
	.cfi_restore 20
	.cfi_def_cfa_offset 0
	adrp	x0, :got:_ZNSt8ios_base4InitD1Ev
	ldr	x0, [x0, :got_lo12:_ZNSt8ios_base4InitD1Ev]
	b	__cxa_atexit
.L155:
	.cfi_restore_state
.L161:
	ldr	x2, [sp, 96]
	mov	x19, x0
	cmp	x2, x22
	beq	.L148
.L159:
	ldr	x1, [sp, 112]
	mov	x0, x2
	add	x1, x1, 1
	bl	_ZdlPvm
.L148:
	mov	x0, x19
	bl	_Unwind_Resume
.LEHE18:
.L153:
	b	.L161
.L151:
	b	.L161
.L152:
	b	.L161
.L149:
	b	.L161
.L150:
	b	.L161
.L156:
	b	.L161
.L157:
	ldr	x2, [sp, 96]
	add	x1, sp, 112
	mov	x19, x0
	cmp	x2, x1
	bne	.L159
	b	.L148
.L154:
	b	.L161
	.cfi_endproc
.LFE2582:
	.global	__gxx_personality_v0
	.section	.gcc_except_table,"a",@progbits
.LLSDA2582:
	.byte	0xff
	.byte	0xff
	.byte	0x1
	.uleb128 .LLSDACSE2582-.LLSDACSB2582
.LLSDACSB2582:
	.uleb128 .LEHB0-.LFB2582
	.uleb128 .LEHE0-.LEHB0
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB1-.LFB2582
	.uleb128 .LEHE1-.LEHB1
	.uleb128 .L157-.LFB2582
	.uleb128 0
	.uleb128 .LEHB2-.LFB2582
	.uleb128 .LEHE2-.LEHB2
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB3-.LFB2582
	.uleb128 .LEHE3-.LEHB3
	.uleb128 .L156-.LFB2582
	.uleb128 0
	.uleb128 .LEHB4-.LFB2582
	.uleb128 .LEHE4-.LEHB4
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB5-.LFB2582
	.uleb128 .LEHE5-.LEHB5
	.uleb128 .L155-.LFB2582
	.uleb128 0
	.uleb128 .LEHB6-.LFB2582
	.uleb128 .LEHE6-.LEHB6
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB7-.LFB2582
	.uleb128 .LEHE7-.LEHB7
	.uleb128 .L154-.LFB2582
	.uleb128 0
	.uleb128 .LEHB8-.LFB2582
	.uleb128 .LEHE8-.LEHB8
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB9-.LFB2582
	.uleb128 .LEHE9-.LEHB9
	.uleb128 .L153-.LFB2582
	.uleb128 0
	.uleb128 .LEHB10-.LFB2582
	.uleb128 .LEHE10-.LEHB10
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB11-.LFB2582
	.uleb128 .LEHE11-.LEHB11
	.uleb128 .L152-.LFB2582
	.uleb128 0
	.uleb128 .LEHB12-.LFB2582
	.uleb128 .LEHE12-.LEHB12
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB13-.LFB2582
	.uleb128 .LEHE13-.LEHB13
	.uleb128 .L151-.LFB2582
	.uleb128 0
	.uleb128 .LEHB14-.LFB2582
	.uleb128 .LEHE14-.LEHB14
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB15-.LFB2582
	.uleb128 .LEHE15-.LEHB15
	.uleb128 .L150-.LFB2582
	.uleb128 0
	.uleb128 .LEHB16-.LFB2582
	.uleb128 .LEHE16-.LEHB16
	.uleb128 0
	.uleb128 0
	.uleb128 .LEHB17-.LFB2582
	.uleb128 .LEHE17-.LEHB17
	.uleb128 .L149-.LFB2582
	.uleb128 0
	.uleb128 .LEHB18-.LFB2582
	.uleb128 .LEHE18-.LEHB18
	.uleb128 0
	.uleb128 0
.LLSDACSE2582:
	.section	.text.startup
	.size	_Z41__static_initialization_and_destruction_0ii.constprop.0, .-_Z41__static_initialization_and_destruction_0ii.constprop.0
	.align	2
	.p2align 4,,11
	.type	_GLOBAL__sub_I__ZN5Timer3runEv, %function
_GLOBAL__sub_I__ZN5Timer3runEv:
.LFB2571:
	.cfi_startproc
	b	_Z41__static_initialization_and_destruction_0ii.constprop.0
	.cfi_endproc
.LFE2571:
	.size	_GLOBAL__sub_I__ZN5Timer3runEv, .-_GLOBAL__sub_I__ZN5Timer3runEv
	.section	.init_array,"aw"
	.align	3
	.xword	_GLOBAL__sub_I__ZN5Timer3runEv
	.bss
	.align	3
	.set	.LANCHOR0,. + 0
	.type	_ZL7CFG_DIR, %object
	.size	_ZL7CFG_DIR, 16
_ZL7CFG_DIR:
	.zero	16
	.type	_ZL8MENU_DIR, %object
	.size	_ZL8MENU_DIR, 32
_ZL8MENU_DIR:
	.zero	32
	.type	_ZL12DIALOGUE_DIR, %object
	.size	_ZL12DIALOGUE_DIR, 32
_ZL12DIALOGUE_DIR:
	.zero	32
	.type	_ZL8GRID_DIR, %object
	.size	_ZL8GRID_DIR, 32
_ZL8GRID_DIR:
	.zero	32
	.type	_ZL8NODE_DIR, %object
	.size	_ZL8NODE_DIR, 32
_ZL8NODE_DIR:
	.zero	32
	.type	_ZL6BG_DIR, %object
	.size	_ZL6BG_DIR, 32
_ZL6BG_DIR:
	.zero	32
	.type	_ZL10ENTITY_DIR, %object
	.size	_ZL10ENTITY_DIR, 32
_ZL10ENTITY_DIR:
	.zero	32
	.type	_ZL8BODY_DIR, %object
	.size	_ZL8BODY_DIR, 32
_ZL8BODY_DIR:
	.zero	32
	.type	_ZL15PERSONALITY_DIR, %object
	.size	_ZL15PERSONALITY_DIR, 32
_ZL15PERSONALITY_DIR:
	.zero	32
	.type	_ZL8TREE_DIR, %object
	.size	_ZL8TREE_DIR, 32
_ZL8TREE_DIR:
	.zero	32
	.type	_ZStL8__ioinit, %object
	.size	_ZStL8__ioinit, 1
_ZStL8__ioinit:
	.zero	1
	.hidden	DW.ref.__gxx_personality_v0
	.weak	DW.ref.__gxx_personality_v0
	.section	.data.rel.local.DW.ref.__gxx_personality_v0,"awG",@progbits,DW.ref.__gxx_personality_v0,comdat
	.align	3
	.type	DW.ref.__gxx_personality_v0, %object
	.size	DW.ref.__gxx_personality_v0, 8
DW.ref.__gxx_personality_v0:
	.xword	__gxx_personality_v0
	.hidden	__dso_handle
	.ident	"GCC: (Debian 12.2.0-14+deb12u1) 12.2.0"
	.section	.note.GNU-stack,"",@progbits
