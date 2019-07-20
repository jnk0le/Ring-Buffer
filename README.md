# Ring Buffer

- C++11 and above
- no exceptions, RTTI, virtual functions and dynamic memory allocation
- designed for compile time (static) allocation and type evaluation
- No wasted slots
- lock-free atomic operation in SPSC cases
- underrun and overrun checks in insert/remove functions
- highly efficient on most microcontroller architectures

## notes

- On cortex-m and similiar architectures, larger buffer sizes will generate larger instructions (execution might be slower due to waitstates or additional necessary instructions)
- index_t of size less than architecture reg size (size_t) might not be most efficient (arm gcc will generate redundant `uxth/uxtb`)
- Only lamda expressions or functor callbacks can be inlined into `buffWrite`/`buffRead` functions (gcc constprops optimization) 
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