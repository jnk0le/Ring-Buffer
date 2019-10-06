# Ring Buffer

- C++11 and above
- no exceptions, RTTI, virtual functions and dynamic memory allocation
- designed for compile time (static) allocation and type evaluation
- No wasted slots
- lock-free atomic operation in SPSC cases
- underrun and overrun checks in insert/remove functions
- highly efficient on most microcontroller architectures (nearly equal performance as in 'wasted-slot' implemetation)

## notes

- index_t of size less than architecture reg size (size_t) might not be most efficient ([known gcc bug](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71942))
- Only lamda expressions or functor callbacks can be inlined into `buffWrite`/`buffRead` functions
- 8 and odd (53, 48, etc) bit architectures are not supported in master branch at the moment. Broken code is likely to be generated.
- relaxed atomic stores on RISC-V gcc port [may be inefficient](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89835)

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