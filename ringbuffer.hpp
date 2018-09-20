/*!
 * \file ringbuffer.hpp
 * \version 1.3.0
 * \brief Generic ring buffer implementation for embedded targets
 *
 * \author jnk0le <jnk0le@hotmail.com>
 * \copyright CC0 License
 * \date 22 Jun 2017
 */

#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include <stdint.h>
#include <stddef.h>
#include <limits>
#include <atomic>

/*!
 * \brief Lock free, with no wasted slots ringbuffer implementation
 *
 * \tparam T Type of buffered elements
 * \tparam buffer_size Size of the buffer. Must be a power of 2.
 * \tparam wmo_multi_core Generate extra memory barrier instructions in case of weak core to core communication
 * \tparam index_t Type of array indexing type. Serves also as placeholder for future implementations.
 */
template<typename T, size_t buffer_size = 16, bool wmo_multi_core = false, typename index_t = size_t>
	class Ringbuffer
	{
	public:
		/*!
	 	 * \brief Default constructor, will initialize head and tail indexes
	 	 */
		Ringbuffer() : head(0), tail(0) {}

		/*!
		 * \brief Special case constructor to premature out unnecessary initialization code when object is
		 * instatiated in .bss sec
		 * \warning If object is instantiated on stack, heap or inside noinit section then the buffer have to be
	 	 * explicitly cleared before use
		 * \param val Value to initialize indexes with
		 */
		Ringbuffer(int dummy) { (void)(dummy); }

		/*!
		 * \brief Intentionally empty destructor - nothing have to be released
		 */
		~Ringbuffer() {}

		/*!
		 * \brief Clear buffer from producer side
		 */
		void producerClear(void) const {
			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);
			head = tail;
		}

		/*!
		 * \brief Clear buffer from consumer side
		 */
		void consumerClear(void) const {
			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);
			tail = head;
		}

		/*!
		 * \brief Check if buffer is empty
		 * \return Indicates if buffer is empty
		 */
		bool isEmpty(void) const {
			return readAvailable() == 0;
		}

		/*!
		 * \brief Check if buffer is full
		 * \return Indicates if buffer is full
		 */
		bool isFull(void) const {
			return writeAvailable() == 0;
		}

		/*!
		 * \brief Check how many elements can be read from the buffer
		 * \return Number of elements that can be read
		 */
		index_t readAvailable(void) const {
			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);
			return head - tail;
		}

		/*!
		 * \brief Check how many elements can be written into the buffer
		 * \return Number of free slots that can be be written
		 */
		index_t writeAvailable(void) const {
			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);
			return buffer_size - (head - tail);
		}

		/*!
		 * \brief Inserts data into internal buffer, without blocking
		 * \param data element to be inserted into internal buffer
		 * \return Indicates if callback was called and data was inserted into internal buffer
		 */
		bool insert(T data)
		{
			index_t tmp_head = head;

			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);

			if((tmp_head - tail) == buffer_size)
				return false;
			else
			{
				data_buff[tmp_head++ & buffer_mask] = data;

				if(wmo_multi_core)
					std::atomic_thread_fence(std::memory_order_release);
				else
					std::atomic_signal_fence(std::memory_order_release);

				head = tmp_head;

				if(wmo_multi_core)
					std::atomic_thread_fence(std::memory_order_release);
			}
			return true;
		}

		/*!
		 * \brief Inserts data into internal buffer, without blocking
		 * \param[in] data Pointer to memory location where element, to be inserted into internal buffer, is located
		 * \return Indicates if callback was called and data was inserted into internal buffer
		 */
		bool insert(const T* data)
		{
			index_t tmp_head = head;

			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);

			if((tmp_head - tail) == buffer_size)
				return false;
			else
			{
				data_buff[tmp_head++ & buffer_mask] = *data;

				if(wmo_multi_core)
					std::atomic_thread_fence(std::memory_order_release);
				else
					std::atomic_signal_fence(std::memory_order_release);

				head = tmp_head;

				if(wmo_multi_core)
					std::atomic_thread_fence(std::memory_order_release);
			}
			return true;
		}

		/*!
		 * \brief Inserts data returned by callback function, into internal buffer, without blocking
		 *
		 * This is a special purpose function that can be used to avoid redundant availability checks in case when
		 * acquiring data have a side effects (like clearing status flags by reading a data register)
		 *
		 * \param acquire_data_callback Pointer to callback function that returns element to be inserted into buffer
		 * \return Indicates if callback was called and data was inserted into internal buffer
		 */
		bool insertFromCallbackWhenAvailable(T (*acquire_data_callback)(void))
		{
			index_t tmp_head = head;

			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);

			if((tmp_head - tail) == buffer_size)
				return false;
			else
			{
				//execute callback only when there is space in buffer
				data_buff[tmp_head++ & buffer_mask] = acquire_data_callback();

				if(wmo_multi_core)
					std::atomic_thread_fence(std::memory_order_release);
				else
					std::atomic_signal_fence(std::memory_order_release);

				head = tmp_head;

				if(wmo_multi_core)
					std::atomic_thread_fence(std::memory_order_release);
			}
			return true;
		}

		/*!
		 * \brief Reads one element from internal buffer without blocking
		 * \param[out] data Reference to memory location where removed element will be stored
		 * \return Indicates if data was fetched from the internal buffer
		 */
		bool remove(T& data)
		{
			index_t tmp_tail = tail;

			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);

			if(tmp_tail == head)
				return false;
			else
			{
				data = data_buff[tmp_tail++ & buffer_mask];

				if(wmo_multi_core)
					std::atomic_thread_fence(std::memory_order_release);
				else
					std::atomic_signal_fence(std::memory_order_release);

				tail = tmp_tail;

				if(wmo_multi_core)
					std::atomic_thread_fence(std::memory_order_release);
			}
			return true;
		}

		/*!
		 * \brief Reads one element from internal buffer without blocking
		 * \param[out] data Pointer to memory location where removed element will be stored
		 * \return Indicates if data was fetched from the internal buffer
		 */
		bool remove(T* data)
		{
			index_t tmp_tail = tail;

			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);

			if(tmp_tail == head)
				return false;
			else
			{
				*data = data_buff[tmp_tail++ & buffer_mask];

				if(wmo_multi_core)
					std::atomic_thread_fence(std::memory_order_release);
				else
					std::atomic_signal_fence(std::memory_order_release);

				tail = tmp_tail;

				if(wmo_multi_core)
					std::atomic_thread_fence(std::memory_order_release);
			}
			return true;
		}

		/*!
		 * \brief insert multiple elements into internal buffer without blocking
		 *
		 * This function will continue writing new entries until all data is written or there is no more space.
		 * The callback function can be used to indicate to consumer that it can start fetching data.
		 *
		 * \warning This function is not deterministic
		 *
		 * \param[in] buff Pointer to buffer with data to be inserted from
		 * \param count Number of elements to write from the given buffer
		 * \param count_to_callback Number of elements to write before calling a callback function in first loop
		 * \param execute_data_callback Pointer to callback function executed after every loop iteration
		 * \return Number of elements written into circular buffer
		 */
		size_t writeBuff(const T* buff, size_t count, size_t count_to_callback = 0,
				void (*execute_data_callback)(void) = nullptr);

		/*!
		 * \brief load multiple elements from internal buffer without blocking
		 *
		 * This function will continue reading new entries until all requested data is read or there is nothing
		 * more to read.
		 * The callback function can be used to indicate to producer that it can start writing new data.
		 *
		 * \warning This function is not deterministic
		 *
		 * \param[out] buff Pointer to buffer where data will be loaded into
		 * \param count Number of elements to load into the given buffer
		 * \param count_to_callback Number of elements to load before calling a callback function in first iteration
		 * \param execute_data_callback Pointer to callback function executed after every loop iteration
		 * \return Number of elements that were read from internal buffer
		 */
		size_t readBuff(T* buff, size_t count, size_t count_to_callback = 0,
				void (*execute_data_callback)(void) = nullptr);

	protected:
		constexpr static index_t buffer_mask = buffer_size-1; //!< bitwise mask for a given buffer size
		volatile index_t head; //!< head index
		volatile index_t tail; //!< tail index

		// put buffer after variables so everything can be reached with short offsets
		T data_buff[buffer_size]; //!< actual buffer

	private:
		// let's assert that no UB will be compiled
		static_assert((buffer_size != 0), "buffer cannot be of zero size");
		static_assert(!(buffer_size & buffer_mask), "buffer size is not a power of 2");
		static_assert(sizeof(index_t) <= sizeof(size_t),
			"indexing type size is larger than size_t, operation is not lock free and doesn't make sense");

		static_assert(std::numeric_limits<index_t>::is_integer, "indexing type is not integral type");
		static_assert(!(std::numeric_limits<index_t>::is_signed), "indexing type shall not be signed");
		static_assert(buffer_mask <= (std::numeric_limits<index_t>::max() >> 1),
			"buffer size is too large for a given indexing type (maximum size for n-bit type is 2^(n-1))");
	};

template<typename T, size_t buffer_size, bool wmo_multi_core, typename index_t>
	size_t Ringbuffer<T, buffer_size, wmo_multi_core, index_t>::writeBuff(const T* buff, size_t count,
			size_t count_to_callback, void(*execute_data_callback)())
	{
		size_t written = 0;
		index_t available = 0;
		index_t tmp_head = head;
		size_t to_write = count;

		if(count_to_callback != 0 && count_to_callback < count)
			to_write = count_to_callback;

		while(written < count)
		{
			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);

			available = buffer_size - (tmp_head - tail);

			if(available == 0) // less than ??
				break;

			if(to_write > available) // do not write more than we can
				to_write = available;

			while(to_write--)
			{
				data_buff[tmp_head++ & buffer_mask] = buff[written++];
			}

			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_release);
			else
				std::atomic_signal_fence(std::memory_order_release);

			head = tmp_head;

			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_release);

			if(execute_data_callback)
				execute_data_callback();

			to_write = count - written;
		}

		return written;
	}

template<typename T, size_t buffer_size, bool wmo_multi_core, typename index_t>
	size_t Ringbuffer<T, buffer_size, wmo_multi_core, index_t>::readBuff(T* buff, size_t count,
			size_t count_to_callback, void(*execute_data_callback)())
	{
		size_t read = 0;
		index_t available = 0;
		index_t tmp_tail = tail;
		size_t to_read = count;

		if(count_to_callback != 0 && count_to_callback < count)
			to_read = count_to_callback;

		while(read < count)
		{
			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_acquire);

			available = head - tmp_tail;

			if(available == 0) // less than ??
				break;

			if(to_read > available) // do not write more than we can
				to_read = available;

			while(to_read--)
			{
				buff[read++] = data_buff[tmp_tail++ & buffer_mask];
			}

			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_release);
			else
				std::atomic_signal_fence(std::memory_order_release);

			tail = tmp_tail;

			if(wmo_multi_core)
				std::atomic_thread_fence(std::memory_order_release);

			if(execute_data_callback)
				execute_data_callback();

			to_read = count - read;
		}

		return read;
	}

#endif //RINGBUFFER_HPP
