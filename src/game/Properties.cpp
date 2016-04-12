
#include "../sphere/threads.h"
#include "Properties.h"


char NewProperties::GetResPhysical()
{
	ADDTOCALLSTACK("NewProperties::GetResPhysical");
	return m_iResPhysical;
}

void NewProperties::SetResPhysical( char iVal )
{
	ADDTOCALLSTACK("NewProperties::SetResPhysical");
	m_iResPhysical = iVal;
}

char NewProperties::GetResFire()
{
	ADDTOCALLSTACK("NewProperties::GetResFire");
	return m_iResFire;
}

void NewProperties::SetResFire(char iVal)
{
	ADDTOCALLSTACK("NewProperties::SetResFire");
	m_iResFire = iVal;
}

char NewProperties::GetResCold()
{
	ADDTOCALLSTACK("NewProperties::GetResCold");
	return m_iResCold;
}

void NewProperties::SetResCold(char iVal)
{
	ADDTOCALLSTACK("NewProperties::SetResCold");
	m_iResCold = iVal;
}

char NewProperties::GetResPoison()
{
	ADDTOCALLSTACK("NewProperties::GetResPoison");
	return m_iResPoison;
}

void NewProperties::SetResPoison(char iVal)
{
	ADDTOCALLSTACK("NewProperties::SetResPoison");
	m_iResPoison = iVal;
}

char NewProperties::GetResEnergy()
{
	ADDTOCALLSTACK("NewProperties::GetResEnergy");
	return m_iResEnergy;
}

void NewProperties::SetResEnergy(char iVal)
{
	ADDTOCALLSTACK("NewProperties::SetResEnergy");
	m_iResEnergy = iVal;
}

char NewProperties::GetResPhysicalMax()
{
	ADDTOCALLSTACK("NewProperties::GetResPhysicalMax");
	return m_iResPhysicalMax;
}

void NewProperties::SetResPhysicalMax( char iVal )
{
	ADDTOCALLSTACK("NewProperties::SetResPhysicalMax");
	m_iResPhysicalMax = iVal;
}

char NewProperties::GetResFireMax()
{
	ADDTOCALLSTACK("NewProperties::GetResFireMax");
	return m_iResFireMax;
}

void NewProperties::SetResFireMax(char iVal)
{
	ADDTOCALLSTACK("NewProperties::SetResFireMax");
	m_iResFireMax = iVal;
}

char NewProperties::GetResColdMax()
{
	ADDTOCALLSTACK("NewProperties::GetResColdMax");
	return m_iResColdMax;
}

void NewProperties::SetResColdMax(char iVal)
{
	ADDTOCALLSTACK("NewProperties::SetResColdMax");
	m_iResColdMax = iVal;
}

char NewProperties::GetResPoisonMax()
{
	ADDTOCALLSTACK("NewProperties::GetResPoisonMax");
	return m_iResPoisonMax;
}

void NewProperties::SetResPoisonMax(char iVal)
{
	ADDTOCALLSTACK("NewProperties::SetResPoisonMax");
	m_iResPoisonMax = iVal;
}

char NewProperties::GetResEnergyMax()
{
	ADDTOCALLSTACK("NewProperties::GetResEnergyMax");
	return m_iResEnergyMax;
}

void NewProperties::SetResEnergyMax(char iVal)
{
	ADDTOCALLSTACK("NewProperties::SetResEnergyMax");
	m_iResEnergyMax = iVal;
}
