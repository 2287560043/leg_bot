#include "stm32h7xx.h"
#include <errno.h>
#include <stddef.h>

/* Newlib's floating-point printf needs a small C heap, separate from FreeRTOS.
 * Logging serializes formatting; RTOS tasks and queues still use static storage. */
void *_sbrk(ptrdiff_t increment)
{
    extern char _end, _heap_end;
    static char *current = &_end;
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    char *previous = current;
    if (increment > &_heap_end - current || increment < &_end - current) {
        __set_PRIMASK(primask);
        errno = ENOMEM;
        return (void *)-1;
    }
    current += increment;
    __set_PRIMASK(primask);
    return previous;
}
