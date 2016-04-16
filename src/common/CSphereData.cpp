
#include "../sphere/threads.h"
#include "../game/uo_files/CUOMultiItemRec.h"
#include "../game/uo_files/CUOVersionBlock.h"
#include "./sphere_library/CFile.h"
#include "CException.h"
#include "CUOInstall.h"
#include "CSphereMap.h"
#include "common.h"
#include "spherecom.h"

//////////////////////////////////////////////////////////////////////
// -CVerDataMul

CVerDataMul::CVerDataMul()
{

}

CVerDataMul::~CVerDataMul()
{
	Unload();
}

int CVerDataMul::QCompare( size_t left, dword dwRefIndex ) const
{
	dword dwIndex2 = GetEntry(left)->GetIndex();
	return( dwIndex2 - dwRefIndex );
}

void CVerDataMul::QSort( size_t left, size_t right )
{
	ADDTOCALLSTACK("CVerDataMul::QSort");
	static int iReentrant = 0;
	ASSERT( left <= right );
	size_t j = left;
	size_t i = right;

	dword dwRefIndex = GetEntry( (left + right) / 2 )->GetIndex();

	do
	{
		while ( j < m_Data.GetCount() && QCompare( j, dwRefIndex ) < 0 )
			j++;
		while ( i > 0 && QCompare( i, dwRefIndex ) > 0 )
			i--;

		if ( i >= j )
		{
			if ( i != j )
			{
				CUOVersionBlock Tmp = m_Data.GetAt(i);
				CUOVersionBlock block = m_Data.GetAt(j);
				m_Data.SetAt( i, block );
				m_Data.SetAt( j, Tmp );
			}

			if ( i > 0 )
				i--;
			if ( j < m_Data.GetCount() )
				j++;
		}

	} while (j <= i);

	iReentrant++;
	if (left < i)
		QSort(left, i);
	if (j < right)
		QSort(j, right);
	iReentrant--;
}

void CVerDataMul::Load( CGFile & file )
{
	ADDTOCALLSTACK("CVerDataMul::Load");
	// assume it is in sorted order.
	if ( GetCount())	// already loaded.
	{
		return;
	}

// #define g_fVerData g_Install.m_File[VERFILE_VERDATA]

	if ( ! file.IsFileOpen())		// T2a might not have this.
		return;

	file.SeekToBegin();
	dword dwQty;
	if ( file.Read(static_cast<void *>(&dwQty), sizeof(dwQty)) <= 0 )
	{
		throw CSphereError( LOGL_CRIT, CGFile::GetLastError(), "VerData: Read Qty");
	}

	Unload();
	m_Data.SetCount( dwQty );

	if ( file.Read(static_cast<void *>(m_Data.GetBasePtr()), dwQty * sizeof( CUOVersionBlock )) <= 0 )
	{
		throw CSphereError( LOGL_CRIT, CGFile::GetLastError(), "VerData: Read");
	}

	if ( dwQty <= 0 )
		return;

	// Now sort it for fast searching.
	// Make sure it is sorted.
	QSort( 0, dwQty - 1 );

#ifdef _DEBUG
	for ( size_t i = 0; i < (dwQty - 1); i++ )
	{
		dword dwIndex1 = GetEntry(i)->GetIndex();
		dword dwIndex2 = GetEntry(i + 1)->GetIndex();
		if ( dwIndex1 > dwIndex2 )
		{
			DEBUG_ERR(( "VerData Array is NOT sorted !\n" ));
			throw CSphereError( LOGL_CRIT, -1, "VerData: NOT Sorted!");
		}
	}
#endif

}

size_t CVerDataMul::GetCount() const
{
	return( m_Data.GetCount());
}

const CUOVersionBlock * CVerDataMul::GetEntry( size_t i ) const
{
	return( &m_Data.ElementAt(i));
}

void CVerDataMul::Unload()
{
	m_Data.Empty();
}

bool CVerDataMul::FindVerDataBlock( VERFILE_TYPE type, dword id, CUOIndexRec & Index ) const
{
	ADDTOCALLSTACK("CVerDataMul::FindVerDataBlock");
	// Search the verdata.mul for changes to stuff.
	// search for a specific block.
	// assume it is in sorted order.

	int iHigh = (int)(GetCount())-1;
	if ( iHigh < 0 )
	{
		return false;
	}

	dword dwIndex = VERDATA_MAKE_INDEX(type,id);
	const CUOVersionBlock * pArray = m_Data.GetBasePtr();
	int iLow = 0;
	while ( iLow <= iHigh )
	{
		int i = (iHigh+iLow)/2;
		dword dwIndex2 = pArray[i].GetIndex();
		int iCompare = dwIndex - dwIndex2;
		if ( iCompare == 0 )
		{
			Index.CopyIndex( &(pArray[i]));
			return true;
		}
		if ( iCompare > 0 )
		{
			iLow = i+1;
		}
		else
		{
			iHigh = i-1;
		}
	}
	return false;
}


//*********************************************8
// -CSphereMulti

size_t CSphereMulti::Load( MULTI_TYPE id )
{
	ADDTOCALLSTACK("CSphereMulti::Load");
	// Just load the whole thing.

	Release();
	InitCacheTime();		// This is invalid !

	if ( id < 0 || id >= MULTI_QTY )
		return 0;
	m_id = id;

	CUOIndexRec Index;
	if ( ! g_Install.ReadMulIndex( VERFILE_MULTIIDX, VERFILE_MULTI, id, Index ))
		return 0;

	switch ( g_Install.GetMulFormat( VERFILE_MULTIIDX ) )
	{
		case VERFORMAT_HIGHSEAS: // high seas multi format (CUOMultiItemRec_HS)
			m_iItemQty = Index.GetBlockLength() / sizeof(CUOMultiItemRec_HS);
			m_pItems = new CUOMultiItemRec_HS [ m_iItemQty ];
			ASSERT( m_pItems );

			ASSERT( (sizeof(m_pItems[0]) * m_iItemQty) >= Index.GetBlockLength() );
			if ( ! g_Install.ReadMulData( VERFILE_MULTI, Index, static_cast <CUOMultiItemRec_HS *>(m_pItems) ))
				return 0;
			break;

		case VERFORMAT_ORIGINAL: // old format (CUOMultiItemRec)
		default:
			m_iItemQty = Index.GetBlockLength() / sizeof(CUOMultiItemRec);
			m_pItems = new CUOMultiItemRec_HS [ m_iItemQty ];
			ASSERT( m_pItems );

			CUOMultiItemRec* pItems = new CUOMultiItemRec[m_iItemQty];
			ASSERT( (sizeof(pItems[0]) * m_iItemQty) >= Index.GetBlockLength() );
			if ( ! g_Install.ReadMulData( VERFILE_MULTI, Index, static_cast <CUOMultiItemRec *>(pItems) ))
			{
				delete[] pItems;
				return 0;
			}

			// copy to new format
			for (size_t i = 0; i < m_iItemQty; i++)
			{
				m_pItems[i].m_wTileID = pItems[i].m_wTileID;
				m_pItems[i].m_dx = pItems[i].m_dx;
				m_pItems[i].m_dy = pItems[i].m_dy;
				m_pItems[i].m_dz = pItems[i].m_dz;
				m_pItems[i].m_visible = pItems[i].m_visible;
				m_pItems[i].m_unknown = 0;
			}

			delete[] pItems;
			break;
	}

	HitCacheTime();
	return( m_iItemQty );
}
