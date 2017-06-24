# AVR Optimized Ring Buffer

- up to 255 stored elements (BUFFER_SIZE - 1)
- provides underrun and overrun checks in insert/remove/load functions
- only 2 bytes memory footprint except actuall buffer
- atomic operation for SPSC cases without disabling interrupts
- buffer size implemented as a compile time constant to make use of AVR immediate instructions
- currently implemented as a direct replacement for LUFA ring buff with one additional function (RingBuffer_Load()).

```
uint8_t tmp;

if(RingBuffer_Insert(&buff, tmp))
	//element inserted
else
	//buffer full
	
tmp = RingBuffer_Remove(&buff);

if(RingBuffer_Load(&buff, &tmp)) // function is inlined so tmp never gets onto stack
	// something was read
else
	// buffer empty
```

## notes

The actual code might not be the most efficient in interrupt handlers since one more register have to be preserved for storing temporary index. 
Further optimizations are possible but but gcc is known to generate inefficient code especially when larger elements are used or "just because".
It is possible to reimplement in assembly:
- Insert function [using only 3 registers + SREG](https://github.com/jnk0le/Easy-AVR-USART-C-Library/blob/master/usart.c#L4871)
- Remove/Load functions [using only 2 registers + SREG](https://github.com/jnk0le/Easy-AVR-USART-C-Library/blob/master/usart.c#L4702)

## todo
- compare sizes