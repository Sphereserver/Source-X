 /**
 * @file CUID.h
 * @brief MD5 hashing.
 */

 /*
 This code has been built from scratch using rfc1321 and Colin Plumb's
 public domain code.
 */

#ifndef _INC_CMD5_H 
#define _INC_CMD5_H

#include "../datatypes.h"
#include <cstring>

class CMD5
{
private:
	uint m_buffer[4];
	uint m_bits[2];
	uchar m_input[64];
	bool m_finalized;

	void private_update() noexcept;

public:
	CMD5() noexcept;
    ~CMD5() noexcept = default;
    CMD5(const CMD5& copy) = delete;
    CMD5& operator=(const CMD5& other) = delete;

	void reset() noexcept;
	void update( const uchar * data, uint length ) noexcept;
	void finalize() noexcept;

	// Digest has to be 33 bytes long
	void digest( char * digest ) noexcept;
	// Get digest in a "numeric" form to be usable
	void numericDigest( uchar * digest ) noexcept;
	
    static void fastDigest(char * digest, const char * message) noexcept;
};

#endif // _INC_CMD5_H
