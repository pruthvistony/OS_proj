// switching function

.global switch_to
.global switch_out
.global switch_to_ring3
.global rdmsr_read
.global wrmsr_write
.global switch_to_child

switch_to:
	// pushing all general purpose registers
	pushq %rax;
	pushq %rbx;
	pushq %rcx;
	pushq %rdx;
	pushq %rsi;
	//pushq %rdi;
	pushq %rbp;
	//pushq %rsp;

	pushq %r8;
	pushq %r9;
	pushq %r10;
	pushq %r11;
	pushq %r12;
	pushq %r13;
	pushq %r14;
	pushq %r15;

	movq %cr3, %rax;
	pushq %rax;

	//pushq %rip;					// pushing the program counter
	pushfq;						// push the eflags/rflags

	// actual switch to functionality
	pushq %rdi; 				//save me to my stack
	movq %rsp, 8(%rdi); 		// save my rsp value in pcb
	movq 8(%rsi), %rsp; 		// update the rsp value with the next process stack
	popq %rdi; 					// update me to next task

    // restore the registers according to the new stack
	popfq; 						// pop the eflags/rflags
	//popq %rip; 					// pop the program counter

	popq %rax;
	movq %rax, %cr3;

	popq %r15;
	popq %r14;
	popq %r13;
	popq %r12;
	popq %r11;
	popq %r10;
	popq %r9;
	popq %r8;

	//popq %rsp;
	popq %rbp;
	//popq %rdi;
	popq %rsi;
	popq %rdx;
	popq %rcx;
	popq %rbx;
	popq %rax;

	ret;

switch_out:
	movq 8(%rdi), %rsp; 		// update the rsp value with the next process stack
	popq %rdi; 					// update me to next task

    // restore the registers according to the new stack
	popfq; 						// pop the eflags/rflags
	//popq %rip; 					// pop the program counter

	popq %rax;
	movq %rax, %cr3;

	popq %r15;
	popq %r14;
	popq %r13;
	popq %r12;
	popq %r11;
	popq %r10;
	popq %r9;
	popq %r8;

	//popq %rsp;
	popq %rbp;
	//popq %rdi;
	popq %rsi;
	popq %rdx;
	popq %rcx;
	popq %rbx;
	popq %rax;

	retq;

switch_to_ring3:
	cli;
	mov $0x23, %ax;  
	mov %ax, %ds; 
	mov %ax, %es;  
	mov %ax, %fs;  
	mov %ax, %gs;
	//mov %ax, %ss;  
	              
	//mov %rsp, %rax;  
	pushq $0x23;  
	//pushq %rax; 
	pushq %rsi; 
	pushfq;  
	popq %rax;
	orq $0x200, %rax;
	push %rax;
	pushq $0x2B;  
	pushq %rdi;  
	iretq;  

rdmsr_read:
	mov %edi, %ecx;
	rdmsr;
	ret;

wrmsr_write: 
	mov %edi, %ecx;

	mov %esi, %eax;
	wrmsr;
	ret;

switch_to_child:
// pid - rdi
// new rsp - rsi
// parent pcb - rdx
// child pcb - rcx
//	pushq rax;

	mov %esp, %eax;
	and $0xfff, %eax;
	or %eax, %esi;

	//subq $8, %rsp;
	//subq $8, %rsi;

	movq %rdi, -8(%rsp);
	movq $0, -8(%rsi);

	//addq $8, %rsp;
	//addq $8, %rsi;

	movq %rsi, 8(%rcx);

	movq %rsp, 8(%rdx); 		// save my rsp value in pcb
	movq 8(%rcx), %rsp; 		// update the rsp value with the next process stack
	
//	popq %rax;
	ret;



