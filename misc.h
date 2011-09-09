#ifndef CRYPTOPP_MISC_H
#define CRYPTOPP_MISC_H

#include "config.h"
#include <assert.h>
#include <memory.h>
#include <algorithm>

NAMESPACE_BEGIN(CryptoPP)

inline unsigned int bitsToBytes(unsigned int bitCount)
{
	return ((bitCount+7)/(8));
}

inline unsigned int bytesToWords(unsigned int byteCount)
{
	return ((byteCount+WORD_SIZE-1)/WORD_SIZE);
}

inline unsigned int bitsToWords(unsigned int bitCount)
{
	return ((bitCount+WORD_BITS-1)/(WORD_BITS));
}

void xorbuf(byte *buf, const byte *mask, unsigned int count);
void xorbuf(byte *output, const byte *input, const byte *mask, unsigned int count);

template <class T> inline T rotl(T x, unsigned int y)
{
	return ((x<<y) | (x>>(sizeof(T)*8-y)));
}

template <class T> inline T rotr(T x, unsigned int y)
{
	return ((x>>y) | (x<<(sizeof(T)*8-y)));
}

#if defined(_MSC_VER) || defined(__BCPLUSPLUS__)

#include <stdlib.h>
#define FAST_ROTATE

template<> inline unsigned short rotl<unsigned short>(unsigned short x, unsigned int y)
{
	return (unsigned short)((x<<y) | (x>>(16-y)));
}

template<> inline unsigned short rotr<unsigned short>(unsigned short x, unsigned int y)
{
	return (unsigned short)((x>>y) | (x<<(16-y)));
}

template<> inline unsigned long rotl<unsigned long>(unsigned long x, unsigned int y)
{
	return _lrotl(x, y);
}

template<> inline unsigned long rotr<unsigned long>(unsigned long x, unsigned int y)
{
	return _lrotr(x, y);
}
#endif // defined(_MSC_VER) || defined(__BCPLUSPLUS__)

inline word16 byteReverse(word16 value)
{
	return rotl(value, 8U);
}

inline word32 byteReverse(word32 value)
{
#ifdef FAST_ROTATE
	// 5 instructions with rotate instruction, 9 without
	return (rotr(value, 8U) & 0xff00ff00) | (rotl(value, 8U) & 0x00ff00ff);
#else
	// 6 instructions with rotate instruction, 8 without
	value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
	return rotl(value, 16U);
#endif
}

#ifdef WORD64_AVAILABLE
inline word64 byteReverse(word64 value)
{
	value = ((value & W64LIT(0xFF00FF00FF00FF00)) >> 8) | ((value & W64LIT(0x00FF00FF00FF00FF)) << 8);
	value = ((value & W64LIT(0xFFFF0000FFFF0000)) >> 16) | ((value & W64LIT(0x0000FFFF0000FFFF)) << 16);
	return rotl(value, 32U);
}
#endif

template <class T> void byteReverse(T *out, const T *in, unsigned int byteCount)
{
	assert(byteCount%sizeof(T) == 0);
	byteCount /= sizeof(T);
	for (unsigned i=0; i<byteCount; i++)
		out[i] = byteReverse(in[i]);
}

#ifdef _MSC_VER
#define GETBYTE(x, y) (((byte *)&(x))[y])
#else
#define GETBYTE(x, y) (unsigned int)(((x)>>(8*(y)))&255)
#endif

unsigned int Parity(unsigned long);
unsigned int BytePrecision(unsigned long);
unsigned int BitPrecision(unsigned long);
unsigned long Crop(unsigned long, int size);

// ********************************************************

#ifdef SECALLOC_DEFAULT
#define SecAlloc(type, number) (new type[(number)])
#define SecFree(ptr, number) (memset((ptr), 0, (number)*sizeof(*(ptr))), delete [] (ptr))
#else
#define SecAlloc(type, number) (new type[(number)])
#define SecFree(ptr, number) (delete [] (ptr))
#endif

template <class T> struct SecBlock
{
	SecBlock(unsigned int size=0)
		: size(size) {ptr = SecAlloc(T, size);}
	SecBlock(const SecBlock<T> &t)
		: size(t.size) {ptr = SecAlloc(T, size); CopyFrom(t);}
	SecBlock(const T *t, unsigned int size)
		: size(size) {ptr = SecAlloc(T, size); memcpy(ptr, t, size*sizeof(T));}
	~SecBlock()
		{SecFree(ptr, size);}

#if defined(__GNUC__) || defined(__BCPLUSPLUS__)
	operator const void *() const
		{return ptr;}
	operator void *()
		{return ptr;}
#endif

	operator const T *() const
		{return ptr;}
	operator T *()
		{return ptr;}

#ifndef _MSC_VER
	T *operator +(unsigned int offset)
		{return ptr+offset;}
	const T *operator +(unsigned int offset) const
		{return ptr+offset;}
	T& operator[](unsigned int index)
		{assert(index<size); return ptr[index];}
	const T& operator[](unsigned int index) const
		{assert(index<size); return ptr[index];}
#endif

	const T* Begin() const
		{return ptr;}
	T* Begin()
		{return ptr;}
	const T* End() const
		{return ptr+size;}
	T* End()
		{return ptr+size;}

	void CopyFrom(const SecBlock<T> &t)
	{
		New(t.size);
		memcpy(ptr, t.ptr, size*sizeof(T));
	}

	SecBlock& operator=(const SecBlock<T> &t)
	{
		CopyFrom(t);
		return *this;
	}

	bool operator==(const SecBlock<T> &t) const
	{
		return size == t.size && memcmp(ptr, t.ptr, size*sizeof(T)) == 0;
	}

	void New(unsigned int newSize)
	{
		if (newSize != size)
		{
			T *newPtr = SecAlloc(T, newSize);
			SecFree(ptr, size);
			ptr = newPtr;
			size = newSize;
		}
	}

	void CleanNew(unsigned int newSize)
	{
		if (newSize != size)
		{
			T *newPtr = SecAlloc(T, newSize);
			SecFree(ptr, size);
			ptr = newPtr;
			size = newSize;
		}
		memset(ptr, 0, size*sizeof(T));
	}

	void Grow(unsigned int newSize)
	{
		if (newSize > size)
		{
			T *newPtr = SecAlloc(T, newSize);
			memcpy(newPtr, ptr, size*sizeof(T));
			SecFree(ptr, size);
			ptr = newPtr;
			size = newSize;
		}
	}

	void CleanGrow(unsigned int newSize)
	{
		if (newSize > size)
		{
			T *newPtr = SecAlloc(T, newSize);
			memcpy(newPtr, ptr, size*sizeof(T));
			memset(newPtr+size, 0, (newSize-size)*sizeof(T));
			SecFree(ptr, size);
			ptr = newPtr;
			size = newSize;
		}
	}

	void Resize(unsigned int newSize)
	{
		if (newSize != size)
		{
			T *newPtr = SecAlloc(T, newSize);
			memcpy(newPtr, ptr, STDMIN(newSize, size)*sizeof(T));
			SecFree(ptr, size);
			ptr = newPtr;
			size = newSize;
		}
	}

	void swap(SecBlock<T> &b);

	unsigned int size;
	T *ptr;
};

template <class T> void SecBlock<T>::swap(SecBlock<T> &b)
{
	std::swap(size, b.size);
	std::swap(ptr, b.ptr);
}

typedef SecBlock<byte> SecByteBlock;
typedef SecBlock<word> SecWordBlock;

NAMESPACE_END

NAMESPACE_BEGIN(std)
template <class T>
inline void swap(CryptoPP::SecBlock<T> &a, CryptoPP::SecBlock<T> &b)
{
	a.swap(b);
}
NAMESPACE_END

#endif // MISC_H
