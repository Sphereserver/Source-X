#include "../../sphere/threads.h"
#include "../CException.h"
#include "sstringobjs.h"

/*
// temporary string storage
#define NO_CONTEXT_TEMPSTRING_MAX_LINES	256
//#define STRINGOBJ_DEFAULT_SIZE 48

struct TemporaryStringUnsafeStateHolder
{
	// NOT thread safe
	size_t m_tempPosition;
    std::unique_ptr<char[]> m_tempStrings;

	static TemporaryStringUnsafeStateHolder& get() {
		static TemporaryStringUnsafeStateHolder instance;
		return instance;
	}

private:
    TemporaryStringUnsafeStateHolder()
    {
        m_tempStrings = std::make_unique<char[]>(NO_CONTEXT_TEMPSTRING_MAX_LINES * THREAD_STRING_LENGTH);
    }
};

[[nodiscard]]
static tchar* getUnsafeStringBuffer() noexcept
{
	auto& unsafe_state_holder = TemporaryStringUnsafeStateHolder::get();
    tchar* unsafe_buffer = &(unsafe_state_holder.m_tempStrings[unsafe_state_holder.m_tempPosition * THREAD_STRING_LENGTH]);
    unsafe_state_holder.m_tempPosition += 1;
    if (unsafe_state_holder.m_tempPosition >= NO_CONTEXT_TEMPSTRING_MAX_LINES)
	{
		unsafe_state_holder.m_tempPosition = 0;
	}
	return unsafe_buffer;
}
*/

[[nodiscard]]
tchar* Str_GetTemp() noexcept
{
    /*
    // To be used if we decide to let each thread have its string buffer pool.
	AbstractThread *pThreadState = ThreadHolder::get().current();
	if (pThreadState)
		return static_cast<AbstractSphereThread*>(pThreadState)->allocateBuffer();
	return getUnsafeStringBuffer();
    */

    // Get from the common but mutex-guarded buffer pool.
    return AbstractSphereThread::Strings::allocateBuffer();
}


/*
 * AbstractString
*/

AbstractString::AbstractString() :
	m_buf(nullptr), m_length(0), m_realLength(0)
{
}

AbstractString::~AbstractString() = default;

void AbstractString::ensureLengthHeap(size_t newLength)
{
	if (newLength >= m_realLength)
	{
		// always grow with 20% extra space to decrease number of future grows
		m_realLength = newLength + newLength / 5;
		char* newBuf = new char[m_realLength + 1];

		if (newBuf == nullptr)
		{
			throw CSError(LOGL_FATAL, 0, "Run out of memory while allocating memory for string");
		}

		if (m_buf != nullptr)
		{
			Str_CopyLimitNull(newBuf, m_buf, m_length);
			delete[] m_buf;
		}
		newBuf[m_length] = 0;
		m_buf = newBuf;
	}
	m_length = newLength;
	m_buf[m_length] = '\0';
}


void AbstractString::destroyHeap() noexcept
{
	if (m_realLength && (m_buf != nullptr))
	{
		delete[] m_buf;
		m_buf = nullptr;
		m_realLength = 0;
		m_length = 0;
	}
}

char AbstractString::charAt(size_t index) const noexcept
{
	return m_buf[index];
}

void AbstractString::setAt(size_t index, char c) noexcept
{
	m_buf[index] = c;
}

void AbstractString::append(const char *s) noexcept
{
	resize(m_length + strlen(s));
	strcat(m_buf, s);
}

void AbstractString::replace(char what, char toWhat) noexcept
{
	for (size_t i = 0; i < m_length; ++i )
	{
		if ( m_buf[i] == what )
		{
			m_buf[i] = toWhat;
		}
	}
}

int AbstractString::compareTo(const char *s) const noexcept
{
	return strcmp(m_buf, s);
}

int AbstractString::compareToIgnoreCase(const char *s) const noexcept
{
	return strcmpi(m_buf, s);
}

bool AbstractString::equals(const char *s) const noexcept
{
	return strcmp(m_buf, s) == 0;
}

bool AbstractString::equalsIgnoreCase(const char *s) const noexcept
{
	return strcmpi(m_buf, s) == 0;
}

bool AbstractString::startsWith(const char *s) const noexcept
{
	return (strnicmp(m_buf, s, strlen(s)) == 0);
}

bool AbstractString::startsWithHead(const char *s) const noexcept
{
	for ( int i = 0; ; ++i )
	{
		char ch1 = (uchar)(tolower(m_buf[i]));
		char ch2 = (uchar)(tolower(s[i]));
		if( ch2 == '\0' )
		{
			if( !isalnum(ch1) )
				return true;
			return false;
		}
		if ( ch1 != ch2 )
			return false;
	}
}

size_t AbstractString::indexOf(char c) const noexcept
{
	char *pos = strchr(m_buf, c);
	return (size_t)(( pos == nullptr ) ? -1 : pos - m_buf);
}

size_t AbstractString::indexOf(const char *s) const noexcept
{
	char *pos = strstr(m_buf, s);
	return (size_t)((pos == nullptr) ? -1 : pos - m_buf);
}

size_t AbstractString::lastIndexOf(char c) const noexcept
{
	char *pos = strrchr(m_buf, c);
	return (size_t)((pos == nullptr) ? -1 : pos - m_buf);
}


/*
 * HeapString
*/

/*
HeapString::HeapString()
{
    resize(STRINGOBJ_DEFAULT_SIZE);
}

HeapString::~HeapString()
{
    destroyHeap();
}

void HeapString::resize(size_t newLength)
{
    ensureLengthHeap(newLength);
}
*/


/*
 * TemporaryString
*/


TemporaryString::TemporaryString() //:
	//m_useHeap(false), m_state(nullptr)
{
    /*
    // To be used if we decide to let each thread have its string buffer pool.
    AbstractSphereThread *current = static_cast<AbstractSphereThread*> (ThreadHolder::get().current());
    if ( current != nullptr )
    {
        // allocate from thread context
        current->Strings::getBuffer(*this);
    }
    else
    {
        // allocate from global, thread-UNsafe buffer when context is not available.
        tchar* unsafe_buffer = getUnsafeStringBuffer();
        init(unsafe_buffer, nullptr);
    }
    */

    // Get from the common but mutex-guarded buffer pool.

    AbstractSphereThread::Strings::getBufferForStringObject(*this);

	// At this point, both m_useHeap and m_state should be initialized.
}

TemporaryString::TemporaryString(char *buffer, char *state)
{
	init(buffer, state);
}

TemporaryString::TemporaryString(lpctstr pStr) : TemporaryString()
{
	m_length = Str_CopyLimitNull(m_buf, pStr, m_realLength);
}

TemporaryString::TemporaryString(lpctstr pStr, size_t uiLen) : TemporaryString()
{
	m_length = Str_CopyLimitNull(m_buf, pStr, minimum(uiLen, m_realLength));
}

TemporaryString::~TemporaryString()
{
	if (m_useHeap)
	{
		destroyHeap();
	}
	else
	{
		if (m_state != nullptr)
		{
			*m_state = '\0';
		}
	}
}

void TemporaryString::init(char *buffer, char *state) noexcept
{
	m_useHeap = false;
	m_buf = buffer;
	m_state = state;
	if( m_state != nullptr )
	{
		*m_state = 'U';
		m_realLength = THREAD_STRING_LENGTH;
	}
	else
	{
		m_realLength = THREAD_STRING_LENGTH;
	}
	m_length = 0;
}

void TemporaryString::resize(size_t newLength)
{
	if (m_useHeap)
	{
		ensureLengthHeap(newLength);
		return;
	}

    if (newLength >= m_realLength)
    {
        // switch back to behaving like a normal string, since the thread context does not have the
        // capacity we need. To accomplish this we:
        // 1. create a new buffer with the desired length (+20%)
        // 2. copy the old buffer content to the new buffer
        // 3. replace the old buffer with the new buffer
        // 4. clear the state to allow the old buffer to be used elsewhere
        // 5. flag this string instance to use the heap (String::)

        m_realLength = newLength + newLength / 5;
        char* newBuf = new char[m_realLength + 1];
        if (newBuf == nullptr)
            throw CSError(LOGL_FATAL, 0, "Run out of memory while allocating memory for string");

        Str_CopyLimitNull(newBuf, m_buf, m_length);
        newBuf[m_length] = '\0';

        m_buf = newBuf;
        if (m_state != nullptr)
        {
            *m_state = '\0';
            m_state = nullptr;
        }

        m_useHeap = true;
    }

    m_length = newLength;
    m_buf[m_length] = '\0';
}
