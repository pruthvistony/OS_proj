#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/paging.h>
#include <sys/gdt.h>

extern uint64_t *kernel_cr3;

void kernel_1_thread();
void kernel_2_thread();
void yeild();
void switch_out(pcb*);
void switch_to(pcb* , pcb*);
void switch_to_ring3(uint64_t *, uint64_t);
void user_process_init(uint64_t *func_add, uint32_t no_of_pages);
void create_user_ring3_kernel_thread(uint64_t* func_ptr);
void user_process_1();

pcb pcb_entries[1024];
uint64_t thread_st[512];
int free_pcb=0;
int no_of_task=0;
int current_process=0;

void push_asm()
{
	//__asm__{
	//	""
	//}
}

uint64_t get_rflags_asm()
{
	uint64_t rflags;
	__asm__ __volatile__(
		"PUSHFQ \n\t"
		"POPQ %%rax\n\r"
		"MOVQ %%rax, %0"
		:"=m"(rflags)
		:
		:"%rax");
	return rflags;
}

void create_kernel_thread(uint64_t* func_ptr){
  uint64_t *tmp;

  //Kernel Thread PCB
  pcb_entries[free_pcb].pid = free_pcb;
  pcb_entries[free_pcb].cr3 = (uint64_t)kernel_cr3;
  pcb_entries[free_pcb].state = 0;
  pcb_entries[free_pcb].exit_status = -1;
  pcb_entries[free_pcb].kstack = kmalloc(4096, NULL);
  pcb_entries[free_pcb].rsp = (uint64_t)(pcb_entries[free_pcb].kstack) + 0xF80;

  //kprintf("kstack of second thread kstack->%x rsp-> %x\n",pcb_entries[free_pcb].kstack,pcb_entries[free_pcb].rsp);

   
  //Initialize Thread 1
  // set structures of reg = 0;
  tmp = (uint64_t*) pcb_entries[free_pcb].rsp;
  //push the start address of thread
  *tmp-- = (uint64_t) func_ptr;
  pcb_entries[free_pcb].rsp -= 8;
  for(int i = 14; i > 0; i--)
  {
    *(tmp--) = 0;
    pcb_entries[free_pcb].rsp -= 8;
  }

  //push cr3 onto the stack
  *(tmp--) = (uint64_t)kernel_cr3;

  uint64_t rflags = get_rflags_asm();
  *tmp-- = rflags;
  *tmp-- = (uint64_t)&pcb_entries[free_pcb];
  pcb_entries[free_pcb].rsp -= 16;    // finaly value of RSP
  
  //kprintf("rsp after i have Initialize process 2 -> %x\n",pcb_entries[free_pcb].rsp);

  // update the pcb structure
  free_pcb++;
  no_of_task++;
}

void wrmsr(uint32_t msrid, uint64_t msr_value){
  __asm__ __volatile__ ("wrmsr": : "c" (msrid), "A" (msr_value));
}

uint64_t rdmsr(uint32_t msrid){
  uint64_t msr_value;
  __asm__ __volatile__ ("rdmsr": "=A" (msr_value) : "c" (msrid));
  return msr_value;
}

void init_syscalls(){
  wrmsr(0xC0000082, 0xFFFFFF0000082);
  uint64_t star = rdmsr(0xC0000081);
  uint64_t lstar = rdmsr(0xC0000082);
  uint64_t cstar = rdmsr(0xC0000083);
  uint64_t sfmask = rdmsr(0xC0000084);
  kprintf("star -> %x, lstar -> %x, cstar -> %x, sfmask -> %x\n", star, lstar, cstar, sfmask);
}

void main_task(){

  init_syscalls();
	//setting main task PCB
  /*pcb_entries[free_pcb].pid = free_pcb;
  pcb_entries[free_pcb].cr3 = (uint64_t)kernel_cr3;
  pcb_entries[free_pcb].state = 0;
  pcb_entries[free_pcb].exit_status = -1;
  free_pcb++;
  no_of_task++;*/
  
  create_kernel_thread((uint64_t *)&kernel_1_thread);
  create_kernel_thread((uint64_t *)&kernel_2_thread);

  //int j=0;
	//while(j<2){
    kprintf("This is the main_task of kernel thread\n");
    //j++;
    switch_out(&pcb_entries[current_process]);
    kprintf("returning to main thread\n");
	//}  

	return;
}

void create_pcb_stack(uint64_t *user_cr3,uint64_t va_func){
	//Kernel Thread 1 PCB
  pcb_entries[free_pcb].pid = free_pcb;
  pcb_entries[free_pcb].cr3 = (uint64_t)user_cr3;
  pcb_entries[free_pcb].state = 0;
  pcb_entries[free_pcb].exit_status = -1;
  pcb_entries[free_pcb].kstack = kmalloc(4096, NULL);
  pcb_entries[free_pcb].rsp = (uint64_t)(pcb_entries[free_pcb].kstack) + 0xF80;

  //kprintf("kstack of second thread kstack->%x rsp-> %x\n",pcb_entries[free_pcb].kstack,pcb_entries[free_pcb].rsp);

   
  //Initialize Thread 1
  // set structures of reg = 0;
  uint64_t *tmp = (uint64_t*) pcb_entries[free_pcb].rsp;
  //push the start address of thread
  *tmp-- = (uint64_t) va_func;
  pcb_entries[free_pcb].rsp -= 8;
  for(int i = 14; i > 0; i--)
  {
  	*(tmp--) = 0;
  	pcb_entries[free_pcb].rsp -= 8;
  }

  //push cr3 onto the stack
  *(tmp--) = (uint64_t)user_cr3;

  uint64_t rflags = get_rflags_asm();
  *tmp-- = rflags;
  *tmp-- = (uint64_t)&pcb_entries[free_pcb];
  pcb_entries[free_pcb].rsp -= 16;		// finaly value of RSP
  
  //kprintf("rsp after i have Initialize process 2 -> %x\n",pcb_entries[free_pcb].rsp);

	// update the pcb structure
  free_pcb++;
  no_of_task++;
}

void map_user_process_init(uint64_t *func_add, uint32_t no_of_pages){
	uint64_t pa_func = ((uint64_t)func_add - (uint64_t)&kernmem + (uint64_t)&physbase);
	pa_func = pa_func & (uint64_t)0xFFFFFFFFFFFFF000;
	uint64_t va_func = 0xFFFFFEFF20000000;
  pa_func-=4096;
	uint64_t* user_cr3 = create_user_page_table(va_func,pa_func,3);
	va_func = va_func | ((uint64_t)func_add & (uint64_t)0xfff);
  va_func+=4096;
	create_pcb_stack(user_cr3,va_func);
}

void user_process_1(){
  //int j = 0;
  while(1){
    kprintf("This is user process 1\n");
    yeild();
    kprintf("returning to user process 1\n");
  }
}

void user_ring3_process() {
  kprintf("This is ring 3 user process 1\n");
  while(1){};
  //yeild();
  /*while(1) {
    kprintf("This is ring 3 user process 1\n");
    yeild();
    kprintf("returning to ring 3 user process 1\n");
  }*/
}

void save_rsp(){
  uint64_t rsp;
  __asm__ __volatile__ ("movq %%rsp, %0" : "=m"(rsp) : : );
  set_tss_rsp((void *)rsp);
}

void kernel_1_thread(){
  int j = 0;

  //kprintf("This is the first kernel thread\n");

  //user ring3 process
  //create_user_ring3_kernel_thread((uint64_t*) &user_ring3_process);

  while(j<2){
    j++;
    kprintf("This is the first kernel thread\n");
    //user process init
    //uint64_t func_ptr = (uint64_t)&user_process_1;
    //map_user_process_init((uint64_t*)func_ptr,1);

    yeild();
    kprintf("returning to kernel_1_thread\n");
  }
  uint64_t stack = (uint64_t)kmalloc(4096,NULL);
  stack+= 4088;
  save_rsp();
  switch_to_ring3((uint64_t *)&user_ring3_process, stack);
  while(1){};
}

void kernel_2_thread(){
  int j = 0;
  while(j<2){
    j++;
    kprintf("This is the second kernel thread\n");
    //user process init
    /*uint64_t func_ptr = (uint64_t)&user_process_1;
    user_process_init((uint64_t*)func_ptr,1);*/
    yeild();
    kprintf("returning to kernel_2_thread\n");
  }
  while(1){};
}

void yeild(){
  //round robin scheduler
  pcb *me = &pcb_entries[current_process],*next = NULL;
  if(no_of_task == 1){
  	return;
  }
  else if(current_process+1 == no_of_task){
    next = &pcb_entries[0];
    current_process = 0;
  }else{
  	int i;
    for(i=current_process+1; i<1024; i++){
      if(pcb_entries[i].state == 0){
      	next = &pcb_entries[i];
      	current_process = i;
      	break;
      }
    }
    if(i==1024){
      next = &pcb_entries[0];
      current_process = 0;
    }
  }

  //kprintf("next.rsp -> %x\n",next->rsp, me->rsp);
  /*if(current_process == 2)    // hack to check the user ring 3 process
    switch_to_ring3(me, next);
  else*/
    switch_to(me, next);
}


/*void create_user_ring3_kernel_thread(uint64_t* func_ptr){
  uint64_t *tmp;

  //Kernel Thread PCB
  pcb_entries[free_pcb].pid = free_pcb;
  pcb_entries[free_pcb].cr3 = (uint64_t)kernel_cr3;
  pcb_entries[free_pcb].state = 0;
  pcb_entries[free_pcb].exit_status = -1;
  pcb_entries[free_pcb].kstack = kmalloc(4096, NULL);
  pcb_entries[free_pcb].rsp = (uint64_t)(pcb_entries[free_pcb].kstack) + 0xF80;
   
  //Initialize Thread 1
  // set structures of reg = 0;
  // TODO : need more elements to push to stack for ring3
  tmp = (uint64_t*) pcb_entries[free_pcb].rsp;

#if 1
  // put the magic number of ring3 switching
  *tmp-- = 0x23;
  *tmp-- = pcb_entries[free_pcb].rsp; //eax ... just pushing rsp for the heck of it
  uint64_t fflags = get_rflags_asm();
  *tmp-- = fflags;
  *tmp-- = 0x23;
  pcb_entries[free_pcb].rsp -= 32;
#endif

  //push the start address of thread
  *tmp-- = (uint64_t) func_ptr;
  pcb_entries[free_pcb].rsp -= 8;

  for(int i = 14; i > 0; i--)
  {
    *(tmp--) = 0;
    pcb_entries[free_pcb].rsp -= 8;
  }

  //push cr3 onto the stack
  *(tmp--) = (uint64_t)kernel_cr3;

  uint64_t rflags = get_rflags_asm();
  *tmp-- = rflags;
  *tmp-- = (uint64_t)&pcb_entries[free_pcb];
  pcb_entries[free_pcb].rsp -= 16;    // finaly value of RSP
 
  // update the pcb structure
  free_pcb++;
  no_of_task++;
}*/
