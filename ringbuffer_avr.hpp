/*****************************************************************************
* Generic ring buffer class template for embedded targets                    *
* Author : jnk0le@hotmail.com                                                *
*          https://github.com/jnk0le                                         *
* License: CC0                                                               *
******************************************************************************/

#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

// there is no STL available for AVR-GCC and likely other architectures

#include <stdint.h>
#include <stddef.h>

#ifndef __AVR_ARCH__
	#warning "this is STL less implementation dedicated dedicated for AVR-GCC, other 8 bitters or STL-less cpus should also fall here, otherwise use ringbuffer.hpp if possible"
#endif

// those functions should be inline for performance reasons (register pressure, double comparisons and references that will get passed through the stack otherwise)
// in case of need for uninlined multiple buffer write/read calls (code size reasons), it should be done at higher abstraction level

template<typename T, size_t buffer_size = 16, typename index_t = uint_fast8_t>
	class Ringbuffer
	{
	public:
		Ringbuffer() {} // empty - it have to be statically allocated during compilation
		Ringbuffer(int val)
		{
			head = val & buffer_mask;
			tail = val & buffer_mask;
		}

		~Ringbuffer() {} // empty - it have to be statically allocated during compilation

		void clear(void)
		{
			head = tail;
		}

		bool isEmpty(void)
		{
			return readAvailable() == 0;
		}

		bool isFull(void)
		{
			return writeAvailable() == 0;
		}

		index_t readAvailable(void)
		{
			return (head - tail) & buffer_mask;
		}

		index_t writeAvailable(void)
		{
			return (tail - head - 1) & buffer_mask;
		}

		bool insert(T data)
		{
			index_t tmpHead = head; // reduce overhead of accessing volatile variables and make SPSC (interrupt<->thread) access atomic

			tmpHead = (tmpHead + 1) & buffer_mask;

			if (tmpHead == tail)
				return false;
			else
			{
				data_buff[tmpHead] = data;

				asm volatile("":::"memory");
				head = tmpHead; // write it back after writing element - consumer now can read this element
			}
			return true;
		}

		bool insert(T* dst)
		{
			index_t tmpHead = head;

			tmpHead = (tmpHead + 1) & buffer_mask;

			if (tmpHead == tail)
				return false;
			else
			{
				data_buff[tmpHead] = *dst;

				asm volatile("":::"memory");
				head = tmpHead; // write it back after writing element - consumer now can read this element
			}
			return true;
		}

		bool remove(T& data)
		{
			index_t tmpTail = tail; // reduce overhead of accessing volatile variables and make SPSC (interrupt<->thread) access atomic

			if (tmpTail == head)
				return false;
			else
			{
				tmpTail = (tmpTail + 1) & buffer_mask;
				data = data_buff[tmpTail];

				asm volatile("":::"memory");
				tail = tmpTail; // write it back after reading element - producer can now use this location for new element
			}
			return true;
		}

		bool remove(T* dst)
		{
			index_t tmpTail = tail;

			if (tmpTail == head)
				return false;
			else
			{
				tmpTail = (tmpTail + 1) & buffer_mask;
				*dst = data_buff[tmpTail];

				asm volatile("":::"memory");
				tail = tmpTail; // write it back after reading element - producer can now use this location for new element
			}
			return true;
		}

	private:
		constexpr static index_t buffer_mask = buffer_size - 1;
		volatile index_t head;
		volatile index_t tail;

		T data_buff[buffer_size]; // put it at the bottom of the class to allow easier (object base pointer + offsets) access to other variables
		
		// let's assert that no UB will be compiled
		static_assert((buffer_size != 0), "buffer cannot be of zero size");
		static_assert((buffer_size != 1), "buffer cannot be of zero available size");
		static_assert(!(buffer_size & buffer_mask), "buffer size is not a power of 2");
		//static_assert(sizeof(index_t) <= sizeof(size_t), "indexing type size is larger than size_t, operation is not implemented atomic and doesn't make sense");

		static_assert(buffer_size - 1 <= UINT_FAST8_MAX, "(temporary 8b only) buffers larger than 256 elements are not implemented atomic"); // cover most UB cases
		static_assert(sizeof(index_t) == sizeof(uint8_t), "(temporary 8b only) indexing type larger than unit8_t should still be atomic (<256 bytes), but it doesn't make sense");
		
		// there is no STL available for AVR architecture
		// container type cannot be asserted at the moment
		// container type sign cannot be asserted at the moment
	};

template<typename T, size_t buffer_size = 16, typename index_t = uint_fast8_t>
	class Ringbuffer_unmasked
	{
	public:
		Ringbuffer_unmasked() {} // empty - it have to be statically allocated during compilation
		Ringbuffer_unmasked(int val)
		{
			head = val;
			tail = val;
		}

		~Ringbuffer_unmasked() {} // empty - it have to be statically allocated during compilation

		void clear(void)
		{
			head = tail;
		}

		bool isEmpty(void)
		{
			return readAvailable() == 0;
		}

		bool isFull(void)
		{
			return writeAvailable() == 0;
		}

		index_t readAvailable(void)
		{
			return head - tail;
		}

		index_t writeAvailable(void)
		{
			return buffer_size - (head - tail);
		}

		bool insert(T data)
		{
			index_t tmpHead = head; // reduce overhead of accessing volatile variables and make SPSC (interrupt<->thread) access atomic

			if ((head - tail) == buffer_size)
				return false;
			else
			{
				data_buff[tmpHead++ & buffer_mask] = data;

				asm volatile("":::"memory");
				head = tmpHead; // write it back after writing element - consumer now can read this element
			}
			return true;
		}

		bool insert(T* data)
		{
			index_t tmpHead = head; // reduce overhead of accessing volatile variables and make SPSC (interrupt<->thread) access atomic

			if ((head - tail) == buffer_size)
				return false;
			else
			{
				data_buff[tmpHead++ & buffer_mask] = *data;

				asm volatile("":::"memory");
				head = tmpHead; // write it back after writing element - consumer now can read this element
			}
			return true;
		}

		bool remove(T& data)
		{
			index_t tmpTail = tail; // reduce overhead of accessing volatile variables and make SPSC (interrupt<->thread) access atomic

			if (tmpTail == head)
				return false;
			else
			{
				data = data_buff[tmpTail++ & buffer_mask];

				asm volatile("":::"memory");
				tail = tmpTail;  // write it back after reading element - producer can now use this location for new element
			}
			return true;
		}

		bool remove(T* data)
		{
			index_t tmpTail = tail; // reduce overhead of accessing volatile variables and make SPSC (interrupt<->thread) access atomic

			if (tmpTail == head)
				return false;
			else
			{
				*data = data_buff[tmpTail++ & buffer_mask];

				asm volatile("":::"memory");
				tail = tmpTail;  // write it back after reading element - producer can now use this location for new element
			}
			return true;
		}

	private:
		constexpr static index_t buffer_mask = buffer_size - 1;
		volatile index_t head;
		volatile index_t tail;

		T data_buff[buffer_size]; // put it at the bottom of the class to allow easier (object base pointer + offsets) access to other variables

		// let's assert that no UB will be compiled
		static_assert((buffer_size != 0), "buffer cannot be of zero size");
		static_assert(!(buffer_size & buffer_mask), "buffer size is not a power of 2");
		//static_assert(sizeof(index_t) <= sizeof(size_t), "indexing type size is larger than size_t, operation is not implemented atomic and doesn't make sense");

		static_assert(((buffer_size - 1) >> 1) <= UINT_FAST8_MAX, "(temporary 8b only) buffers larger than 128 elements are not implemented atomic"); // cover most UB cases
		static_assert(sizeof(index_t) == sizeof(uint8_t), "(temporary 8b only) indexing type larger than unit8_t should still be atomic (<128 bytes), but it doesn't make sense");
		
		// there is no STL available for AVR architecture
		// container type cannot be asserted at the moment
		// container type sign cannot be asserted at the moment
	};

#endif //RINGBUFFER_HPP
