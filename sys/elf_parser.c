#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/terminal.h>
#include <sys/syscalls.h>
#include <sys/utils.h>
#include <sys/elf64.h>

uint64_t va_start;
uint32_t elffd;
;

// file should already to opened
// pass/set the FD to read from that file
void elfheader()
{
	//__sysread(elffd, &elfhdr, sizeof(Elf64_Ehdr));

}


void parseelf(pcb *curpcb)
{
	// Not much check about supported files, since file is from tarfs, built by us.
	//read the header or direcly map the header as it is memory
	//Elf64_Ehdr *elfhdr = curpcb->fstartaddr;

	//uint8_t hdr[sizeof(Elf64_Ehdr)];







}



