#include "irq.h"
#include "x86.h"
#include "arch.h"
#include "printf.h"
#include "heap.h"

struct IDT {
	uint16_t off_low;
	uint16_t sel;
	uint8_t zero0;
	uint8_t type;
	uint16_t off_mid;
	uint32_t off_high;
	uint32_t zero1;
} __attribute__((packed));

typedef void (*trap_fn_t)(struct mcontext *);

extern void trap_unknown(struct mcontext *);
extern void trap_unknown_arg(struct mcontext *);
extern void trap_page_fault(struct mcontext *);
extern void trap_syscall(struct mcontext *);

syscall_handler_fn_t syscall_handler_fn;
page_fault_handler_fn_t page_fault_handler_fn;

static void default_syscall_handler(struct mcontext *ctx)
{
	printf("x86: unhandled syscall: rip=%p\n", ctx->rip);
	arch_abort();
}

static void default_page_fault_handler(struct mcontext *mcontext, uint64_t va)
{
	printf("x86: unhandled page-fault: code=%lx, rip=%p, va=%p\n", mcontext->extra, mcontext->rip, va);
	arch_abort();
}

static void set_idt(struct IDT *idt, trap_fn_t fn, int allow_user)
{
	idt->zero0 = 0;
	idt->zero1 = 0;

	idt->off_low = ((uint64_t)fn) & 0xffff;
	idt->off_mid = (((uint64_t)fn) >> 16) & 0xffff;
	idt->off_high = (((uint64_t)fn) >> 32);

	idt->sel = 0x8;
	idt->type = 0x8e | (allow_user ? 0x60 : 0);
}

void irq_init()
{
	int i;
	
	struct {
		uint16_t limit;
		uint64_t base;
	} __attribute__((packed)) IDTR;

	IDTR.limit = sizeof(struct IDT) * 0x100;
	IDTR.base = (uint64_t)heap_alloc_page();

	struct IDT *idt = (struct IDT *)IDTR.base;

	// Initialise the table with unknowns
	for (i = 0; i < 0x100; i++) {
		set_idt(&idt[i], trap_unknown, 0);
	}

	set_idt(&idt[0x08], trap_unknown_arg, 0);
	set_idt(&idt[0x08], trap_unknown_arg, 0);
	set_idt(&idt[0x0a], trap_unknown_arg, 0);
	set_idt(&idt[0x0b], trap_unknown_arg, 0);
	set_idt(&idt[0x0c], trap_unknown_arg, 0);
	set_idt(&idt[0x0d], trap_unknown_arg, 0);
	set_idt(&idt[0x0e], trap_page_fault, 0);
	set_idt(&idt[0x11], trap_unknown_arg, 0);
	set_idt(&idt[0x1e], trap_unknown_arg, 0);
	set_idt(&idt[0x80], trap_syscall, 1);

	asm volatile("lidt %0\n" :: "m"(IDTR));

	irq_reset_syscall_handler();
	irq_reset_page_fault_handler();
}

void irq_install_syscall_handler(syscall_handler_fn_t handler_fn)
{
	syscall_handler_fn = handler_fn;
}

void irq_install_page_fault_handler(page_fault_handler_fn_t handler_fn)
{
	page_fault_handler_fn = handler_fn;
}

void irq_reset_syscall_handler()
{
	syscall_handler_fn = default_syscall_handler;
}

void irq_reset_page_fault_handler()
{
	page_fault_handler_fn = default_page_fault_handler;
}

void handle_trap_unknown(struct mcontext *mcontext)
{
	printf("unknown exception from rip=%lx\n", mcontext->rip);
	arch_abort();
}

void handle_trap_unknown_arg(struct mcontext *mcontext, uint64_t val)
{
	printf("unknown exception from rip=%lx, val=%lx\n", mcontext->rip, val);
	arch_abort();
}
