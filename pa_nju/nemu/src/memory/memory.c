#include "nemu.h"
#include "cpu/cpu.h"
#include "memory/memory.h"
#include "device/mm_io.h"
#include <memory.h>
#include <stdio.h>

uint8_t hw_mem[MEM_SIZE_B];

uint32_t hw_mem_read(paddr_t paddr, size_t len)
{
	uint32_t ret = 0;
	memcpy(&ret, hw_mem + paddr, len);
	return ret;
}

void hw_mem_write(paddr_t paddr, size_t len, uint32_t data)
{
	memcpy(hw_mem + paddr, &data, len);
}

uint32_t paddr_read(paddr_t paddr, size_t len)
{
#ifdef HAS_DEVICE_VGA
    if(is_mmio(paddr)!=-1)return mmio_read(paddr,len,is_mmio(paddr));
#endif
	uint32_t ret = 0;
#ifdef CACHE_ENABLED
		ret = cache_read(paddr, len);
#else
		ret = hw_mem_read(paddr, len);
#endif
	return ret;
}

void paddr_write(paddr_t paddr, size_t len, uint32_t data)
{
#ifdef HAS_DEVICE_VGA
    if(is_mmio(paddr)!=-1)
    {
        mmio_write(paddr,len,data,is_mmio(paddr));
        return;
    }
#endif
#ifdef CACHE_ENABLED
		cache_write(paddr, len, data);
#else
		hw_mem_write(paddr, len, data);
#endif
}

uint32_t laddr_read(laddr_t laddr, size_t len)
{
    if(cpu.cr0.pg)
    {
        if((laddr&0xfff)+len<=0x1000)return paddr_read(page_translate(laddr), len);
	    size_t len1=0x1000-(laddr&0xfff);
	    return paddr_read(page_translate(laddr),len1)+(paddr_read(page_translate(laddr+len1),len-len1)<<(len1*8));
    }
    return paddr_read(laddr,len);
}

void laddr_write(laddr_t laddr, size_t len, uint32_t data)
{
    if(cpu.cr0.pg)
    {
        if((laddr&0xfff)+len<=0x1000){paddr_write(page_translate(laddr), len,data);return;}
	    size_t len1=0x1000-(laddr&0xfff);
	    paddr_write(page_translate(laddr),len1,data&((1<<(len1*8))-1));
	    paddr_write(page_translate(laddr+len1),len-len1,data>>(len1*8));
	    return;
    }
    paddr_write(laddr,len,data);
}

uint32_t vaddr_read(vaddr_t vaddr, uint8_t sreg, size_t len)
{
	assert(len == 1 || len == 2 || len == 4);
	if(cpu.cr0.pe)vaddr=segment_translate(vaddr, sreg);
	return laddr_read(vaddr, len);
}

void vaddr_write(vaddr_t vaddr, uint8_t sreg, size_t len, uint32_t data)
{
	assert(len == 1 || len == 2 || len == 4);
	if(cpu.cr0.pe)vaddr=segment_translate(vaddr, sreg);
	laddr_write(vaddr, len, data);
}

void init_mem()
{
	// clear the memory on initiation
	memset(hw_mem, 0, MEM_SIZE_B);
	
#ifdef CACHE_ENABLED
	init_cache();
#endif

#ifdef TLB_ENABLED
	make_all_tlb();
	init_all_tlb();
#endif
}

uint32_t instr_fetch(vaddr_t vaddr, size_t len)
{
	assert(len == 1 || len == 2 || len == 4);
	return vaddr_read(vaddr, SREG_CS, len);
}

uint8_t *get_mem_addr()
{
	return hw_mem;
}
