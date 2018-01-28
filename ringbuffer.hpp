/*********************************************************************************************************************
 * Generic ring buffer class templates for embedded targets                                                          *
 * Author : jnk0le@hotmail.com                                                                                       *
 *          https://github.com/jnk0le                                                                                *
 * License: CC0                                                                                                      *
 *********************************************************************************************************************/

#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include <stdint.h>
#include <stddef.h>
#include <limits>
#include <atomic>

// those functions are intentionally inline for performance reasons (register pressure, double comparisons
// and references that will get passed through the stack otherwise). in case of requirement for uninlined multiple
// buffer write/read calls (code size reasons), it should be done at higher abstraction level

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
			index_t tmpHead = head;

			tmpHead = (tmpHead + 1) & buffer_mask;

			if(tmpHead == tail)
				return false;
			else
			{
				data_buff[tmpHead] = data;

				std::atomic_signal_fence(std::memory_order_release);
				head = tmpHead;
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

				std::atomic_signal_fence(std::memory_order_release);
				head = tmpHead;
			}
			return true;
		}

		bool remove(T& data)
		{
			index_t tmpTail = tail;

			if(tmpTail == head)
				return false;
			else
			{
				tmpTail = (tmpTail + 1) & buffer_mask;
				data = data_buff[tmpTail];

				std::atomic_signal_fence(std::memory_order_release);
				tail = tmpTail;
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

				std::atomic_signal_fence(std::memory_order_release);
				tail = tmpTail;
			}
			return true;
		}

	private:
		constexpr static index_t buffer_mask = buffer_size-1;
		volatile index_t head;
		volatile index_t tail;

		T data_buff[buffer_size]; // put buffer after variables so everything can be reached with short offsets

		// let's assert that no UB will be compiled
		static_assert((buffer_size != 0), "buffer cannot be of zero size");
		static_assert((buffer_size != 1), "buffer cannot be of zero available size");
		static_assert(!(buffer_size & buffer_mask), "buffer size is not a power of 2");
		static_assert(sizeof(index_t) <= sizeof(size_t),
			"indexing type size is larger than size_t, operation is not implemented atomic and doesn't make sense");

		static_assert(std::numeric_limits<index_t>::is_integer, "indexing type is not integral type");
		static_assert(!(std::numeric_limits<index_t>::is_signed), "indexing type shall not be signed");
		static_assert(buffer_mask <= std::numeric_limits<index_t>::max(),
			"buffer size is too large for a given indexing type (maximum size for n-bit variable is 2^n)");

		static_assert(sizeof(uint8_t) != sizeof(uint_fast8_t),
			"(temporary workaround) cannot do a range check for 8 bit architectures at the moment. "
			"Please shot an issue if you belive that your cpu is compatible or not 8-bit");
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
			index_t tmpHead = head;

			if((head - tail) == buffer_size)
				return false;
			else
			{
				data_buff[tmpHead++ & buffer_mask] = data;

				std::atomic_signal_fence(std::memory_order_release);
				head = tmpHead;
			}
			return true;
		}

		bool insert(T* data)
		{
			index_t tmpHead = head;

			if((head - tail) == buffer_size)
				return false;
			else
			{
				data_buff[tmpHead++ & buffer_mask] = *data;

				std::atomic_signal_fence(std::memory_order_release);
				head = tmpHead;
			}
			return true;
		}

		bool remove(T& data)
		{
			index_t tmpTail = tail;

			if(tmpTail == head)
				return false;
			else
			{
				data = data_buff[tmpTail++ & buffer_mask];

				std::atomic_signal_fence(std::memory_order_release);
				tail = tmpTail;
			}
			return true;
		}

		bool remove(T* data)
		{
			index_t tmpTail = tail;

			if(tmpTail == head)
				return false;
			else
			{
				*data = data_buff[tmpTail++ & buffer_mask];

				std::atomic_signal_fence(std::memory_order_release);
				tail = tmpTail;
			}
			return true;
		}

	private:
		constexpr static index_t buffer_mask = buffer_size-1;
		volatile index_t head;
		volatile index_t tail;

		T data_buff[buffer_size]; // put buffer after variables so everything can be reached with short offsets

		// let's assert that no UB will be compiled
		static_assert((buffer_size != 0), "buffer cannot be of zero size");
		static_assert(!(buffer_size & buffer_mask), "buffer size is not a power of 2");
		static_assert(sizeof(index_t) <= sizeof(size_t),
			"indexing type size is larger than size_t, operation is not implemented atomic and doesn't make sense");

		static_assert(std::numeric_limits<index_t>::is_integer, "indexing type is not integral type");
		static_assert(!(std::numeric_limits<index_t>::is_signed), "indexing type shall not be signed");
		static_assert(buffer_mask <= (std::numeric_limits<index_t>::max() >> 1),
			"buffer size is too large for a given indexing type (maximum size for n-bit variable is 2^(n-1))");

		static_assert(sizeof(uint8_t) != sizeof(uint_fast8_t) ,
			"(temporary workaround) cannot do a range check for 8 bit architectures at the moment. "
			"Please shot an issue if you belive that your cpu is compatible or not 8-bit");
	};

#endif //RINGBUFFER_HPP
