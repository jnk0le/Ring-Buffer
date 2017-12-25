# Template Ring Buffer

- C++11 and above
- no exceptions/RTTI
- designed for compile time (static) allocation and type evaluation
- lock-free atomic operation in SPSC cases
- underrun and overrun checks in insert/remove functions
- highly efficient on most microcontroller architectures (eg. avr, cortex-m)

## available implementations


### "Always Keep One Slot Open"

- implemented in `Ringbuffer` class
- n-1 available slots (`head == tail` empty state)

### Counting total elements written/read (aka unmasked indices)

- implemented in `Ringbuffer_unmasked` class
- whole array is used
- masking is performed when writing/reading array
- insert operation might be a little slower due to comparison with (usually) large immediates.

```
// gcc 6.2 breaks it even more by doing weird unnecessary operations
// on avr it gets even worser
          unmasked_insert:
08000268:   ldr     r2, [pc, #28]
0800026a:   ldr     r3, [r2, #0]
0800026c:   ldr     r1, [r2, #0] <--- ???
0800026e:   ldr     r0, [r2, #4]
08000270:   subs    r1, r1, r0 <-- ?? subs r1, r3, r0
08000272:   cmp     r1, #8
08000274:   beq.n   0x8000284
08000276:   adds    r1, r3, #1
08000278:   and.w   r3, r3, #7
0800027c:   add     r3, r2
0800027e:   movs    r0, #2
08000280:   strb    r0, [r3, #8]
08000282:   str     r1, [r2, #0]
08000284:   bx      lr
          masked_insert:
0800028c:   ldr     r2, [pc, #24]
0800028e:   ldr     r3, [r2, #0]
08000290:   ldr     r1, [r2, #4]
08000292:   adds    r3, #1
08000294:   and.w   r3, r3, #7
08000298:   cmp     r3, r1
0800029a:   beq.n   0x80002a4
0800029c:   adds    r1, r2, r3
0800029e:   movs    r0, #1
080002a0:   strb    r0, [r1, #8]
080002a2:   str     r3, [r2, #0]
080002a4:   bx      lr
```

## notes

- if ring buffer is allocated on the stack (local scope) or heap, it have to be explicitly cleared before use or be createt using value initializing constructor

```
Ringbuffer<uint8_t, 256> a; // global objects can use empty constructor // it is zero initialized through bss section

int main()
{
	Ringbuffer<uint8_t, 512> b(0); // stack objects have undefined initial values so explicitly initialize head and tail to zeroth position
	static Ringbuffer<uint16_t, 1024> c; // static objects can use empty constructor // it is zero initialized through bss section
	
	...
}
```

- index_t of size less than architecture reg size (size_t) might not be most efficient (arm gcc can still generate `uxth/uxtb` when not necessary)

## example

```
Ringbuffer<const char*, 256> message;

int main()
{
	//...
	while(1)
	{
		const char* tmp;
		while(!message.remove(tmp));
		printf("%s fired\n", tmp);
		//...
	}
}

// if multiple contexts are writing/reading buffer they shall not be interrupting each other 
// in this case, those interrupts have to be of the same priority (nesting not allowed) 
	
extern "C" void SysTick_Handler(void)
{
	message.insert("SysTick_Handler");
}

extern "C" void USART2_IRQHandler(void)
{
	message.insert("USART2_IRQHandler");
}
```

## todo:
- block write/read
- insert overwrite
- pick appropriate namespace that will not end in "using namespace"
- memory barriers for non gcc compilers
- multi core ??