// Huffman Compression Class

#include "../common.h"

class CHuffman
{
public:
    static const char* m_sClassName;
    CHuffman() noexcept = default;
    CHuffman(const CHuffman& copy) = delete;
    CHuffman& operator=(const CHuffman& other) = delete;

    static uint Compress(byte* pOutput, const byte* pInput, uint outLen, uint inLen);
};
