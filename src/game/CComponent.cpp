
#include "CComponent.h"
#include "CObjBase.h"

CComponent::CComponent(COMP_TYPE type) : 
    _iType(type)
{
}

COMP_TYPE CComponent::GetType() const
{
    return _iType;
}

