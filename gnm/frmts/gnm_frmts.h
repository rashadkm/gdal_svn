/******************************************************************************
 * $Id$
 *
 * Name:     gnm_frmts.h
 * Project:  GDAL/OGR Geography Network support (Geographic Network Model)
 * Purpose:  GNM declarations for network formats implementation.
 * Author:   Mikhail Gusev (gusevmihs at gmail dot com)
 *
 ******************************************************************************
 * Copyright (c) 2014, Mikhail Gusev
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

#ifndef _GNM_FRMTS_H_INCLUDED
#define _GNM_FRMTS_H_INCLUDED

#include "gnm.h"

// All GNM formats.
// TODO: expected that at least these and also more others will be supported in future.
static const char *GNMAllFormats[] = {
  "GDAL Network",
  "pgRouting Network",
  "SpatiaLite VirtualNetwok",
   //...
};

typedef void *GNMFormatH;

/************************************************************************/
/*                           GNMFormat                                  */
/************************************************************************/
/**
 * Abstract networks factory. The concrete implementations must
 * implement all these methods to manage the concrete network formats.
 *
 * @since GDAL 2.0
 */
class CPL_DLL GNMFormat
{
    friend class GNMManager;

    private:

    virtual GNMNetwork *create (GDALDataset *poDS,
                                char **papszOptions = NULL) = 0;

    virtual GNMNetwork *open (GDALDataset *poDS,
                              char **papszOptions = NULL) = 0;

    virtual GNMErr close (GNMNetwork *poNet) = 0;

    virtual GNMErr remove (GDALDataset *poDS) = 0;

    virtual bool has (GDALDataset *poDS) = 0;

    public:

    ~GNMFormat () {}
};


/************************************************************************/
/*              Public methods for working with formats                 */
/************************************************************************/

// Here will be stored all registered network formats (except GDAL-network
// format).
static std::vector<std::string> _regNetFrmts;


CPL_C_START

/************************************************************************/
/*                         GetFormatByName()                            */
/************************************************************************/
/**
 * Returns GNMFormat object that will be used in GNMManager methods
 * in order to initialize the network of specific format.
 *
 * @since GDAL 2.0
 */

GNMFormatH CPL_DLL *GetFormatByName (const char *pszDriverName)
{
    // FOR DEVELOPERS: Add the return of new GNMFormat object to this method.

    return NULL;
}


/************************************************************************/
/*                    RegisterAllNetworkFormats()                       */
/************************************************************************/
/**
 * Registeres all network formats and write them to _regNetFrmts.
 *
 * @since GDAL 2.0
 */

void CPL_DLL RegisterAllNetworkFormats ()
{
    // FOR DEVELOPERS: Add registration of new format to this method.

}

CPL_C_END

#endif //_GNM_FRMTS_H_INCLUDED
