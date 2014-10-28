/******************************************************************************
 * $Id$
 *
 * Name:     gnmgdalrule.cpp
 * Project:  GDAL/OGR Geography Network support (Geographic Network Model)
 * Purpose:  GNMGdalNetwork rule syntax.
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

#include "gnm.h"


GNMErr GNMGdalNetwork::_parseRuleString (const char *str)
{
    std::string ruleStr = str;

    // Parse the string and build the given rule.
    ruleStr += " ";
    std::vector<std::string> words;
    std::string word;

    //std::vector<int>::size_type sz = myvector.size();

    for (int i = 0; i < ruleStr.size(); i++)
    {
        if (ruleStr[i] == ' ') // Spaces are served as the splitters.
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word.push_back(ruleStr[i]);
        }

    }
    if (words.empty())
        return GNMERR_FAILURE;

    // The first word of the resulting array may be only the CLASS or
    // NETWORK keyword.

    if (words.size() >= 2 && words[0].compare(GNM_KW_CLASS) == 0)
    {
        // The second is a name of the class.

        // We restrict creating rules for system edges class.
        if (words[1] == GNM_CLASSLAYER_SYSEDGES)
        {
            return GNMERR_FAILURE;
        }

        // Find an old class rules structure or create a new one.
        GNMClassRule *classRule = _findRuleForClass(words[1].data());
        if (classRule == NULL)
        {
            //CPLError(CE_Warning,CPLE_None,"The rule is being created for "
            //         "unexisted class %s ...",words[1].data());
            // ISSUE: Check the correct new layer name?
            classRule = new GNMClassRule();
        }

        // The next should go several key-phrases, started from the key
        // words. We parse them and form the rules.
        // If each separate rule already exist we return an error.
        int words_count = 2;
        do
        {
            if (words.size() >= words_count + 2
             && words[words_count].compare(GNM_KW_COSTS) == 0)
            {
                //CPLError(CE_Warning,CPLE_None,"There is no field %s in layer %s,"
                //         " but the rule is being created ...",words[words_count+1].data(),
                //         words[1].data());
                //if (classRule->dirCostField == "")
                // We will not modify new rule if an old one already exists.
                if (classRule->dirCostOper == gnmOpUndefined)
                {
                    // Assign the field name or float constant.
                    // TODO: somehow check if this is a float value in other way.
                    const char *toDouble = words[words_count+1].data();
                    double dConst = atof(toDouble);
                    if (dConst == 0.0)
                    {
                        classRule->dirCostField = words[words_count+1];
                        classRule->dirCostOper = gnmOpField;
                        words_count += 2;

                        if (words.size() >= words_count + 2)
                        {
                            toDouble = words[words_count+1].data();
                            dConst = atof(toDouble);
                            // Check if the third word after COSTS is a float constant.
                            if (dConst != 0.0)
                            {
                                // Check if the second word after COSTS is a mathematic sign.
                                if (words[words_count].compare(GNM_KW_ADD) == 0)
                                {
                                    classRule->dirCostConst = dConst;
                                    classRule->dirCostOper = gnmOpFieldAConst;
                                    words_count += 2;
                                }
                                else if (words[words_count].compare(GNM_KW_SUB) == 0)
                                {
                                    classRule->dirCostConst = dConst;
                                    classRule->dirCostOper = gnmOpFieldSConst;
                                    words_count += 2;
                                }
                                else if (words[words_count].compare(GNM_KW_MULT) == 0)
                                {
                                    classRule->dirCostConst = dConst;
                                    classRule->dirCostOper = gnmOpFieldMConst;
                                    words_count += 2;
                                }
                                else if (words[words_count].compare(GNM_KW_DIV) == 0)
                                {
                                    classRule->dirCostConst = dConst;
                                    classRule->dirCostOper = gnmOpFieldDConst;
                                    words_count += 2;
                                }
                                else
                                {
                                    // CPLErr
                                    delete classRule;
                                    return GNMERR_FAILURE;
                                }
                            }
                            else
                            {
                                // CPLErr
                                delete classRule;
                                return GNMERR_FAILURE;
                            }
                        }
                    }
                    else
                    {
                        classRule->dirCostConst = dConst;
                        classRule->dirCostOper = gnmOpConst;
                        words_count += 2;
                    }
                }
                else
                {
                    // CPLErr
                    delete classRule;
                    return GNMERR_FAILURE;
                }
            }

            else if (words.size() >= words_count + 2
             && words[words_count].compare(GNM_KW_INVCOSTS) == 0)
            {
                // TODO: Make a warning, if there is no such field in the layer.
                //if (classRule->dirCostField == "")
                // We will not modify new rule if an old one already exists.
                if (classRule->invCostOper == gnmOpUndefined)
                {
                    // Assign the field name or float constant.
                    // TODO: somehow check if this is a float value in other way.
                    const char *toDouble = words[words_count+1].data();
                    double dConst = atof(toDouble);
                    if (dConst == 0.0)
                    {
                        classRule->invCostField = words[words_count+1];
                        classRule->invCostOper = gnmOpField;
                        words_count += 2;

                        if (words.size() >= words_count + 2)
                        {
                            toDouble = words[words_count+1].data();
                            dConst = atof(toDouble);
                            // Check if the third word after COSTS is a float constant.
                            if (dConst != 0.0)
                            {
                                // Check if the second word after COSTS is a mathematic sign.
                                if (words[words_count].compare(GNM_KW_ADD) == 0)
                                {
                                    classRule->invCostConst = dConst;
                                    classRule->invCostOper = gnmOpFieldAConst;
                                    words_count += 2;
                                }
                                else if (words[words_count].compare(GNM_KW_SUB) == 0)
                                {
                                    classRule->invCostConst = dConst;
                                    classRule->invCostOper = gnmOpFieldSConst;
                                    words_count += 2;
                                }
                                else if (words[words_count].compare(GNM_KW_MULT) == 0)
                                {
                                    classRule->invCostConst = dConst;
                                    classRule->invCostOper = gnmOpFieldMConst;
                                    words_count += 2;
                                }
                                else if (words[words_count].compare(GNM_KW_DIV) == 0)
                                {
                                    classRule->invCostConst = dConst;
                                    classRule->invCostOper = gnmOpFieldDConst;
                                    words_count += 2;
                                }
                                else
                                {
                                    // CPLErr
                                    delete classRule;
                                    return GNMERR_FAILURE;
                                }
                            }
                            else
                            {
                                // CPLErr
                                delete classRule;
                                return GNMERR_FAILURE;
                            }
                        }
                    }
                    else
                    {
                        classRule->invCostConst = dConst;
                        classRule->invCostOper = gnmOpConst;
                        words_count += 2;
                    }
                }
                else
                {
                    // CPLErr
                    delete classRule;
                    return GNMERR_FAILURE;
                }
            }

            else if (words.size() >= words_count + 2
             && words[words_count].compare(GNM_KW_DIRECTS) == 0)
            {
                // TODO: Make a warning, if there is no such field in the layer.
                if (classRule->dirField == "")
                {
                    classRule->dirField = words[words_count+1];
                    words_count += 2;
                }
                else
                {
                    // CPLErr
                    delete classRule;
                    return GNMERR_FAILURE;
                }
            }

            else if (words.size() >= words_count + 2
             && words[words_count].compare(GNM_KW_BEHAVES) == 0)
            {
                if (classRule->roleStr == "")
                {
                    classRule->roleStr = words[words_count+1];
                    words_count += 2;
                }
                else
                {
                    // CPLErr
                    delete classRule;
                    return GNMERR_FAILURE;
                }
            }

            else return GNMERR_FAILURE; // Some error in the string.
        }
        while (words_count < words.size());

        // There was a successfull parsing of 1 to N key phrases.
        // The rule was updated/created and we save it if needed.
        _classRules.insert(std::make_pair(words[1].data(),classRule));
        return GNMERR_NONE;
    }

    // Try to parse network rule.
    else if (words[0].compare(GNM_KW_NETWORK) == 0)
    {
        int words_count = 1;
        do
        {
            if (words.size() >= words_count + 4
             && words[words_count].compare(GNM_KW_CONNECTS) == 0
             && words[words_count+2].compare(GNM_KW_WITH) == 0)
            {
                std::string className1 = words[words_count+1].data();
                std::string className2 = words[words_count+3].data();

                GNMClassRule *classRule1 = _findRuleForClass(className1);
                GNMClassRule *classRule2 = _findRuleForClass(className2);
                if (classRule1 == NULL)
                {
                    classRule1 = new GNMClassRule();
                }
                if (classRule2 == NULL)
                {
                    classRule2 = new GNMClassRule();
                }

                _GNMVertEdgeClassNames pair1;
                _GNMVertEdgeClassNames pair2;

                if (words.size() >= words_count + 6
                 && words[words_count+4].compare(GNM_KW_VIA) == 0)
                {
                    std::string classNameC = words[words_count+5].data();

                    pair1 = std::make_pair(className2,classNameC);
                    pair2 = std::make_pair(className1,classNameC);
                    words_count += 6;
                }
                else
                {
                    pair1 = std::make_pair(className2,"");
                    pair2 = std::make_pair(className1,"");
                    words_count += 4;
                }

                classRule1->conRules.insert(pair1);
                classRule2->conRules.insert(pair2);

                // There will be no insertion into the map, if there is fully the
                // same rule already.
                _classRules.insert(std::make_pair(className1,classRule1));
                _classRules.insert(std::make_pair(className2,classRule2));
            }

            else return GNMERR_FAILURE;
        }
        while (words_count < words.size());

        return GNMERR_NONE;
    }

    return GNMERR_FAILURE;
}



GNMClassRule *GNMGdalNetwork::_findRuleForClass (std::string clStr)
{
    GNMClassRuleMap::iterator it;
    it = _classRules.find(clStr);
    if (it == _classRules.end())
        return NULL;
    return (*it).second;
}




