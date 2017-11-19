#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/paging.h>
#include <sys/gdt.h>
#include <sys/tarfs.h>
#include <sys/syscalls.h>

void kernel_1_thread();
void kernel_2_thread();
void switch_out(pcb*);
void switch_to_ring3(uint64_t *, uint64_t);
void user_process_init(uint64_t *func_add, uint32_t no_of_pages);
void create_user_ring3_kernel_thread(uint64_t* func_ptr);
void user_process_1();

void rdmsr_read(uint32_t);
void wrmsr_write(uint32_t, uint32_t, uint32_t);

void main_task(){

  init_syscalls();
	//setting main task PCB
  /*pcb_struct[free_pcb].pid = free_pcb;
  pcb_struct[free_pcb].cr3 = (uint64_t)kernel_cr3;
  pcb_struct[free_pcb].state = 0;
  pcb_struct[free_pcb].exit_status = -1;
  free_pcb++;
  no_of_task++;*/
  
  create_kernel_thread((uint64_t *)&kernel_1_thread);
  create_kernel_thread((uint64_t *)&kernel_2_thread);

  //int j=0;
	//while(j<2){
    kprintf("This is the main_task of kernel thread\n");
    //j++;
    switch_out(&pcb_struct[current_process]);
    kprintf("returning to main thread\n");
	//}  

	return;
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
    yield();
    kprintf("returning to user process 1\n");
  }
}

void user_ring3_process() {
  kprintf("This is ring 3 user process 1\n");
  /*uint64_t __err;
  __asm__ __volatile__ ("movq $0, %%rdi\n\t"
                        "movq $0, %%rsi\n\t"
                        "movq $0, %%rax\n\t"
                        "syscall\n\t"
                        :"=a"(__err)
                        :"0" (57));*/
  uint8_t buf[] = "hello\nsecond\nthird";
  uint8_t *buf1 = buf;
  uint8_t a =0,b=10;
  int c=a+b;
  kprintf("buf add -> %p & %p %d %d \n", buf1, &buf1, b, c);

  __asm__ __volatile__ ("movq $1, %%rax\n\t"
                        "movq $1, %%rdi\n\t"
                        "movq %0, %%rsi\n\t"
                        "movq $18, %%rdx\n\t"
                        "movq $5, %%r10\n\t"
                        "syscall\n\t"
                        :
                        :"m"(buf1)
                        :"rsp","rcx","r11","rdi","rsi","rdx","r10");

  kprintf("buf1 add after return- %d %p & %p %d \n",c, buf1, &buf1, b);
  kprintf(" bufjpruthvi %s\n", buf1);
  uint8_t buf2[256];
  uint8_t *buf3 = buf2;
  for(int i = 0;i<10;i++){
    __asm__ __volatile__ ("movq $0, %%rax\n\t"
                          "movq $0, %%rdi\n\t"
                          "movq %0, %%rsi\n\t"
                          "movq $128, %%rdx\n\t"
                          "movq $5, %%r10\n\t"
                          "syscall\n\t"
                          :
                          :"m"(buf3)
                          :"rsp","rcx","r11","rdi","rsi","rdx","r10");
    __asm__ __volatile__ ("movq $1, %%rax\n\t"
                          "movq $1, %%rdi\n\t"
                          "movq %0, %%rsi\n\t"
                          "movq $18, %%rdx\n\t"
                          "movq $5, %%r10\n\t"
                          "syscall\n\t"
                          :
                          :"m"(buf3)
                          :"rsp","rcx","r11","rdi","rsi","rdx","r10");
    kprintf("returned from write\n");
  }
  //kprintf("returned from syscall\n");
  while(1){};
  //yield();
  /*while(1) {
    kprintf("This is ring 3 user process 1\n");
    yield();
    kprintf("returning to ring 3 user process 1\n");
  }*/
}

void save_rsp(){
  uint64_t rsp;
  __asm__ __volatile__ ("movq %%rsp, %0" : "=m"(rsp) : : );
  set_tss_rsp((void *)rsp);
}

uint64_t power(uint64_t num, uint64_t pow){
  uint64_t result = 1;
  for(int i=0; i<pow; i++){
    result *= num;
  }
  return result;
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

    yield();
    kprintf("returning to kernel_1_thread\n");
  }
  init_tarfs();



  //switching to ring 3
  uint64_t stack = (uint64_t)kmalloc(4096,NULL);
  stack+= 4088;
  save_rsp();
  switch_to_ring3((uint64_t *)&user_ring3_process, stack);

  while(1){};
}

void kernel_2_thread(){
  while(1){
    kprintf("This is the second kernel thread\n");
    //user process init
    /*uint64_t func_ptr = (uint64_t)&user_process_1;
    user_process_init((uint64_t*)func_ptr,1);*/
    yield();
    kprintf("returning to kernel_2_thread\n");
  }
  while(1){};
}

/*void create_user_ring3_kernel_thread(uint64_t* func_ptr){
  uint64_t *tmp;

  //Kernel Thread PCB
  pcb_struct[free_pcb].pid = free_pcb;
  pcb_struct[free_pcb].cr3 = (uint64_t)kernel_cr3;
  pcb_struct[free_pcb].state = 0;
  pcb_struct[free_pcb].exit_status = -1;
  pcb_struct[free_pcb].kstack = kmalloc(4096, NULL);
  pcb_struct[free_pcb].rsp = (uint64_t)(pcb_struct[free_pcb].kstack) + 0xF80;
   
  //Initialize Thread 1
  // set structures of reg = 0;
  // TODO : need more elements to push to stack for ring3
  tmp = (uint64_t*) pcb_struct[free_pcb].rsp;

#if 1
  // put the magic number of ring3 switching
  *tmp-- = 0x23;
  *tmp-- = pcb_struct[free_pcb].rsp; //eax ... just pushing rsp for the heck of it
  uint64_t fflags = get_rflags_asm();
  *tmp-- = fflags;
  *tmp-- = 0x23;
  pcb_struct[free_pcb].rsp -= 32;
#endif

  //push the start address of thread
  *tmp-- = (uint64_t) func_ptr;
  pcb_struct[free_pcb].rsp -= 8;

  for(int i = 14; i > 0; i--)
  {
    *(tmp--) = 0;
    pcb_struct[free_pcb].rsp -= 8;
  }

  //push cr3 onto the stack
  *(tmp--) = (uint64_t)kernel_cr3;

  uint64_t rflags = get_rflags_asm();
  *tmp-- = rflags;
  *tmp-- = (uint64_t)&pcb_struct[free_pcb];
  pcb_struct[free_pcb].rsp -= 16;    // finaly value of RSP
 
  // update the pcb structure
  free_pcb++;
  no_of_task++;
}*/
