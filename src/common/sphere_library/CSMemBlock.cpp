#include <cstring>
#include "CSMemBlock.h"

// CSMemBlock:: Constructors, Destructor, Asign operator.

CSMemBlock::CSMemBlock()
{
    m_pData = nullptr;
}

CSMemBlock::~CSMemBlock()
{
    Free();
}

// CSMemBlock:: Modifiers.

void CSMemBlock::Alloc( size_t uiSize )
{
    Free();
    ASSERT(m_pData == nullptr);
    if ( uiSize > 0 )
        m_pData = AllocBase(uiSize);
}

byte * CSMemBlock::AllocBase( size_t uiSize )  // Static
{
    ASSERT(uiSize > 0);
    byte * pData = new byte[ uiSize ]; //();
    ASSERT( pData != nullptr );
    return( pData );
}

void CSMemBlock::Free()
{
    if ( m_pData != nullptr )
    {
        delete[] m_pData;
        m_pData = nullptr;
    }
}

void CSMemBlock::MemLink( byte * pData )
{
    ASSERT( m_pData == nullptr );
    m_pData = pData;
}


// CSMemLenBlock:: Constructors, Destructor, Asign operator.

CSMemLenBlock::CSMemLenBlock() :
    m_uiLength(0)
{
}

CSMemLenBlock::~CSMemLenBlock() = default;

// CSMemLenBlock:: Modifiers.

void CSMemLenBlock::Alloc( size_t uiSize )
{
    m_uiLength = uiSize;
    CSMemBlock::Alloc(uiSize);
}

void CSMemLenBlock::Free()
{
    m_uiLength = 0;
    CSMemBlock::Free();
}

void CSMemLenBlock::Resize( size_t uiSizeNew )
{
    ASSERT( uiSizeNew != m_uiLength );
    byte * pDataNew = AllocBase( uiSizeNew );
    ASSERT(pDataNew);
    if ( GetData())
    {
        // move any existing data.
        ASSERT(m_uiLength);
        memcpy( pDataNew, GetData(), minimum( uiSizeNew, m_uiLength ));
        CSMemBlock::Free();
    }
    else
    {
        ASSERT(!m_uiLength);
    }
    m_uiLength = uiSizeNew;
    MemLink( pDataNew );
}

