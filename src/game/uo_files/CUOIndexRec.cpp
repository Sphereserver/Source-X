#include "CUOIndexRec.h"
#include "CUOVersionBlock.h"


// CUOIndexRec:: Modifiers.

void CUOIndexRec::Init()
{
    m_dwLength = 0;
}

void CUOIndexRec::CopyIndex( const CUOVersionBlock * pVerData )
{
    // Get an index rec from the verdata rec.
    m_dwOffset = pVerData->m_filepos;
    m_dwLength = pVerData->m_length | 0x80000000;
    m_wVal3 = pVerData->m_wVal3;
    m_wVal4 = pVerData->m_wVal4;
}

void CUOIndexRec::SetupIndex( dword dwOffset, dword dwLength )
{
    m_dwOffset = dwOffset;
    m_dwLength = dwLength;
}

// CUOIndexRec:: Operations.

dword CUOIndexRec::GetFileOffset() const
{
    return ( m_dwOffset );
}

dword CUOIndexRec::GetBlockLength() const
{
    return ( m_dwLength &~ 0x80000000 );
}

bool CUOIndexRec::IsVerData() const
{
    return ( m_dwLength & 0x80000000 ) != 0;
}

bool CUOIndexRec::HasData() const
{
    return ( m_dwOffset != 0xFFFFFFFF && m_dwLength != 0 );
}
