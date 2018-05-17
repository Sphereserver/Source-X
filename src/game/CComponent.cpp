
#include "CComponent.h"
#include "CObjBase.h"

CComponent::CComponent(COMP_TYPE type, CObjBase *pLink)
{
    _iType = type;
    _pLink = pLink;
}

COMP_TYPE CComponent::GetType() const
{
    return _iType;
}

CObjBase * CComponent::GetLink() const
{
    return _pLink;
}
