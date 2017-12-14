/*****************************************************************************
 * Generic ring buffer class for embedded targets                            *
 * Author : jnk0le@hotmail.com                                               *
 *          https://github.com/jnk0le                                        *
 * License: CC0                                                              *
 *****************************************************************************/

#ifndef FIFO_HPP
#define FIFO_HPP

#include <stdint.h>

#ifndef __AVR_ARCH__ // there is no STL available for AVR architecture
	#include <limits>
#endif

template<typename T, size_t buffer_size = 16, typename index_t = uint_fast8_t>
	class Ringbuffer
	{
	public:
		Ringbuffer() {} // empty - it have to be statically allocated during compilation
		//default copy constructor will not harm anything
		~Ringbuffer() {} // empty - it have to be statically allocated during compilation

		void clear(void)
		{
			head = tail;
		}

		bool isEmpty(void)
		{
			return getCount() == 0;
		}

		bool isFull(void)
		{
			return getFree() == 0;
		}

		index_t getCount(void)
		{
			return (head - tail) & buffer_mask;
		}

		index_t getFree(void)
		{
			return (tail - head - 1) & buffer_mask;
		}

		size_t getSize(void)
		{
			return buffer_size;
		}

		size_t getUsableSize(void)
		{
			return buffer_size-1;
		}

		bool insert(T data)
		{
			index_t tmpHead = head; // reduce overhead of accessing volatile variables and make SPSC (interrupt<->thread) access atomic

			tmpHead = (tmpHead + 1) & buffer_mask;

			if(tmpHead == tail)
				return false;
			else
			{
				data_buff[tmpHead] = data;
				head = tmpHead; // write it back after writing element - consumer now can read this element
			}
			return true;
		}

		bool insert(T* dst)
		{
			index_t tmpHead = head;

			tmpHead = (tmpHead + 1) & buffer_mask;

			if(tmpHead == tail)
				return false;
			else
			{
				data_buff[tmpHead] = *dst;
				head = tmpHead; // write it back after writing element - consumer now can read this element
			}
			return true;
		}

		bool remove(T& data)
		{
			index_t tmpTail = tail; // reduce overhead of accessing volatile variables and make SPSC (interrupt<->thread) access atomic

			if(tmpTail == head)
				return false;
			else
			{
				tmpTail = (tmpTail + 1) & buffer_mask;
				data = data_buff[tmpTail];
				tail = tmpTail; // write it back after reading element - producer can now use this location for new element
			}
			return true;
		}

		bool remove(T* dst)
		{
			index_t tmpTail = tail;

			if(tmpTail == head)
				return false;
			else
			{
				tmpTail = (tmpTail + 1) & buffer_mask;
				*dst = data_buff[tmpTail];
				tail = tmpTail; // write it back after reading element - producer can now use this location for new element
			}
			return true;
		}
	private:
		constexpr static index_t buffer_mask = buffer_size-1;
		volatile index_t head, tail;

		T data_buff[buffer_size]; // put it at the bottom of the class to allow easier (object base pointer + offsets) access to other variables

		// let's assert that no UB will be compiled
		static_assert((buffer_size != 0), "buffer cannot be of zero size");
		static_assert((buffer_size != 1), "buffer cannot be of zero available size");
		static_assert(!(buffer_size & buffer_mask), "buffer size is not a power of 2");
		static_assert(sizeof(index_t) <= sizeof(buffer_size), "indexing type size is larger than size_t, operation is not implemented atomic and doesn't make sense");

	#ifdef __AVR_ARCH__ // there is no STL available for AVR architecture
		static_assert(buffer_size-1 <= UINT_FAST8_MAX, "buffers larger than 256 elements are not implemented atomic"); // cover most UB cases
		static_assert(sizeof(index_t) == sizeof(uint8_t), "indexing type larger than unit8_t should still be atomic (<256 bytes), but it doesn't make sense");
		// container type sign cannot be checked at the moment
	#else // just do some regular assertions
		static_assert(std::numeric_limits<index_t>::is_integer, "indexing type is not integral type");
		static_assert(!(std::numeric_limits<index_t>::is_signed), "indexing type shall not be signed");
		static_assert(buffer_mask <= std::numeric_limits<index_t>::max(), "buffer size is too large for a given indexing type");
	#endif
	};

#endif //FIFO_HPP
