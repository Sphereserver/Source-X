
#include "CComponent.h"
#include "CObjBase.h"

CComponent::CComponent(COMP_TYPE type) noexcept :
    _iType(type)
{
}

COMP_TYPE CComponent::GetType() const noexcept
{
    return _iType;
}

