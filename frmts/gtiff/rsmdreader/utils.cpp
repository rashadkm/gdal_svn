/******************************************************************************
 * $Id$
 *
 * Project:  RSMDReader - Remote Sensing MetaData Reader
 * Purpose:  Read remote sensing metadata from files from different providers like as DigitalGlobe, GeoEye et al.
 * Author:   Alexander Lisovenko
 *
 ******************************************************************************
 * Copyright (c) 2014 NextGIS <info@nextgis.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include <time.h>

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

void ReadXML(CPLXMLNode* psNode, CPLString szFullName, CPLStringList& szlValues, const CPLString& osRootNodeName, const CPLStringList& expulsionNodeNames)
{
	if (psNode->eType == CXT_Element)
	{
		if( expulsionNodeNames.FindString(psNode->pszValue) != -1 )
			return;
	}

	if (psNode->eType == CXT_Text)
	{
		if( szFullName.size() != 0)
		{
			szFullName[szFullName.size()-1] = '=';
			CPLString value = szFullName + psNode->pszValue;
			szlValues.AddString(value.c_str());
		}
		else
		{
			CPLString value = osRootNodeName + "=" + psNode->pszValue;
			szlValues.AddString(value.c_str());
		}
	}

	CPLXMLNode* psNodeChildren = psNode->psChild;
	if (psNodeChildren != NULL)
		ReadXML(psNodeChildren, szFullName + psNode->pszValue + ".", szlValues, osRootNodeName, expulsionNodeNames);

	
	CPLXMLNode* psNodeNeighbor = psNode->psNext;
	if (psNodeNeighbor != NULL)
		ReadXML(psNodeNeighbor, szFullName, szlValues, osRootNodeName, expulsionNodeNames);

	delete psNodeChildren;
	delete psNodeNeighbor;

	return;
}

void ReadXMLToStringList(CPLXMLNode* psNode, const CPLStringList& expulsionNodeNames, CPLStringList& szlValues)
{
	if (psNode != NULL)
		ReadXML(psNode->psChild, 
		"", 
		szlValues, 
		CPLString(psNode->pszValue), 
		expulsionNodeNames);
}

const char *CPLGoodParseNameValue(const char *pszNameValue, char **ppszKey, const char separator)
{
    const char *pszValue;

	size_t iNameValueSymbol = 0;

	while( pszNameValue[iNameValueSymbol] == ' ' || 
			pszNameValue[iNameValueSymbol] == '\t' )
		iNameValueSymbol++;

	if( pszNameValue[iNameValueSymbol] == separator )
		return NULL;

	size_t iFirstSignificantSimbol = iNameValueSymbol;

    for( iNameValueSymbol; pszNameValue[iNameValueSymbol] != '\0'; iNameValueSymbol++ )
    {
        if( pszNameValue[iNameValueSymbol] == separator )
        {
            pszValue = pszNameValue + iNameValueSymbol + 1;

            while( *pszValue == ' ' || *pszValue == '\t' )
                pszValue++;

            if( ppszKey != NULL )
            {
				size_t iKey = iNameValueSymbol;
				while( pszNameValue[iKey-1] == ' ' || pszNameValue[iKey-1] == '\t' )
					iKey--;

                *ppszKey = (char *) CPLMalloc(iKey - iFirstSignificantSimbol + 1);
                strncpy( *ppszKey, pszNameValue + iFirstSignificantSimbol, iKey - iFirstSignificantSimbol );
                (*ppszKey)[iKey - iFirstSignificantSimbol] = '\0';
            }

            return pszValue;
        }
    }

    return NULL;
}

bool GetAcqisitionTime(const CPLString& rsAcqisitionStartTime, const CPLString& rsAcqisitionEndTime, const CPLString& osDateTimeTemplate, CPLString& osAcqisitionTime)
{
		size_t iYearStart;
		size_t iYearEnd;

		size_t iMonthStart;
		size_t iMonthEnd;

		size_t iDayStart;
		size_t iDayEnd;
		
		size_t iHoursStart;
		size_t iHoursEnd;

		size_t iMinStart;
		size_t iMinEnd;

		size_t iSecStart;
		size_t iSecEnd;

		int r1 = sscanf ( rsAcqisitionStartTime.c_str(), osDateTimeTemplate.c_str(), &iYearStart, &iMonthStart, &iDayStart, &iHoursStart, &iMinStart, &iSecStart);
		int r2 = sscanf ( rsAcqisitionEndTime.c_str(), osDateTimeTemplate.c_str(), &iYearEnd, &iMonthEnd, &iDayEnd, &iHoursEnd, &iMinEnd, &iSecEnd);
		
		if (r1 != 6 || r2 != 6)
			return false;

		tm timeptrStart;
		timeptrStart.tm_sec = iSecStart;
		timeptrStart.tm_min = iMinStart;
		timeptrStart.tm_hour = iHoursStart;
		timeptrStart.tm_mday = iDayStart;
		timeptrStart.tm_mon = iMonthStart-1;
		timeptrStart.tm_year = iYearStart-1900;

		time_t timeStart = mktime(&timeptrStart);

		tm timeptrEnd;
		timeptrEnd.tm_sec = iSecEnd;
		timeptrEnd.tm_min = iMinEnd;
		timeptrEnd.tm_hour = iHoursEnd;
		timeptrEnd.tm_mday = iDayEnd;
		timeptrEnd.tm_mon = iMonthEnd-1;
		timeptrEnd.tm_year = iYearEnd-1900;
		
		time_t timeEnd = mktime(&timeptrEnd);

		time_t timeAverage = timeStart + (timeEnd - timeStart)/2;

		tm * acqisitionTime = localtime(&timeAverage);
		
		osAcqisitionTime.Printf("%d %d %d %d %d %d", 
			1900 + acqisitionTime->tm_year,
			1 + acqisitionTime->tm_mon,
			acqisitionTime->tm_mday,
			acqisitionTime->tm_hour,
			acqisitionTime->tm_min,
			acqisitionTime->tm_sec);

		return true;
	}
