	.section	__TEXT,__text,regular,pure_instructions
	.build_version ios, 12, 0	sdk_version 11, 1
	.globl	_comparePath            ; -- Begin function comparePath
	.p2align	2
_comparePath:                           ; @comparePath
	.cfi_startproc
; %bb.0:
	ldrb	w8, [x0]
	cmp	w8, #47                 ; =47
	b.ne	LBB0_18
; %bb.1:
	ldrb	w8, [x0, #1]
	cmp	w8, #117                ; =117
	b.ne	LBB0_18
; %bb.2:
	ldrb	w8, [x0, #2]
	cmp	w8, #115                ; =115
	b.ne	LBB0_18
; %bb.3:
	ldrb	w8, [x0, #3]
	cmp	w8, #114                ; =114
	b.ne	LBB0_18
; %bb.4:
	ldrb	w8, [x0, #4]
	cmp	w8, #47                 ; =47
	b.ne	LBB0_18
; %bb.5:
	ldrb	w8, [x0, #5]
	cmp	w8, #108                ; =108
	b.ne	LBB0_18
; %bb.6:
	ldrb	w8, [x0, #6]
	cmp	w8, #111                ; =111
	b.ne	LBB0_18
; %bb.7:
	ldrb	w8, [x0, #7]
	cmp	w8, #99                 ; =99
	b.ne	LBB0_18
; %bb.8:
	ldrb	w8, [x0, #8]
	cmp	w8, #97                 ; =97
	b.ne	LBB0_18
; %bb.9:
	ldrb	w8, [x0, #9]
	cmp	w8, #108                ; =108
	b.ne	LBB0_18
; %bb.10:
	ldrb	w8, [x0, #10]
	cmp	w8, #47                 ; =47
	b.ne	LBB0_18
; %bb.11:
	ldrb	w8, [x0, #11]
	cmp	w8, #122                ; =122
	b.ne	LBB0_18
; %bb.12:
	ldrb	w8, [x0, #12]
	cmp	w8, #104                ; =104
	b.ne	LBB0_18
; %bb.13:
	ldrb	w8, [x0, #13]
	cmp	w8, #117                ; =117
	b.ne	LBB0_18
; %bb.14:
	ldrb	w8, [x0, #14]
	cmp	w8, #111                ; =111
	b.ne	LBB0_18
; %bb.15:
	ldrb	w8, [x0, #15]
	cmp	w8, #119                ; =119
	b.ne	LBB0_18
; %bb.16:
	ldrb	w8, [x0, #16]
	cmp	w8, #101                ; =101
	b.ne	LBB0_18
; %bb.17:
	ldrb	w8, [x0, #17]
	cmp	w8, #105                ; =105
	cset	w0, eq
	ret
LBB0_18:
	mov	w0, #0
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
