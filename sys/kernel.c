#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/syscalls.h>
#include <sys/paging.h>
#include <sys/kernel.h>

void switch_to(pcb* , pcb*, volatile pcb**);

pcb pcb_struct[1024];
int free_pcb=0;
int no_of_task=0;
int current_process=0;

extern uint64_t pf_error_code, pf_cr2;

void clean_up(volatile pcb *last){
  uint64_t *proc_pml4 = (uint64_t *)get_va_add(last->cr3);
  for(int i=0;i<500;i++){
    
    if((proc_pml4[i] & 0x1) == 1){
      uint64_t *proc_pdp = (uint64_t *)get_va_add((uint64_t)proc_pml4[i] & 0xFFFFFFFFFFFFF000);
      
      for(int j=0; j<512 ; j++){
        if((proc_pdp[j] & 0x1) == 1){

          uint64_t *proc_pd = (uint64_t *)get_va_add((uint64_t)proc_pdp[j] & 0xFFFFFFFFFFFFF000);
          for(int k=0; k<512 ; k++){
            if((proc_pd[k] & 0x1) == 1){

              uint64_t *proc_pt = (uint64_t *)get_va_add((uint64_t)proc_pd[k] & 0xFFFFFFFFFFFFF000);
              for(int m=0; m<512 ; m++){
                if((proc_pt[m] & 0x1) == 1){
                  free((uint64_t *)proc_pt[m]);
                }
              }
              free((uint64_t *)proc_pd[k]);
            }
          }
          free((uint64_t *)proc_pdp[j]);
        }
      }
      free((uint64_t *)proc_pml4[i]);
    }
  }
  free((uint64_t *)last->cr3);

  //free kernel stack
  kfree(last->kstack);

  pcb *parent = &pcb_struct[last->ppid];

  if(parent->wait_for_any_proc == 1){
    parent->wait_for_any_proc = 0;
    parent->state = 0;
  }else if(parent->wait_child[last->pid] == 1){
    parent->wait_child[last->pid] = 0;
    int flag = 0;
    for(int i = 0; i<1024; i++){
      if(parent->wait_child[i] == 1){
        flag = 1;
        break;
      }
    }
    if(flag == 0){
      parent->state = 0;
    }
  }

  no_of_task--;
}

void yield(){
  //round robin scheduler
  pcb *me = &pcb_struct[current_process],*next = NULL;
  volatile pcb *last = NULL;
  if(no_of_task == 1){
  	return;
  }
  /*else if(current_process+1 == no_of_task){
    next = &pcb_struct[0];
    current_process = 0;
  }*/else{
  	int i;
    for(i=current_process+1; i<1024; i++){
      if(pcb_struct[i].state == 0){
      	next = &pcb_struct[i];
      	current_process = i;
      	break;
      }
    }
    if(i==1024){
      next = &pcb_struct[0];
      current_process = 0;
    }
  }  
  switch_to(me, next, &last);
  kprintf("last process pid %d\n", last->pid);
  if(last->state == 3){
    clean_up(last);
  }
  if(current_process != 0){
    save_rsp();
  }
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
  pcb_struct[free_pcb].pid = free_pcb;
  pcb_struct[free_pcb].ppid = current_process;
  pcb_struct[free_pcb].cr3 = (uint64_t)kernel_cr3;
  pcb_struct[free_pcb].state = 0;
  pcb_struct[free_pcb].exit_status = -1;
  pcb_struct[free_pcb].kstack = kmalloc(4096);
  pcb_struct[free_pcb].rsp = (uint64_t)(pcb_struct[free_pcb].kstack) + 0xF80;
  pcb_struct[free_pcb].mfdes[0].type = TERMINAL;
  pcb_struct[free_pcb].mfdes[0].status = 1;
  pcb_struct[free_pcb].mfdes[1].type = TERMINAL;
  pcb_struct[free_pcb].mfdes[1].status = 1;
  pcb_struct[free_pcb].mfdes[2].type = TERMINAL;
  pcb_struct[free_pcb].mfdes[2].status = 1;
  
  //kprintf("kstack of second thread kstack->%x rsp-> %x\n",pcb_struct[free_pcb].kstack,pcb_struct[free_pcb].rsp);

   
  //Initialize Thread 1
  // set structures of reg = 0;
  tmp = (uint64_t*) pcb_struct[free_pcb].rsp;
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
  volatile uint64_t last;
  *tmp-- = (uint64_t)&last;
  pcb_struct[free_pcb].rsp -= 24;    // finaly value of RSP
  
  //kprintf("rsp after i have Initialize process 2 -> %x\n",pcb_struct[free_pcb].rsp);

  // update the pcb structure
  free_pcb++;
  no_of_task++;
}

void create_pcb_stack(uint64_t *user_cr3,uint64_t va_func){
	//Kernel Thread 1 PCB
  pcb_struct[free_pcb].pid = free_pcb;
  pcb_struct[free_pcb].ppid = current_process;
  pcb_struct[free_pcb].cr3 = (uint64_t)user_cr3;
  pcb_struct[free_pcb].state = 0;
  pcb_struct[free_pcb].exit_status = -1;
  pcb_struct[free_pcb].kstack = kmalloc(4096);
  pcb_struct[free_pcb].rsp = (uint64_t)(pcb_struct[free_pcb].kstack) + 0xF80;
  pcb_struct[free_pcb].mfdes[0].type = TERMINAL;
  pcb_struct[free_pcb].mfdes[0].status = 1;
  pcb_struct[free_pcb].mfdes[1].type = TERMINAL;
  pcb_struct[free_pcb].mfdes[1].status = 1;
  pcb_struct[free_pcb].mfdes[2].type = TERMINAL;
  pcb_struct[free_pcb].mfdes[2].status = 1;
  //kprintf("kstack of second thread kstack->%x rsp-> %x\n",pcb_struct[free_pcb].kstack,pcb_struct[free_pcb].rsp);

   
  //Initialize Thread 1
  // set structures of reg = 0;
  uint64_t *tmp = (uint64_t*) pcb_struct[free_pcb].rsp;
  //push the start address of thread
  *tmp-- = (uint64_t) va_func;
  pcb_struct[free_pcb].rsp -= 8;
  for(int i = 14; i > 0; i--)
  {
  	*(tmp--) = 0;
  	pcb_struct[free_pcb].rsp -= 8;
  }

  //push cr3 onto the stack
  *(tmp--) = (uint64_t)user_cr3;

  uint64_t rflags = get_rflags_asm();
  *tmp-- = rflags;
  *tmp-- = (uint64_t)&pcb_struct[free_pcb];
  pcb_struct[free_pcb].rsp -= 16;		// finaly value of RSP
  
  //kprintf("rsp after i have Initialize process 2 -> %x\n",pcb_struct[free_pcb].rsp);

	// update the pcb structure
  free_pcb++;
  no_of_task++;
}

//TODO
//Parse complete VMA and properties to check if it it executable and we need to copy
//or it is dynamic memory allocation in that case just allocate a page and create mapping
void page_fault_handle(){
	kprintf("cr2 - %x, pf_error_code - %x\n", pf_cr2, pf_error_code);
	uint64_t *p_add = get_free_page();
	uint64_t *va_start = (uint64_t *)(pf_cr2 & 0xFFFFFFFFFFFFF000); 
  create_pf_pt_entry(p_add, (uint64_t)va_start);
  uint64_t *elf_start = (uint64_t *)pcb_struct[current_process].elf_start;
  for(int i=0;i<512;i++){
  	va_start[i] = elf_start[i];
  }
}