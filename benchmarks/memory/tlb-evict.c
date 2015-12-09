#include "benchmark.h"
#include "mem.h"

#define MEMORY_BENCHMARK_ITERATIONS 2000
#define VIRT_BASE 0x40000000

asm (".align 12\n");

static size_t virt_start, virt_end;

static void kernel_mmu_init()
{
	mem_init();

	// Map physical memory into a virtual memory region
	const phys_mem_info_t *phys_mem = mem_get_phys_info();
	virt_start = 0x40000000;
	
	uintptr_t ptr = phys_mem->phys_mem_start;
	uintptr_t vptr = virt_start;
	size_t pagesize = mem_get_page_size();
	while(ptr < phys_mem->phys_mem_end) {
		mem_create_page_mapping(ptr, vptr);
		ptr += pagesize;
		vptr += pagesize;
	}
	
	virt_end = vptr;
	
	mem_mmu_enable();
	mem_tlb_flush();
}
static void kernel_mmu()
{
	uint32_t total_iterations = BENCHMARK_ITERATIONS * MEMORY_BENCHMARK_ITERATIONS;
	uint32_t i;

	uintptr_t ptr = virt_start;
	size_t pagesize = mem_get_page_size();
	
	for(i = 0; i < total_iterations; ++i) {
		*(uint32_t*)ptr = 0x12345678;
		ptr += pagesize;
		if(ptr >= virt_end) ptr = virt_start;
		mem_tlb_evict(ptr);
	}
}
static void kernel_mmu_cleanup()
{
	mem_mmu_disable();
	mem_tlb_flush();
	mem_reset();
}

static benchmark_t bmark = {
	.name="TLB-Evict",
	.category="Memory",
	.kernel_init=kernel_mmu_init,
	.kernel=kernel_mmu,
	.kernel_cleanup=kernel_mmu_cleanup
};

REG_BENCHMARK(bmark);
