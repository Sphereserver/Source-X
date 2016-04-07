#include "CMemBlock.h"

// CMemBlock:: Constructors, Destructor, Asign operator.

CMemBlock::CMemBlock()
{
    m_pData = NULL;
}

CMemBlock::~CMemBlock()
{
    Free();
}

// CMemBlock:: Modifiers.

void CMemBlock::Alloc( size_t dwSize )
{
    Free();
    if ( dwSize > 0 )
    {
        m_pData = AllocBase(dwSize);
    }
}

byte * CMemBlock::AllocBase( size_t dwSize )
{
    ASSERT(dwSize > 0);
    byte * pData = new byte[ dwSize ];
    ASSERT( pData != NULL );
    return( pData );
}

void CMemBlock::Free()
{
    if ( m_pData != NULL )
    {
        delete[] m_pData;
        m_pData = NULL;
    }
}

void CMemBlock::MemLink( byte * pData )
{
    ASSERT( m_pData == NULL );
    m_pData = pData;
}

// CMemBlock:: Operations.

byte * CMemBlock::GetData() const
{
    return( m_pData );
}

// CMemLenBlock:: Constructors, Destructor, Asign operator.

CMemLenBlock::CMemLenBlock()
{
    m_dwLength = 0;
}

// CMemLenBlock:: Modifiers.

void CMemLenBlock::Alloc( size_t dwSize )
{
    m_dwLength = dwSize;
    CMemBlock::Alloc(dwSize);
}

void CMemLenBlock::Free()
{
    m_dwLength = 0;
    CMemBlock::Free();
}

void CMemLenBlock::Resize( size_t dwSizeNew )
{
    ASSERT( dwSizeNew != m_dwLength );
    byte * pDataNew = AllocBase( dwSizeNew );
    ASSERT(pDataNew);
    if ( GetData())
    {
        // move any existing data.
        ASSERT(m_dwLength);
        memcpy( pDataNew, GetData(), minimum( dwSizeNew, m_dwLength ));
        CMemBlock::Free();
    }
    else
    {
        ASSERT(!m_dwLength);
    }
    m_dwLength = dwSizeNew;
    MemLink( pDataNew );
}

// CMemLenBlock:: Operations.

size_t CMemLenBlock::GetDataLength() const
{
    return( m_dwLength );
}
