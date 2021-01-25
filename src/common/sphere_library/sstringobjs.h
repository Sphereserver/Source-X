/**
* @file sstringobjs.h
*
*/

#ifndef _INC_SSTRINGOBJS_H
#define _INC_SSTRINGOBJS_H

#include "../common.h"

// temporary string storage
#define THREAD_STRING_STORAGE	4096
#define THREAD_TSTRING_STORAGE	2048
#define THREAD_STRING_LENGTH	4096


// Base abstract class for strings, provides basic information of what should be available
// NOTE: Children should override destroy()
class AbstractString
{
public:
	AbstractString();
	virtual ~AbstractString() = default;

private:
	AbstractString(const AbstractString& copy);
	AbstractString& operator=(const AbstractString& other);

public:
	// information
    inline size_t size() const noexcept {
        return m_length;
    }
    inline size_t capacity() const noexcept {
        return m_realLength;
    }
    inline bool empty() const noexcept {
        return m_length != 0;
    }
    inline char *buffer() noexcept {
        return m_buf;
    }
	inline const char* buffer() const noexcept {
		return m_buf;
	}

	// character operations
	char charAt(size_t index) const noexcept;
	void setAt(size_t index, char c) noexcept;

	// modification
	void append(const char *s) noexcept;
	void replace(char what, char toWhat) noexcept;

	// comparison
	int compareTo(const char *s) const noexcept;				// compare with
	int compareToIgnoreCase(const char *s) const noexcept;	// compare with [case ignored]
	bool equals(const char *s) const noexcept;				// equals?
	bool equalsIgnoreCase(const char *s) const noexcept;		// equals? [case ignored]
	bool startsWith(const char *s) const noexcept;			// starts with this? [case ignored]
	bool startsWithHead(const char *s) const noexcept;		// starts with this and separator [case ignored]

	// search
	size_t indexOf(char c) const noexcept;
	size_t indexOf(const char *s) const noexcept;
	size_t lastIndexOf(char c) const noexcept;

	// operator
	inline operator lpctstr() const noexcept {      // as a C string
        return m_buf;
    }
	/* // Those dangerous casts need to be explicit!
	inline operator const char*&() noexcept {		// as a C string
        return const_cast<const char*&>(m_buf);
    } */

protected:
	// not implemented, should take care that newLength should fit in the buffer
	virtual void resize(size_t newLength) = 0;

	void ensureLengthHeap(size_t newLength);
	void destroyHeap() noexcept;

protected:
	char	*m_buf;
	size_t	m_length;       // length of the string (which is the part of the buffer we're actually using)
	size_t	m_realLength;   // length of the buffer
};


// Common string implementation, implementing AbstractString and working on heap
//	It's redundant, since we have a more feature-complete CSString
/*
class HeapString : public AbstractString
{
public:
	HeapString();
	virtual ~HeapString();

	HeapString(const HeapString& copy) = delete;
	HeapString& operator=(const HeapString& other) = delete;

protected:
	virtual void resize(size_t newLength) override;
};
*/


#define MAX_TEMP_LINES_NO_CONTEXT	512

// Temporary string implementation. Works with thread-safe string
// To create such string:
// TemporaryString str;
// it could be also created via new TemporaryString but whats the point if we still use memory allocation? :)
class TemporaryString : public AbstractString
{
public:
	TemporaryString();
	TemporaryString(char *buffer, char *state);
	~TemporaryString();

	TemporaryString(const TemporaryString& copy) = delete;
	TemporaryString& operator=(const TemporaryString& other) = delete;

protected:
	virtual void resize(size_t newLength) override;

	// should not really be used, made for use of AbstractSphereThread *only*
	friend class AbstractSphereThread;
	void init(char* buffer, char* state) noexcept;

private:
	bool m_useHeap;					// a mark whatever we are in heap (String) or stack (ThreadLocal) mode
	char *m_state;					// a pointer to thread local state of the line we occupy

	// static buffer to allow similar operations for non-threaded environment
	// NOTE: this buffer have no protection against overrun, so beware
	static size_t m_tempPosition;
	static char m_tempStrings[MAX_TEMP_LINES_NO_CONTEXT][THREAD_STRING_LENGTH];
};

#endif // _INC_SSTRINGOBJS_H
