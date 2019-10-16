/*!
 * \file ringbuffer.hpp
 * \version 1.7.0
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
 * \tparam wmo_multi_core Generate extra memory barrier instructions in case of core to core communication
 * \tparam cacheline_size Size of the cache line, to insert appropriate padding in between indexes and buffer
 * \tparam index_t Type of array indexing type. Serves also as placeholder for future implementations.
 */
template<typename T, size_t buffer_size = 16, bool wmo_multi_core = true, size_t cacheline_size = 0, typename index_t = size_t>
	class Ringbuffer
	{
	public:
		/*!
	 	 * \brief Default constructor, will initialize head and tail indexes
	 	 */
		Ringbuffer() : head(0), tail(0) {}

		/*!
		 * \brief Special case constructor to premature out unnecessary initialization code when object is
		 * instatiated in .bss section
		 * \warning If object is instantiated on stack, heap or inside noinit section then the buffer have to be
	 	 * explicitly cleared before use
		 * \param dummy Ignored
		 */
		Ringbuffer(int dummy) { (void)(dummy); }

		/*!
		 * \brief Clear buffer from producer side
		 */
		void producerClear(void) const {
			//this may fail
			head.store(tail.load(std::memory_order_relaxed), std::memory_order_relaxed);
		}

		/*!
		 * \brief Clear buffer from consumer side
		 */
		void consumerClear(void) const {
			tail.store(head.load(std::memory_order_relaxed), std::memory_order_relaxed);
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
			return head.load(index_acquire_barrier) - tail.load(std::memory_order_relaxed);
		}

		/*!
		 * \brief Check how many elements can be written into the buffer
		 * \return Number of free slots that can be be written
		 */
		index_t writeAvailable(void) const {
			return buffer_size - (head.load(std::memory_order_relaxed) - tail.load(index_acquire_barrier));
		}

		/*!
		 * \brief Inserts data into internal buffer, without blocking
		 * \param data element to be inserted into internal buffer
		 * \return Indicates if callback was called and data was inserted into internal buffer
		 */
		bool insert(T data)
		{
			index_t tmp_head = head.load(std::memory_order_relaxed);

			if((tmp_head - tail.load(index_acquire_barrier)) == buffer_size)
				return false;
			else
			{
				data_buff[tmp_head++ & buffer_mask] = data;
				std::atomic_signal_fence(std::memory_order_release);
				head.store(tmp_head, index_release_barrier);
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
			index_t tmp_head = head.load(std::memory_order_relaxed);

			if((tmp_head - tail.load(index_acquire_barrier)) == buffer_size)
				return false;
			else
			{
				data_buff[tmp_head++ & buffer_mask] = *data;
				std::atomic_signal_fence(std::memory_order_release);
				head.store(tmp_head, index_release_barrier);
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
			index_t tmp_head = head.load(std::memory_order_relaxed);

			if((tmp_head - tail.load(index_acquire_barrier)) == buffer_size)
				return false;
			else
			{
				//execute callback only when there is space in buffer
				data_buff[tmp_head++ & buffer_mask] = acquire_data_callback();
				std::atomic_signal_fence(std::memory_order_release);
				head.store(tmp_head, index_release_barrier);
			}
			return true;
		}

		/*!
		 * \brief
		 * \return
		 */
		bool remove()
		{
			index_t tmp_tail = tail.load(std::memory_order_relaxed);

			if(tmp_tail == head.load(std::memory_order_relaxed))
				return false;
			else
				tail.store(++tmp_tail, index_release_barrier); // release in case data was loaded/used before

			return true;
		}

		/*!
		 * \brief Reads one element from internal buffer without blocking
		 * \param[out] data Reference to memory location where removed element will be stored
		 * \return Indicates if data was fetched from the internal buffer
		 */
		bool remove(T& data)
		{
			index_t tmp_tail = tail.load(std::memory_order_relaxed);

			if(tmp_tail == head.load(index_acquire_barrier))
				return false;
			else
			{
				data = data_buff[tmp_tail++ & buffer_mask];
				std::atomic_signal_fence(std::memory_order_release);
				tail.store(tmp_tail, index_release_barrier);
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
			index_t tmp_tail = tail.load(std::memory_order_relaxed);

			if(tmp_tail == head.load(index_acquire_barrier))
				return false;
			else
			{
				*data = data_buff[tmp_tail++ & buffer_mask];
				std::atomic_signal_fence(std::memory_order_release);
				tail.store(tmp_tail, index_release_barrier);
			}
			return true;
		}

		/*!
		 * \brief
		 * \return
		 */
		T* peek() {
			index_t tmp_tail = tail.load(std::memory_order_relaxed);

			if(tmp_tail == head.load(index_acquire_barrier))
				return nullptr;
			else
				return &data_buff[tmp_tail];
		}

		/*!
		 * \brief
		 * \param index
		 * \return
		 */
		T* at(size_t index) {
			index_t tmp_tail = tail.load(std::memory_order_relaxed);

			if((head.load(index_acquire_barrier) - tmp_tail) <= index)
				return nullptr;
			else
				return &data_buff[(tmp_tail + index) & buffer_mask];
		}

		/*!
		 * \brief
		 * \param index
		 * \return
		 */
		T& operator[](size_t index) {
			return data_buff[(tail.load(std::memory_order_relaxed) + index) & buffer_mask];
		}

		/*!
		 * \brief insert multiple elements into internal buffer without blocking
		 *
		 * This function will insert as much data as possible from given buffer.
		 *
		 * \param[in] buff Pointer to buffer with data to be inserted from
		 * \param count Number of elements to write from the given buffer
		 * \return Number of elements written into circular buffer
		 */
		size_t writeBuff(const T* buff, size_t count);

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
		size_t writeBuff(const T* buff, size_t count, size_t count_to_callback, void (*execute_data_callback)(void));

		/*!
		 * \brief load multiple elements from internal buffer without blocking
		 *
		 * This function will read up to specified amount of data.
		 *
		 * \param[out] buff Pointer to buffer where data will be loaded into
		 * \param count Number of elements to load into the given buffer
		 * \return Number of elements that were read from internal buffer
		 */
		size_t readBuff(T* buff, size_t count);

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
		size_t readBuff(T* buff, size_t count, size_t count_to_callback, void (*execute_data_callback)(void));

	protected:
		constexpr static index_t buffer_mask = buffer_size-1; //!< bitwise mask for a given buffer size
		constexpr static std::memory_order index_acquire_barrier = wmo_multi_core ?
				  std::memory_order_acquire // do not load from, or store to buffer before confirmed by the opposite side
				: std::memory_order_relaxed;
		constexpr static std::memory_order index_release_barrier = wmo_multi_core ?
				  std::memory_order_release // do not update own side before all operations on data_buff committed
				: std::memory_order_relaxed;

		alignas(cacheline_size) std::atomic<index_t> head; //!< head index
		alignas(cacheline_size) std::atomic<index_t> tail; //!< tail index

		// put buffer after variables so everything can be reached with short offsets
		alignas(cacheline_size) T data_buff[buffer_size]; //!< actual buffer

	private:
		// let's assert that no UB will be compiled in
		static_assert((buffer_size != 0), "buffer cannot be of zero size");
		static_assert(!(buffer_size & buffer_mask), "buffer size is not a power of 2");
		static_assert(sizeof(index_t) <= sizeof(size_t),
			"indexing type size is larger than size_t, operation is not lock free and doesn't make sense");

		static_assert(std::numeric_limits<index_t>::is_integer, "indexing type is not integral type");
		static_assert(!(std::numeric_limits<index_t>::is_signed), "indexing type shall not be signed");
		static_assert(buffer_mask <= (std::numeric_limits<index_t>::max() >> 1),
			"buffer size is too large for a given indexing type (maximum size for n-bit type is 2^(n-1))");
	};

template<typename T, size_t buffer_size, bool wmo_multi_core, size_t cacheline_size, typename index_t>
	size_t Ringbuffer<T, buffer_size, wmo_multi_core, cacheline_size, index_t>::writeBuff(const T* buff, size_t count)
	{
		index_t available = 0;
		index_t tmp_head = head.load(std::memory_order_relaxed);
		size_t to_write = count;

		available = buffer_size - (tmp_head - tail.load(index_acquire_barrier));

		if(available < count) // do not write more than we can
			to_write = available;

		// maybe divide it into 2 separate writes
		for(size_t i = 0; i < to_write; i++)
			data_buff[tmp_head++ & buffer_mask] = buff[i];

		std::atomic_signal_fence(std::memory_order_release);
		head.store(tmp_head, index_release_barrier);

		return to_write;
	}

template<typename T, size_t buffer_size, bool wmo_multi_core, size_t cacheline_size, typename index_t>
	size_t Ringbuffer<T, buffer_size, wmo_multi_core, cacheline_size, index_t>::writeBuff(const T* buff, size_t count,
			size_t count_to_callback, void(*execute_data_callback)())
	{
		size_t written = 0;
		index_t available = 0;
		index_t tmp_head = head.load(std::memory_order_relaxed);
		size_t to_write = count;

		if(count_to_callback != 0 && count_to_callback < count)
			to_write = count_to_callback;

		while(written < count)
		{
			available = buffer_size - (tmp_head - tail.load(index_acquire_barrier));

			if(available == 0) // less than ??
				break;

			if(to_write > available) // do not write more than we can
				to_write = available;

			while(to_write--)
				data_buff[tmp_head++ & buffer_mask] = buff[written++];

			std::atomic_signal_fence(std::memory_order_release);
			head.store(tmp_head, index_release_barrier);

			if(execute_data_callback != nullptr)
				execute_data_callback();

			to_write = count - written;
		}

		return written;
	}

template<typename T, size_t buffer_size, bool wmo_multi_core, size_t cacheline_size, typename index_t>
	size_t Ringbuffer<T, buffer_size, wmo_multi_core, cacheline_size, index_t>::readBuff(T* buff, size_t count)
	{
		index_t available = 0;
		index_t tmp_tail = tail.load(std::memory_order_relaxed);
		size_t to_read = count;

		available = head.load(index_acquire_barrier) - tmp_tail;

		if(available < count) // do not read more than we can
			to_read = available;

		// maybe divide it into 2 separate reads
		for(size_t i = 0; i < to_read; i++)
			buff[i] = data_buff[tmp_tail++ & buffer_mask];

		std::atomic_signal_fence(std::memory_order_release);
		tail.store(tmp_tail, index_release_barrier);

		return to_read;
	}

template<typename T, size_t buffer_size, bool wmo_multi_core, size_t cacheline_size, typename index_t>
	size_t Ringbuffer<T, buffer_size, wmo_multi_core, cacheline_size, index_t>::readBuff(T* buff, size_t count,
			size_t count_to_callback, void(*execute_data_callback)())
	{
		size_t read = 0;
		index_t available = 0;
		index_t tmp_tail = tail.load(std::memory_order_relaxed);
		size_t to_read = count;

		if(count_to_callback != 0 && count_to_callback < count)
			to_read = count_to_callback;

		while(read < count)
		{
			available = head.load(index_acquire_barrier) - tmp_tail;

			if(available == 0) // less than ??
				break;

			if(to_read > available) // do not write more than we can
				to_read = available;

			while(to_read--)
				buff[read++] = data_buff[tmp_tail++ & buffer_mask];

			std::atomic_signal_fence(std::memory_order_release);
			tail.store(tmp_tail, index_release_barrier);

			if(execute_data_callback != nullptr)
				execute_data_callback();

			to_read = count - read;
		}

		return read;
	}

#endif //RINGBUFFER_HPP
