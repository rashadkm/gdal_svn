#ifndef _CPLXML_UTILS_H_INCLUDED
#define _CPLXML_UTILS_H_INCLUDED

#include "cpl_minixml.h"

#include "cpl_string.h"

const char* GetCPLXMLNodeTextValue(const CPLXMLNode* psNode);

void ReadXML(CPLXMLNode* psNode, CPLString szFullName, CPLStringList& szlValues);

void ReadXML(CPLXMLNode* psNode, CPLStringList& szlValues);

#endif /* _CPLXML_UTILS_H_INCLUDED */