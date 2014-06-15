	.section "start", "ax"

	.extern main
	.global _start

	.equ	STACK_TOP,	0xc8000000
	.equ	ABT_STACK,	STACK_TOP			@64K
	.equ	UND_STACK,	(ABT_STACK - 0x10000) @64K
	.equ	FIQ_STACK,	(UND_STACK - 0x10000) @64K
	.equ	IRQ_STACK,	(FIQ_STACK - 0x10000) @512K
	.equ	SVC_STACK,	(IRQ_STACK - 0x80000) @32M,system run in svc mode
	.equ	USR_STACK,	(SVC_STACK - 0x200000)

_start:
	bl		clean_dcache
	bl		invalidate_icache

	msr       cpsr_c, #0xd7 @ abort mode with fiq and irq msked
	ldr       r13, =ABT_STACK

	msr       cpsr_c, #0xdb @ undefine mode with fiq and irq msked
	ldr       r13, =UND_STACK

	msr       cpsr_c, #0xd1      @ fiq mode with fiq and irq msked
	ldr       r13, =FIQ_STACK

	msr       cpsr_c, #0xd2      @ irq mode with fiq and irq msked
	ldr       r13, =IRQ_STACK

	@***	  switch to system mode for initialization
	msr       cpsr_c, #0xdf      @ user sys mode with fiq and irq msked
	ldr       r13, =USR_STACK


	@***	  copy exception table to 0x00000000
	mov		  r8, #0x00
	ldr		  r9, =exception_table
	ldmia	  r9!,{r0-r7}
	stmia     r8!,{r0-r7}
	ldmia	  r9!,{r0-r7}
	stmia     r8!,{r0-r7}


	@***		clear bss
	ldr		r0,=_bss_start
	ldr		r1,=_bss_end
	mov		r2,#0
1:
	str		r2,[r0],#4
	cmp		r0,r1
	bcc		1b

	@*** switch to svc mode, enable irq
	msr       cpsr_c, #0xd3 @ svc mode with fiq and irq msked
	ldr       r13, =SVC_STACK
	msr       cpsr_c, #0x13 @XXX svc mode with fiq and irq unmsked

	bl	main

exit:
	b	exit
	.ltorg

	.extern ucos_irq_handler

exception_table:
	ldr		pc,reset_vector
	ldr		pc,undef_vector
	ldr		pc,swi_vector
	ldr		pc,pabt_vector
	ldr		pc,dabt_vector
	ldr		pc,reserved_vector
	ldr		pc,irq_vector
	ldr		pc,fiq_vector

reset_vector:
	.word	reset_handler
undef_vector:
	.word	undef_handler
swi_vector:
	.word	swi_handler
pabt_vector:
	.word	pabt_handler
dabt_vector:
	.word	dabt_handler
reserved_vector:
	.word	reserved_handler
irq_vector:
	.word	ucos_irq_handler
fiq_vector:
	.word	fiq_handler

	.equ VIC0_ADDR,0x71200000
	.equ VIC1_ADDR,0x71300000
	.equ VIC_VECT_ADDR,0xF00
	.equ VIC_INT_EN,0x10
	.equ VIC_INT_EN_CLR,0x14
	.equ IRQ_MAX,64
	.extern debug_s_irq_handler_out @void debug_s_irq_handler_out(void *stack)
	.extern debug_s_irq_handler_in @void debug_s_irq_handler_in(void *stack)
s_irq_handler:
	sub		lr,lr,#4
	stmfd	r13!,{r0-r12,lr}
	mrs		r0,spsr
	stmfd	r13!,{r0}

	@*** debug irq
	@mov		r0, r13 @r0:stack frame pointer
	@ldr		r1, = debug_s_irq_handler_in
	@mov		lr,pc
	@bx		r1
	
	@*** not sure:need to read VIC1 VECT_ADDR ??
	@ldr	r5,=(VIC1_ADDR+VIC_VECT_ADDR)
	@ldr	r1,[r5]
	
	@ mask_and_ack irq
	@ note, we must read VIC_VECT_ADDR to clear vic interrupt request 
	@ before enable irq again,or once irq reenable,it will occurs immediately
	@ XXX, need to read VIC1 VIC_VECT_ADDR ??
	ldr	r4,=(VIC0_ADDR+VIC_VECT_ADDR)
	ldr	r0,[r4]

	msr		cpsr_c, #0x13 @switch to svc mode with irq enable
	stmfd	r13!,{lr} @ save lr_svc

	@ XXX handle irq
	ldr	r1,=irq_c_handler @r0 is intterupt source number
	mov	lr,pc
	bx	r1

	ldmfd	r13!,{lr} @restore lr_svc
	msr		cpsr_c, #(0x80 | 0x12) @switch to irq mode with irq disable

	@ unmask irq,it will allow lower priority irq source requesting interrupt
	ldr	r4,=(VIC0_ADDR+VIC_VECT_ADDR)
	ldr	r0,=IRQ_MAX @unmask vic0 interrupt
	str	r0,[r4]

	@*** not sure:need to write VIC1 VECT_ADDR ??
	@ldr	r5,=(VIC1_ADDR+VIC_VECT_ADDR)
	@ldr	r0,[r5]

	@*** debug irq
	@mov		r0, r13 @r0:stack frame pointer
	@ldr		r1, = debug_s_irq_handler_out
	@mov		lr,pc
	@bx		r1

	ldmfd	r13!,{r0}
	msr		spsr_csxf,r0
	ldmfd	r13!,{r0-r12,pc}^
	.ltorg

reset_handler:
	b	c_reset_handler
undef_handler:
	b	c_undef_handler
swi_handler:
	b	c_swi_handler
pabt_handler:
	b	c_pabt_handler
dabt_handler:
	b	c_dabt_handler
reserved_handler:
	b	reserved_handler
fiq_handler:
	b	fiq_handler

	.global invalidate_icache
	.global invalidate_dcache
	.global clean_dcache
invalidate_icache:
	mcr	p15,0,r0,c7,c5,0
	mov	pc,lr

clean_dcache:
	mcr	p15,0,r0,c7,c10,0
	mov	pc,lr

invalidate_dcache:
	mcr	p15,0,r0,c7,c6,0

	.global switch_to_user_mode
	@ void switch_to_user_mode(void (*user_function)(void));
	@ this function will not return.
switch_to_user_mode:
	msr		cpsr_c, #(0xc0 | 0x13) @ switch to svc mode with irq,fiq disable
	mov		r1,#0x10 @ user mode with fiq,irq enable
	msr		spsr_cxsf,r1
	movs	pc,r0

