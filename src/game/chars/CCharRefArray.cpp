
#include "../../sphere/threads.h"
#include "CChar.h"
#include "CCharRefArray.h"

//*****************************************************************
// -CCharRefArray

size_t CCharRefArray::FindChar( const CChar *pChar ) const
{
    ADDTOCALLSTACK("CCharRefArray::FindChar");
    if ( !pChar )
        return m_uidCharArray.BadIndex();

    CUID uid(pChar->GetUID());
    size_t iQty = m_uidCharArray.GetCount();
    for ( size_t i = 0; i < iQty; i++ )
    {
        if ( uid == m_uidCharArray[i] )
            return i;
    }
    return m_uidCharArray.BadIndex();
}

bool CCharRefArray::IsCharIn( const CChar * pChar ) const
{
    return( FindChar( pChar ) != m_uidCharArray.BadIndex() );
}

size_t CCharRefArray::AttachChar( const CChar *pChar )
{
    ADDTOCALLSTACK("CCharRefArray::AttachChar");
    size_t i = FindChar(pChar);
    if ( i != m_uidCharArray.BadIndex() )
        return i;
    return m_uidCharArray.Add(pChar->GetUID());
}

size_t CCharRefArray::InsertChar( const CChar *pChar, size_t i )
{
    ADDTOCALLSTACK("CCharRefArray::InsertChar");
    size_t currentIndex = FindChar(pChar);
    if ( currentIndex != m_uidCharArray.BadIndex() )
    {
        if ( currentIndex == i )	// already there
            return i;
        DetachChar(currentIndex);	// remove from list
    }

    if ( !IsValidIndex(i) )		// prevent from being inserted too high
        i = GetCharCount();

    m_uidCharArray.InsertAt(i, pChar->GetUID() );
    return i;
}

void CCharRefArray::DetachChar( size_t i )
{
    ADDTOCALLSTACK("CCharRefArray::DetachChar");
    m_uidCharArray.RemoveAt(i);
}

size_t CCharRefArray::DetachChar( const CChar *pChar )
{
    ADDTOCALLSTACK("CCharRefArray::DetachChar");
    size_t i = FindChar(pChar);
    if ( i != m_uidCharArray.BadIndex() )
        DetachChar(i);
    return i;
}

void CCharRefArray::DeleteChars()
{
    ADDTOCALLSTACK("CCharRefArray::DeleteChars");
    size_t iQty = m_uidCharArray.GetCount();
    while ( iQty > 0 )
    {
        CChar *pChar = m_uidCharArray[--iQty].CharFind();
        if ( pChar )
            pChar->Delete();
    }
    m_uidCharArray.Clear();
}


void CCharRefArray::WritePartyChars( CScript &s )
{
    ADDTOCALLSTACK("CCharRefArray::WritePartyChars");
    size_t iQty = m_uidCharArray.GetCount();
    for ( size_t j = 0; j < iQty; j++ )		// write out links to all my chars
        s.WriteKeyHex("CHARUID", m_uidCharArray[j]);
}

