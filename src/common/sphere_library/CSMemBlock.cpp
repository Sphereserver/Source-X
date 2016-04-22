#include "CSMemBlock.h"

// CSMemBlock:: Constructors, Destructor, Asign operator.

CSMemBlock::CSMemBlock()
{
    m_pData = NULL;
}

CSMemBlock::~CSMemBlock()
{
    Free();
}

// CSMemBlock:: Modifiers.

void CSMemBlock::Alloc( size_t dwSize )
{
    Free();
    if ( dwSize > 0 )
    {
        m_pData = AllocBase(dwSize);
    }
}

byte * CSMemBlock::AllocBase( size_t dwSize )
{
    ASSERT(dwSize > 0);
    byte * pData = new byte[ dwSize ];
    ASSERT( pData != NULL );
    return( pData );
}

void CSMemBlock::Free()
{
    if ( m_pData != NULL )
    {
        delete[] m_pData;
        m_pData = NULL;
    }
}

void CSMemBlock::MemLink( byte * pData )
{
    ASSERT( m_pData == NULL );
    m_pData = pData;
}

// CSMemBlock:: Operations.

byte * CSMemBlock::GetData() const
{
    return( m_pData );
}

// CSMemLenBlock:: Constructors, Destructor, Asign operator.

CSMemLenBlock::CSMemLenBlock()
{
    m_dwLength = 0;
}

// CSMemLenBlock:: Modifiers.

void CSMemLenBlock::Alloc( size_t dwSize )
{
    m_dwLength = dwSize;
    CSMemBlock::Alloc(dwSize);
}

void CSMemLenBlock::Free()
{
    m_dwLength = 0;
    CSMemBlock::Free();
}

void CSMemLenBlock::Resize( size_t dwSizeNew )
{
    ASSERT( dwSizeNew != m_dwLength );
    byte * pDataNew = AllocBase( dwSizeNew );
    ASSERT(pDataNew);
    if ( GetData())
    {
        // move any existing data.
        ASSERT(m_dwLength);
        memcpy( pDataNew, GetData(), minimum( dwSizeNew, m_dwLength ));
        CSMemBlock::Free();
    }
    else
    {
        ASSERT(!m_dwLength);
    }
    m_dwLength = dwSizeNew;
    MemLink( pDataNew );
}

// CSMemLenBlock:: Operations.

size_t CSMemLenBlock::GetDataLength() const
{
    return( m_dwLength );
}
