#ifndef _CPLXML_UTILS_H_INCLUDED
#define _CPLXML_UTILS_H_INCLUDED

#include "cpl_minixml.h"

#include "cpl_string.h"

const char* GetCPLXMLNodeTextValue(const CPLXMLNode* psNode);

void ReadXML(CPLXMLNode* psNode, CPLString szFullName, CPLStringList& szlValues);

void ReadXML(CPLXMLNode* psNode, CPLStringList& szlValues);

const char *CPLParseNameTabValue(const char *pszNameValue, char **ppszKey );

#endif /* _CPLXML_UTILS_H_INCLUDED */