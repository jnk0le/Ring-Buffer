# Template Ring Buffer

- C++11 and above
- no exceptions, RTTI, virtual functions and dynamic memory allocation
- designed for compile time (static) allocation and type evaluation
- lock-free atomic operation in SPSC cases
- underrun and overrun checks in insert/remove functions
- highly efficient on most microcontroller architectures (eg. avr, cortex-m)

## available implementations

### "Always Keep One Slot Open"

- implemented in `Ringbuffer` class
- n-1 available slots (`head == tail` empty state)
- In this implementation indices are incremented before accessing arrays, to reduce register pressure and code size relative to "normal" implementations.
(index == 4 actually points to the element 5)

### Counting total elements written/read (aka unmasked/absolute indices)

- implemented in `Ringbuffer_unmasked` class
- whole array is used, no wasted slots
- masking is performed when writing/reading array
- insert operation might be a little slower. (some architectures also won't handle comparison with large immediates as kindly as thumb-2).

## notes

- If ring buffer is allocated on the stack (local scope) or heap, it have to be explicitly cleared before use or be created using value initializing constructor

```
Ringbuffer<uint8_t, 256> a; // global objects can use empty constructor // it is zero initialized through bss section

int main()
{
	Ringbuffer<uint8_t, 512> b(0); // stack objects have undefined initial values so explicitly initialize head and tail to zeroth position
	static Ringbuffer<uint16_t, 1024> c; // static objects can use empty constructor // it is zero initialized through bss section
	
	//...
}
```

- On cortex-m and similiar architectures, larger buffer sizes will generate larger instructions (execution might be slower due to waitstates or additional necessary instructions)
- index_t of size less than architecture reg size (size_t) might not be most efficient (arm gcc (6.2) will generate redundant `uxth/uxtb`)
- 8 and odd (53, 48, etc) bit architectures are not supported in master branch at the moment. Broken code is likely to be generated.

## example

```
Ringbuffer<const char*, 256> message;

int main()
{
	//...
	while(1)
	{
		const char* tmp = nullptr;
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
- consumer/producer flush
- document api
- pick appropriate namespace that will not end in "using namespace"
- multi core // weak memory ordering
- index_t + index_t union implementation ??
- 8 and odd bit archs
- get rid of useless zero extension instructions