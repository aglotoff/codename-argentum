#include <string.h>

#include <kernel/core/irq.h>
#include <kernel/core/tick.h>

#include <kernel/vm.h>
#include <kernel/page.h>
#include <kernel/interrupt.h>
#include <kernel/console.h>
#include <kernel/tty.h>
#include <kernel/trap.h>

#include <arch/i386/ide.h>

void main(void);
void mp_main(void);

void
arch_init(void)
{
	page_init_low();
	arch_vm_init();
	page_init_high();

  arch_interrupt_init();

  main();
}

void
arch_init_devices(void)
{
  interrupt_attach(0, timer_irq, NULL);
  ide_init();
}

void
arch_mp_init(void)
{
  // TODO
}

void
arch_eth_write(const void *buf, size_t n)
{
  // TODO
  (void) buf;
  (void) n;
}