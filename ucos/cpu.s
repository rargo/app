		.equ VIC0_ADDR,0x71200000
		.equ VIC1_ADDR,0x71300000
		.equ VIC_IRQ_STATUS, 0x00
		.equ VIC_VECT_ADDR,0xF00
		.equ VIC_INT_EN,0x10
		.equ VIC_INT_EN_CLR,0x14
		.equ IRQ_MAX,64

		@we don't use irq mode handle irq, instead use svc mode, so we can make 
		@task switch only occur in svc mode. 
		@the registers that need to be saved:
		@r0-r3,r12,lr_irq,spsr_irq,lr_svc,
		@(sprs_svc will be saved in swi handler,here don't need to be saved)

		.global ucos_irq_handler
		.extern do_softirq
ucos_irq_handler:
		@***** save irq mode register: lr, spsr, and r0,r1
		stmfd	r13!,{r0,r1} @save r0,r1 for temp use
		mrs		r0,spsr
		stmfd	r13!,{r0}

		mov		r0,r13 @ r0: point to irq stack base
		add		r13,r13,#12 @restore irq sp, it means irq stack only need 12bytes

		sub		r1,lr,#4 @r1: irq return address
		@now:
		@1. irq stack:
		@		r1
		@		r0
		@		spsr  <----- r0:point to irq stack base
		@
		@2. r1 = lr_irq @r1: irq return address

		@****** switch to svc mode,irq disable, handle irq in svc mode,
		msr		cpsr_c, #(0x80 | 0x13) 
		stmfd	r13!,{r1} @save lr_irq
		stmfd	r13!,{r2-r3,r12} @save r2-r3,r12
		ldmfd	r0,{r2-r3,r12} @ r12 = r1(irq stack), r3 = r0(irq stack), r2 = spsr_irq(irq stack)
		stmfd	r13!,{r2-r3,r12} @save spsr, r0-r1
		stmfd	r13!,{lr} @save lr_svc @be careful with lr_svc, interrupt routine will change it.
		@svc stack now:
		@	lr_irq
		@	r12
		@	r0-r3
		@	spsr_irq
		@	lr_svc

		@***** handle irq
		bl	OSIntEnter

		@find which vic channel request irq
		mov		r0,#64 @ set irq to IRQ_MAX(invalid vic channel)
		@see if vic0 has irq request
		mov		r1,#31
		ldr		r3,=(VIC0_ADDR+VIC_IRQ_STATUS)
		ldr		r2,[r3]
		teq		r2,#0

		@else try vic1
		addeq	r1,r1,#32
		ldreq	r3,=(VIC1_ADDR+VIC_IRQ_STATUS)
		ldreq	r2,[r3]
		teqeq	r2,#0

		clzne	r2,r2
		subne	r0,r1,r2 @r0 get real vic channel now

		stmfd	r13!,{r11}
		mov		r11,r0 @save real vic channel in r11
		bl		irq_mask

		@ handle irq
		@ r0 is intterupt source number
		mov		r0,r11
		ldr	r3,=irq_c_handler 
		mov	lr,pc
		bx	r3

		mov		r0,r11
		bl		irq_unmask
		ldmfd	r13!,{r11}

		@ bl do_softirq

		bl	OSIntExit @may switch task here, task switch only allow to occur in svc mode with irq disable

		ldmfd	r13!,{lr} @restore svc mode lr
		ldmfd	r13!,{r0}
		msr		spsr_cxsf,r0 @ mov spsr_irq to spsr_svc
		ldmfd	r13!,{r0-r3,r12,pc}^
		.ltorg

		.extern c_swi_handler
		.global ucos_swi_handler

ucos_swi_handler:
		stmfd	r13!,{r0-r3,r12,lr}
		mrs		r0,spsr
		stmfd	r13!,{r0}

		ldr		r0,[r14,#-4] @get swi number
		bic		r0,r0,#0xff000000

		msr		cpsr_c, #0x13 @enable irq 

		ldr		r1,=c_swi_handler @in c_swi_handler, task switch may occur
		mov		lr,pc
		bx		r1

		msr		cpsr_c, #(0x80 | 0x13) @disable irq
		
		ldmfd	r13!,{r0} @spsr
		msr		spsr_cxsf, r0
		ldmfd	r13!,{r0-r3,r12,pc}^
		.ltorg
	
	
        .global irq_save_asm
        .global irq_restore_asm
        .global OS_TASK_SW
        .global OSStartHighRdy
        .global ucos_irq_handler

        .extern OSTCBCur
        .extern OSPrioCur
        .extern OSTCBHighRdy
        .extern OSPrioHighRdy
        .extern OSRunning
        .extern OSTaskSwHook
        .extern OSIntEnter
        .extern OSIntExit
        .extern os_need_reschedule

		@unsigned int irq_save_asm(void)
irq_save_asm:
		mrs	r0,cpsr
		orr	r1,r0,#0xc0	
		msr	cpsr_c,r1
		mov	pc,lr


		@void irq_restore_asm(unsigned long save_int_status)
irq_restore_asm:
		msr	cpsr_c,r0
		mov	pc,lr


        /*switch task in svc mode*/
		.global OS_TASK_SW
		.global OSIntCtxSw

		@task context registers:
		@r0-r12, r13_usr, r14_usr, r13_svc,r14_svc, cpsr

		@task stack registers:
		@		r14_svc(pc)
		@		r4~r11
		@		r0
		@		cpsr
		@		r14_usr
		@		r13_usr

		@save r13_svc to OSTCBCur->OSTCBStkPtr

		@void OS_TASK_SW(void)
		@void  OSIntCtxSw(void)
		@in ATPC,r0-r3,r12 is free to used,needn't to be save,
		@the reason we save r0 here,is because when task is init using 
		@OS_STK *OSTaskStkInit(void (*task)(void *pd),void  *pdata,OS_STK *ptos,INT16U opt)
		@task parameter pdata must be put in r0.
		@Interrupt must be disabled before call task switch function
OSIntCtxSw:
OS_TASK_SW:
		stmfd	r13!,{r0,r4-r11,lr} 
		mrs		r0,cpsr
		stmfd	r13!,{r0}

		@save r13_usr, r14_usr
		stmfd	r13!,{r13,r14}^

		ldr		r4,=OSTCBCur
		ldr		r5,=OSPrioCur
        ldr     r0,[r4]
        str     r13,[r0]   @OSTCBCur->OSTCBStkPtr = r13
        
		@ call OSTaskSwHook
        bl      OSTaskSwHook

		ldr		r6,=OSTCBHighRdy
		ldr		r7,=OSPrioHighRdy
		ldr		r0,[r6]
		ldrb	r1,[r7]
		str		r0,[r4] @ OSTCBCur = OSTCBHighRdy
		strb	r1,[r5] @ OSPrioCur = OSPrioHighRdy

		ldr		r0,[r6]
		ldr		r13,[r0] @ r13 = OSTCBHighRdy->OSTCBStkPtr

		@restore r13_usr, r14_usr
		ldmfd	r13!,{r13,r14}^

		ldmfd	r13!,{r0}
		msr		spsr_cxsf,r0 @interrupt has been disable, it's safe to use spsr here
		ldmfd	r13!,{r0,r4-r11,pc}^
        .ltorg


		@ interrupt must be disabled before call this
         /*use only once,OSStart call OSStartHighRdy to run the highest priority task*/
OSStartHighRdy:
        bl      OSTaskSwHook

        ldr     r0,=OSRunning
        mov     r1,#1
        str     r1,[r0]   @OSRunning = 1

		ldr		r0,=OSTCBCur
		ldr		r1,[r0]
		ldr		r13,[r1] @ r13 = OSTCBCur->OSTCBStkPtr

		@restore r13_usr, r14_usr
		ldmfd	r13!,{r13,r14}^

		ldmfd	r13!,{r0}
		msr		spsr_cxsf,r0 @interrupt has been disable, it's safe to use spsr here
		ldmfd	r13!,{r0,r4-r11,pc}^
        .ltorg


