/************************************************************************************
 *  Author: jnk0le@hotmail.com                                                      *
 *  https://github.com/jnk0le                                                       *
 *  This library is distributed under MIT license terms                             *
 ************************************************************************************/

#ifndef FIFO_8B_H_
#define FIFO_8B_H_

#include <stdint.h>

#ifndef BUFFER_SIZE
	#define BUFFER_SIZE 128 // have to be power of 2
#endif
#ifndef RingBuff_Data_t
	#define RingBuff_Data_t uint8_t
#endif

#define BUFFER_MASK (BUFFER_SIZE - 1)

	typedef struct
	{
		RingBuff_Data_t Buffer[BUFFER_SIZE];
		volatile uint8_t Head;
		volatile uint8_t Tail;
	} RingBuff_t;

	static inline void RingBuffer_Clear(RingBuff_t* Buffer) __attribute__ ((always_inline));
	static inline void RingBuffer_Clear(RingBuff_t* Buffer)
	{
		Buffer->Head = Buffer->Tail;
	}

	static inline uint8_t RingBuffer_GetCount(RingBuff_t* Buffer) __attribute__ ((always_inline));
	static inline uint8_t RingBuffer_GetCount(RingBuff_t* Buffer)
	{
		return (Buffer->Head - Buffer->Tail) & BUFFER_MASK;
	}

	static inline uint8_t RingBuffer_GetFree(RingBuff_t* Buffer) __attribute__ ((always_inline));
	static inline uint8_t RingBuffer_GetFree(RingBuff_t* Buffer)
	{
		return (Buffer->Tail - Buffer->Head - 1) & BUFFER_MASK;
	}

	static inline uint8_t RingBuffer_IsFull(RingBuff_t* Buffer) __attribute__ ((always_inline));
	static inline uint8_t RingBuffer_IsFull(RingBuff_t* Buffer)
	{
		return ((Buffer->Head + 1) & BUFFER_MASK) == Buffer->Tail;
	}

	static inline uint8_t RingBuffer_IsEmpty(RingBuff_t* Buffer) __attribute__ ((always_inline));
	static inline uint8_t RingBuffer_IsEmpty(RingBuff_t* Buffer)
	{
		return (Buffer->Head == Buffer->Tail);
	}

	static inline uint8_t RingBuffer_Insert(RingBuff_t* Buffer, RingBuff_Data_t Data) __attribute__ ((always_inline));
	static inline uint8_t RingBuffer_Insert(RingBuff_t* Buffer, RingBuff_Data_t Data)
	{
		uint8_t tmpHead = Buffer->Head; // reduce overhead of accessing volatile variables and make buffer access atomic
	
		tmpHead = (tmpHead + 1) & BUFFER_MASK;
	
		if(tmpHead == Buffer->Tail)
			return 0;
		else
		{
			Buffer->Buffer[tmpHead] = Data; 
			Buffer->Head = tmpHead; // write it back after writing element
			return 1;
		}
	}

	static inline uint8_t RingBuffer_Load(RingBuff_t* Buffer, RingBuff_Data_t* Data) __attribute__ ((always_inline));
	static inline uint8_t RingBuffer_Load(RingBuff_t* Buffer, RingBuff_Data_t* Data) // this shit will be optimized out as an inline function 
	{
		uint8_t tmpTail = Buffer->Tail; // reduce overhead of accessing volatile variables and make buffer access atomic
		RingBuff_Data_t tmp;
	
		if(tmpTail == Buffer->Head)
			return 0;
		else
		{
			tmpTail = (tmpTail + 1) & BUFFER_MASK;
			tmp = Buffer->Buffer[tmpTail];
			Buffer->Tail = tmpTail; // write it back after reading element
			*Data = tmp; 
			return 1;
		}
	}

	static inline RingBuff_Data_t RingBuffer_Remove(RingBuff_t* Buffer) __attribute__ ((always_inline));
	static inline RingBuff_Data_t RingBuffer_Remove(RingBuff_t* Buffer)
	{
		uint8_t tmpTail = Buffer->Tail; // reduce overhead of accessing volatile variables and make buffer access atomic
		RingBuff_Data_t tmp;
	
		if(tmpTail == Buffer->Head)
			return 0;
		else
		{
			tmpTail = (tmpTail + 1) & BUFFER_MASK;
			tmp = Buffer->Buffer[tmpTail];
			Buffer->Tail = tmpTail; // write it back after reading element
			return tmp;
		}
	}

/*
static inline void RingBuffer_Insert_NoCheck(RingBuff_t* Buffer, RingBuff_Data_t Data) __attribute__ ((always_inline));
static inline void RingBuffer_Insert_NoCheck(RingBuff_t* Buffer, RingBuff_Data_t Data)
{
	uint8_t tmpHead = Buffer->Head;
	
	tmpHead = (tmpHead + 1) & BUFFER_MASK;
	
	Buffer->Buffer[tmpHead] = Data;
	Buffer->Head = tmpHead;
}

static inline RingBuff_Data_t RingBuffer_Remove_NoCheck(RingBuff_t* Buffer) __attribute__ ((always_inline));
static inline RingBuff_Data_t RingBuffer_Remove_NoCheck(RingBuff_t* Buffer)
{
	uint8_t tmpTail = Buffer->Tail;
	uint8_t tmp;
	
	tmpTail = (tmpTail + 1) & BUFFER_MASK;
	tmp = Buffer->Buffer[tmpTail];
	Buffer->Head = tmpTail;

	return tmp;
}
*/

#endif // FIFO_8B_H_ 