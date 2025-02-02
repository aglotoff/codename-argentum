// See the PrimeCell UART (PL011) Technical Reference Manual

#include <arch/arm/pl011.h>

// UART registers, divided by 4 for use as uint32_t[] indicies
#define UARTDR            (0x000 / 4)   // Data Register
#define UARTECR           (0x004 / 4)   // Error Clear Register
#define UARTFR            (0x018 / 4)   // Flag Register
#define   UARTFR_RXFE       (1U << 4)   //   Receive FIFO empty
#define   UARTFR_TXFF       (1U << 5)   //   Transmit FIFO full
#define UARTIBRD          (0x024 / 4)   // Integer Baud Rate Register
#define UARTFBRD          (0x028 / 4)   // Fractional Baud Rate Register
#define UARTLCR           (0x02C / 4)   // Line Control Register
#define   UARTLCR_FEN       (1U << 4)   //   Enable FIFOs
#define   UARTLCR_WLEN8     (3U << 5)   //   Word length = 8 bits
#define UARTCR            (0x030 / 4)   // Control Register
#define   UARTCR_UARTEN     (1U << 0)   //   UART Enable
#define   UARTCR_TXE        (1U << 8)   //   Transmit enable
#define   UARTCR_RXE        (1U << 9)   //   Receive enable
#define UARTIMSC          (0x038 / 4)   // Interrupt Mask Set/Clear Register
#define   UARTIMSC_RXIM     (1U << 4)   //   Receive interrupt mask

/**
 * Initialize the UART driver.
 * 
 * @param pl011 Pointer to the UART driver instance.
 * @param base Memory base address.
 * @param uart_clock Reference clock frequency.
 * @param baud_rate Required baud rate.
 */
int
pl011_init(struct Pl011 *pl011,
           void *base,
           unsigned long uart_clock,
           unsigned long baud_rate)
{
  pl011->base = (volatile uint32_t *) base;

  // Disable UART during initialization.
  pl011->base[UARTCR] &= ~UARTCR_UARTEN;

  // Set the baud rate.
  pl011->base[UARTIBRD] = (uart_clock / (16 * baud_rate)) & 0xFFFF;
  pl011->base[UARTFBRD] = ((uart_clock * 4 / baud_rate) >> 6) & 0x3F;

  // Enable FIFO, 8 data bits, one stop bit, parity off.
  pl011->base[UARTLCR] = UARTLCR_FEN | UARTLCR_WLEN8;

  // Clear any pending errors.
  pl011->base[UARTECR] = 0;

  // Enable UART, transfer & receive.
  pl011->base[UARTCR] = UARTCR_UARTEN | UARTCR_TXE | UARTCR_RXE;

  // Enable interupts.
  pl011->base[UARTIMSC] |= UARTIMSC_RXIM;

  return 0;
}

/**
 * Write a data character to the UART device.
 * 
 * @param pl011 Pointer to the UART driver instance.
 * @param data The character to be transmitted.
 */
static int 
pl011_write(void *ctx, int data)
{ 
  struct Pl011 *pl011 = (struct Pl011 *) ctx;

  // Wait until FIFO is ready to transmit.
  while (pl011->base[UARTFR] & UARTFR_TXFF)
    ;

  pl011->base[UARTDR] = data;

  return 0;
}

/**
 * Read a data character from the UART device.
 * 
 * @param pl011 Pointer to the UART driver instance.
 * @return The received data character or -1 if no data available.
 */
static int
pl011_read(void *ctx)
{ 
  struct Pl011 *pl011 = (struct Pl011 *) ctx;

  // Check whether the receive FIFO is empty.
  if (pl011->base[UARTFR] & UARTFR_RXFE)
    return -1;

  return pl011->base[UARTDR] & 0xFF;
}

struct UartOps pl011_ops = {
  .read = pl011_read,
  .write = pl011_write,
};
