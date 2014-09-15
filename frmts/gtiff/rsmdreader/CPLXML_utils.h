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

#endif /* _CPLXML_UTILS_H_INCLUDED */