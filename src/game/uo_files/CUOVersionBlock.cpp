#include "CUOVersionBlock.h"

// CUOVersionBlock:: File ops.

VERFILE_TYPE CUOVersionBlock::GetFileIndex() const
{
    return( static_cast<VERFILE_TYPE>(m_file) );
}

void CUOVersionBlock::SetFile(VERFILE_TYPE dwFile)
{
    m_file = dwFile;
}

// CUOVersionBlock:: Block ops.

dword CUOVersionBlock::GetIndex() const
{
    return( VERDATA_MAKE_INDEX( m_file, m_block ));
}

dword CUOVersionBlock::GetBlockIndex() const
{
    return( m_block );
}

void CUOVersionBlock::SetBlock(dword dwBlock)
{
    m_block = dwBlock;
}
