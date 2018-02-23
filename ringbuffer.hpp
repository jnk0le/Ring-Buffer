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

// those functions are intentionally inline for performance reasons (register pressure, double
// comparisons and references that will get passed through the stack otherwise). in case of
// requirement for uninlined multiple calls, it should be done at higher abstraction level

template<typename T, size_t buffer_size = 16, typename index_t = size_t>
	class Ringbuffer
	{
	public:
		Ringbuffer() {} // empty - it have to be statically allocated during compilation
		Ringbuffer(int val)
		{
			head = val;
			tail = val;
		}

		~Ringbuffer() {} // empty - it have to be statically allocated during compilation

		void producerClear(void){
			head = tail;
		}

		void consumerClear(void){
			tail = head;
		}

		bool isEmpty(void){
			return readAvailable() == 0;
		}

		bool isFull(void){
			return writeAvailable() == 0;
		}

		index_t readAvailable(void){
			return head - tail;
		}

		index_t writeAvailable(void){
			return buffer_size - (head - tail);
		}

		bool insert(T data)
		{
			index_t tmpHead = head;

			if((tmpHead - tail) == buffer_size)
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

			if((tmpHead - tail) == buffer_size)
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

		size_t writeBuff(T* buff, size_t count, size_t count_to_callback = 0,
				void (*execute_data_callback)(void) = nullptr);

		size_t readBuff(T* buff, size_t count, size_t count_to_callback = 0,
				void (*execute_data_callback)(void) = nullptr);

	private:
		constexpr static index_t buffer_mask = buffer_size-1;
		volatile index_t head;
		volatile index_t tail;

		T data_buff[buffer_size]; // put buffer after variables so everything can be reached with short offsets

		// let's assert that no UB will be compiled
		static_assert((buffer_size != 0), "buffer cannot be of zero size");
		static_assert(!(buffer_size & buffer_mask), "buffer size is not a power of 2");
		static_assert(sizeof(index_t) <= sizeof(size_t),
			"indexing type size is larger than size_t, operation is not lock free and doesn't make sense");

		static_assert(std::numeric_limits<index_t>::is_integer, "indexing type is not integral type");
		static_assert(!(std::numeric_limits<index_t>::is_signed), "indexing type shall not be signed");
		static_assert(buffer_mask <= (std::numeric_limits<index_t>::max() >> 1),
			"buffer size is too large for a given indexing type (maximum size for n-bit variable is 2^(n-1))");
	};

template<typename T, size_t buffer_size, typename index_t>
	size_t Ringbuffer<T, buffer_size, index_t>::writeBuff(T* buff, size_t count, size_t count_to_callback,
			void(*execute_data_callback)())
	{
		size_t written = 0;
		index_t available = 0;
		index_t tmpHead = head;
		size_t toWrite = count;

		if(count_to_callback != 0 && count_to_callback < count)
			toWrite = count_to_callback;

		while(written < count)
		{
			available = buffer_size - (tmpHead - tail);

			if(available == 0) // less than ??
				break;

			if(toWrite > available) // do not write more than we can
				toWrite = available;

			while(toWrite--)
			{
				data_buff[tmpHead++ & buffer_mask] = buff[written++];
			}

			std::atomic_signal_fence(std::memory_order_release);
			head = tmpHead;

			if(execute_data_callback)
				execute_data_callback();

			toWrite = count - written;
		}

		return written;
	}

template<typename T, size_t buffer_size, typename index_t>
	size_t Ringbuffer<T, buffer_size, index_t>::readBuff(T* buff, size_t count, size_t count_to_callback,
			void(*execute_data_callback)())
	{
		size_t read = 0;
		index_t available = 0;
		index_t tmpTail = tail;
		size_t toRead = count;

		if(count_to_callback != 0 && count_to_callback < count)
			toRead = count_to_callback;

		while(read < count)
		{
			available = head - tmpTail;

			if(available == 0) // less than ??
				break;

			if(toRead > available) // do not write more than we can
				toRead = available;

			while(toRead--)
			{
				buff[read++] = data_buff[tmpTail++ & buffer_mask];
			}

			std::atomic_signal_fence(std::memory_order_release);
			tail = tmpTail;

			if(execute_data_callback)
				execute_data_callback();

			toRead = count - read;
		}

		return read;
	}

#endif //RINGBUFFER_HPP
