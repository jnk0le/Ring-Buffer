# Ring Buffer

- pure C++11, no OS dependency
- lock and wait free bounded SPSC operation
- no exceptions, RTTI, virtual functions and dynamic memory allocation
- designed for compile time (static) allocation and type evaluation
- no wasted slots (in powers of 2 granularity)
- underrun and overrun checks in insert/remove functions
- highly efficient on most microcontroller architectures (nearly equal performance as in 'wasted-slot' implemetation)

## notes

- index_t of size less than architecture reg size (size_t) might not be most efficient ([known gcc bug](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71942))
- Only lamda expressions or functor callbacks can be inlined into `buffWrite`/`buffRead` functions
- 8 bit architectures are not supported in master branch at the moment. Broken code is likely to be generated
- relaxed atomic stores on RISC-V gcc port [may be inefficient](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89835)
- the DEC Alpha ultra-weak memory model is not supported

## example

```cpp
jnk0le::Ringbuffer<const char*, 256> message;

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

extern "C" void SysTick_Handler(void)
{
	message.insert("SysTick_Handler");
}
```

## CMake integration - FetchContent

```cmake
FetchContent_Declare(RingBuffer
	GIT_REPOSITORY https://github.com/jnk0le/Ring-Buffer.git
	GIT_TAG master
	FIND_PACKAGE_ARGS NAMES RingBuffer)
FetchContent_MakeAvailable(RingBuffer)
target_link_libraries(myTarget INTERFACE jnk0le::RingBuffer)
```


| CMake Option       | Description            | Default |
| :----------------- | :--------------------- | :-----: |
| RINGBUFFER_INSTALL | Create install targets |   OFF   |
