#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/paging.h>

uint64_t get_pml4(uint64_t kermem);
uint64_t get_pdp(uint64_t kermem);
uint64_t get_pd(uint64_t kermem);
uint64_t get_pt(uint64_t kermem);

uint32_t pml4_off;
uint32_t pdp_off;
uint32_t pd_off;
uint32_t pt_off;

uint64_t free_virtual_address;
page_frame_t *free_page,*table_end;
uint64_t page_count;

page_frame_t *head;
extern char kernmem, physbase;

void create4KbPages(uint32_t *modulep,void *physbase, void *physfree){
  struct smap_t {
    uint64_t base, length;
    uint32_t type;
  }__attribute__((packed)) *smap;

  //Store all the available memory boundaries in an array free memory boundaries
  uint64_t free_mem_boundaries[40];
  uint32_t no_bound = 0;
  while(modulep[0] != 0x9001) modulep += modulep[1]+2;
  for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
    if (smap->type == 1 /* memory */ && smap->length != 0) {
      free_mem_boundaries[no_bound++] = smap->base;  
      //kprintf("Free boundary entry %p\n", free_mem_boundaries[i-1]);
      free_mem_boundaries[no_bound++] = smap->base + smap->length;
      //kprintf("Free boundary entry%p\n", free_mem_boundaries[i-1]);
    }
  }
  head = (page_frame_t*)physfree;
  int j=0,pages_till_physfree = 0;
  page_count = 0;
  uint64_t mem_start = free_mem_boundaries[0];
  while(mem_start < free_mem_boundaries[no_bound-1]){
    page_frame_t *t = head + page_count;
    t->start = (uint64_t*) mem_start;
    t->info = (uint64_t) 0;
    page_count++;
    if(mem_start < (uint64_t)physfree){
      pages_till_physfree++;
    }
    mem_start += 4096;
  }

  //mark all pages till physfree + array size as used.
  int array_page = page_count * sizeof(page_frame_t) / 4096 + 1 + pages_till_physfree; // +1 since we have 
  //a division so it will be floored we need to take 1 as a buffer
  
  kprintf("pages_till_physfree -> %d, page_count -> %d\n",pages_till_physfree,page_count);

  //mark pages till the array end as used
  for(int i=0;i<array_page;i++){
    page_frame_t *t = head + i;
    t->info = 1 | (uint64_t)1 << 32;
  }

  //mark all the holes as 3
  j=1;
  mem_start = free_mem_boundaries[j++];
  while(j<no_bound-1){
    if(mem_start + 4096 < free_mem_boundaries[j]){
      page_frame_t *t = head + (mem_start/4096);
      t->info = (uint64_t) 3;
      mem_start += 4096;
    }else{
      j++;
      mem_start = free_mem_boundaries[j++];
    }
  }
  
  // The link list will start after the page descriptor have finished
  // first page
  page_frame_t *link_start = head + array_page;
  page_frame_t *t = link_start;
  free_page = link_start;
  table_end = free_page;
  t->prev = NULL;
  t->next = (page_frame_t*)(link_start + 1); 
  //kprintf("prev -> %x, next -> %x t-> %x  \n", t->prev,t->next, t);
 
  // last element
  t = head + page_count - 1;
  t->prev = (page_frame_t *)(head+page_count-2);
  t->next = NULL;
  
  //kprintf("prev -> %x, next -> %x t-> %x  \n", t->prev,t->next, t);
  
  for(int i = 1; i < page_count - 1 - array_page ;i++){
    t = link_start + i;
    t->prev = t-1;
    t->next = t+1;
  }
  /*for(int i= 0; i < 6;i++){
    t = link_start + i;
    kprintf("prev -> %x, next -> %x t-> %x\n", t->prev,t->next, t);
  }
  kprintf("no of page frames created %d\n", page_count);*/
  
  //kprintf("physbase -> %x, physfree -> %x, array_end -> %x, array_start -> %x\n", physbase, physfree, (free_page-1)->start, free_page->start);

  //uint64_t* page = get_free_page();
  //kprintf("get free page %x\n", page);
  //free(page);
  //page = get_free_page();
  //kprintf("get free page 2nd time %x\n", page);

  //kprintf("end of create4KbPages function");
  return;
}

//returns the first free page from the free_list
uint64_t* get_free_page(){
  page_frame_t *t = free_page;
  t->info = 1 | (uint64_t)1 << 32;
  free_page = t->next;
  return t->start;
}

//returns the first free page from the free_list
uint64_t* get_free_self__ref_page(){
  page_frame_t *t = free_page;
  t->info = 1 | (uint64_t)1 << 32;
  free_page = t->next;
  t->start[511] = (uint64_t)t->start | 0x3;
  return t->start;
}

//add the page to the required index in the array and set the links correctly
void free(uint64_t* address){
  int page_index = (uint64_t)address/4096;
  page_frame_t *t_start = head + page_index;
  int no_of_pages = t_start->info >> 32;
  page_frame_t *t_end = head + (page_index + no_of_pages -1);
  
  //linking all freed pages together
  page_frame_t *temp = t_start+1;
  for(int i = 1; i<no_of_pages;i++){
    temp->info = 0;
    temp->next = temp + 1;
    temp->prev = temp - 1;
    temp++;
  }

  t_start->info = 0;
  t_start->prev = NULL;
  t_start->next = t_start + 1;
  t_end->info = 0;
  t_end->next = NULL;

  if(address < free_page->start){
    t_end->next = free_page;
    free_page->prev = t_end;
    free_page = t_start;
  }else{
    for(int i=page_index-1;i>=0;i--){
      page_frame_t *des = head + i;
      if(des->info == 0){
        t_end->next = des->next;
        des->next->prev = t_end;
        des->next = t_start;
        t_start->prev = des;
        break;
      }
    }
  }
}

/*void kernel_init(){
  uint64_t *pml4 = get_free_page();
  uint64_t *pdp = get_free_page();
  uint64_t *pd = get_free_page();
  uint64_t *pt = get_free_page();
  for(int i=0; i < 1024; i++){
    pml4[i] = 0x00000002;
    pdp[i] = 0x00000002;
    pd[i] = 0x00000002;
    pt[i] = 0x00000002;
  } 
}*/

uint64_t* kernel_init(){
  // kernel memory address = 0xffff ffff 8020 0000
  // 63 - 48 = 0xffff
  // PML4 - 9 bits - 47 - 39 = 1111 1111 1    // binary   -> hex - 0x1ff
  // PDP  - 9 bits - 38 - 30 = 111 1111 10    // binary   -> hex - 0x1fe
  // PD   - 9 bits - 29 - 21 = 00 0000 001    // binary   -> hex - 0x001
  // PT   - 9 bits - 12 - 20 = 0 0000 0000    // binary   -> hex - 0x000
  // 12 bits - 0 - 11 = 0000 0000 0000 // binary   -> hex - 0x000
  page_dir kernel_page_info;

  kernel_page_info.pml4 = get_free_self__ref_page();
  kernel_page_info.pdp = kernel_page_info.pml4;
  kernel_page_info.pd = get_free_self__ref_page();
  kernel_page_info.pt = get_free_self__ref_page();

  //kprintf("array_start -> %x, physbase ->%x, kernmem ->%x\n",table_end->start, (uint64_t)&physbase, (uint64_t)&kernmem);
  uint64_t size = (uint64_t)(table_end->start) - (uint64_t)&physbase;

  //kprintf("size of the kernel %x, no of pages used %x\n", size, size/4096);
  //Make all the entries in the pages as writeable but set the pages as not used.
  for(int i=0; i < 511; i++){
    kernel_page_info.pml4[i] = 0x00000002;
    kernel_page_info.pdp[i] = 0x00000002;
    kernel_page_info.pd[i] = 0x00000002;
    kernel_page_info.pt[i] = 0x00000002;
  }

  pml4_off = get_pml4((uint64_t)&kernmem);
  pdp_off = get_pdp((uint64_t)&kernmem);
  pd_off = get_pd((uint64_t)&kernmem);
  pt_off = get_pt((uint64_t)&kernmem);
  
  // set pml4[511] = pdp | 0x1;
  /*if(pml4_off == 511)
    kernel_page_info.pdp = kernel_page_info.pml4;
  else
    kernel_page_info.pml4[pml4_off] = (uint64_t)kernel_page_info.pdp | 0x3;
*/
  //PDP setting
  // so PDP offset is 510 ~ 0x1fe, set others to zero
  /*if(pdp_off == 511)
    kernel_page_info.pd = kernel_page_info.pdp;
  else*/
  kernel_page_info.pdp[pdp_off] = (uint64_t)kernel_page_info.pd | 0x3;

  //PD setting
  // so PD offset is 1 ~ 0x1, set others to zero
  /*if(pd_off == 511)
    kernel_page_info.pt = kernel_page_info.pd;
  else*/
  kernel_page_info.pd[pd_off] = (uint64_t)kernel_page_info.pt | 0x3;

  size /= 4096;
  uint64_t curkermem = (uint64_t) &physbase;
  for(uint64_t j=0,i=pt_off;j<size;j++,i++){
    if(pt_off < 512)
    {
      /*if(j < 10)
        kprintf("j = %d, pt_off = %d, kmst = %x, kmend = %x\n", j, pt_off, curkermem, curkermem + 4096);*/
      kernel_page_info.pt[pt_off++] = (curkermem|0x3);
      curkermem += 4096;
    }
    else // pt table is full
    {
      uint64_t *page = get_free_self__ref_page();
      pt_off = 0;

      kprintf("Kernel PT table is full, creating new\n");
      kernel_page_info.pt = page;
      kernel_page_info.pt[pt_off++] = (curkermem|0x3);
      curkermem += 4096;

      //add a pd entry
      if(++pd_off < 512)
      {
        kernel_page_info.pd[pd_off] = ((uint64_t)kernel_page_info.pt|0x3);
      }
      else  // pd table is full
      {
        kprintf("Kernel PD table is full, creating new \n");
        pd_off = 0;
        page = get_free_self__ref_page();

        kernel_page_info.pd = page;
        kernel_page_info.pd[pd_off++] =  ((uint64_t)kernel_page_info.pt|0x3);

        //add a new pdp entry
        if(++pdp_off < 512)
        {
          kernel_page_info.pdp[pdp_off] = ((uint64_t)kernel_page_info.pd|0x3);
        }
        else  // pdp table is full
        {
          kprintf("Kernel PDP table is full, creating new\n");
          pdp_off = 0;
          page = get_free_self__ref_page();

          kernel_page_info.pdp = page;
          kernel_page_info.pdp[pdp_off++] = ((uint64_t) kernel_page_info.pd|0x3);

          //add a new pml4 entry
          kernel_page_info.pml4[pml4_off++] = ((uint64_t) kernel_page_info.pdp|0x3);
        }

      }
    }
    
  }
  
  //move video buffer 0xb8000 to 0xFFFFFFFF9000000
  uint64_t v_mem_address = 0xFFFFFFFF90000000;
  uint64_t v_mem_pd = get_pd(v_mem_address);
  uint64_t v_mem_pt = get_pt(v_mem_address);
  //kprintf("v_mem_pdp-> %x, v_mem_pd -> %x, v_mem_pt -> %x\n", v_mem_pdp, v_mem_pd, v_mem_pt);

  uint64_t *page = get_free_page();
  kernel_page_info.pt = page;
  kernel_page_info.pd[v_mem_pd] = (uint64_t)kernel_page_info.pt | 0x3;

  kernel_page_info.pt[v_mem_pt] = 0xb8000 | 0x3;


  return kernel_page_info.pml4;    // to be set to CR3 :)
}

void clear_page(uint64_t *page){
  for(int i=0; i < 512; i++){
    page[i] = 0x00000002;
  }
}

uint64_t get_pml4(uint64_t kermem)
{
  uint64_t mask = 0x1ff;//0x0000ff8000000000;
  uint64_t shift = 39;
  return ((kermem >> shift) & mask);
}

uint64_t get_pdp(uint64_t kermem)
{
  uint64_t mask = 0x1ff;//0x0000ff8000000000;
  uint64_t shift = 30;
  return ((kermem >> shift) & mask);
}

uint64_t get_pd(uint64_t kermem)
{
  uint64_t mask = 0x1ff;//0x0000ff8000000000;
  uint64_t shift = 21;
  return ((kermem >> shift) & mask);
}

uint64_t get_pt(uint64_t kermem)
{
  uint64_t mask = 0x1ff;//0x0000ff8000000000;
  uint64_t shift = 12;
  return ((kermem >> shift) & mask);
} 

void  update_global_pointers(){
  uint64_t va_temp = ((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)head);
  head = (page_frame_t *)va_temp;
  va_temp = ((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)free_page);
  free_page = (page_frame_t *)va_temp;
}