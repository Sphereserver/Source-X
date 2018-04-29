
#include "CComponent.h"
#include "CObjBase.h"

CComponent::CComponent(COMP_TYPE type, const CObjBase *pLink)
{
    _iType = type;
    _pLink = pLink;
}

COMP_TYPE CComponent::GetType()
{
    return _iType;
}

const CObjBase *CComponent::GetLink()
{
    return _pLink;
}
