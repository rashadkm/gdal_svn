#include "utils.h"

const char* GetCPLXMLNodeTextValue(const CPLXMLNode* psNode)
{
	CPLXMLNode* psChiledNode = psNode->psChild;
	if (psChiledNode == NULL)
		return NULL;
	
	while (psChiledNode != NULL)
	{
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

const char *CPLParseNameTabValue(const char *pszNameValue, char **ppszKey )
{
    int  i;
    const char *pszValue;

    for( i = 0; pszNameValue[i] != '\0'; i++ )
    {
        if( pszNameValue[i] == '\t')
        {
            pszValue = pszNameValue + i + 1;
            while( *pszValue == ' ' || *pszValue == '\t' )
                pszValue++;

            if( ppszKey != NULL )
            {
                *ppszKey = (char *) CPLMalloc(i+1);
                strncpy( *ppszKey, pszNameValue, i );
                (*ppszKey)[i] = '\0';
                while( i > 0 && (*ppszKey)[i] == ' ')
                {
                    (*ppszKey)[i] = '\0';
                    i--;
                }
            }

            return pszValue;
        }
    }

    return NULL;
}
