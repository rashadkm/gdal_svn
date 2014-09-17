#ifndef _CPLXML_UTILS_H_INCLUDED
#define _CPLXML_UTILS_H_INCLUDED

#include "cpl_minixml.h"

const char* GetCPLXMLNodeTextValue(const CPLXMLNode* psNode)
{
	CPLXMLNode* psChiledNode = psNode->psChild;
	if (psChiledNode == NULL)
		return NULL;
	
	while (psChiledNode != NULL)
	{
		//printf(">>> chiled name: %s, type: %d\n", psChiledNode->pszValue, psChiledNode->eType );
		if (psChiledNode->eType == CXT_Text)
		{
			return psChiledNode->pszValue;
		}
		psChiledNode = psChiledNode->psNext;
	}

	return NULL;
}

void ReadXML(CPLXMLNode* psNode, CPLString szFullName, CPLStringList& szlValues)
{
	if (psNode->eType == 1)
	{
		szFullName[szFullName.size()-1] = '=';
		CPLString value = szFullName + psNode->pszValue;
		szlValues.AddString(value.c_str());
		return;
	}

	CPLXMLNode* psNodeChildren = psNode->psChild;
	if (psNodeChildren != NULL)
		ReadXML(psNodeChildren, szFullName + psNode->pszValue + ".", szlValues);

	
	CPLXMLNode* psNodeNeighbor = psNode->psNext;
	if (psNodeNeighbor != NULL)
		ReadXML(psNodeNeighbor, szFullName, szlValues);

	delete psNodeChildren;
	delete psNodeNeighbor;

	return;
}

void ReadXML(CPLXMLNode* psNode, CPLStringList& szlValues)
{
	ReadXML(psNode->psChild, "", szlValues);
}

#endif /* _CPLXML_UTILS_H_INCLUDED */