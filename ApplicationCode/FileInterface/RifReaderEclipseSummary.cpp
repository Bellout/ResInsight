/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Statoil ASA
// 
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
// 
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RifReaderEclipseSummary.h"
#include "RiaStringEncodingTools.h"

#include "ert/ecl/ecl_sum.h"

#include <string>
#include <assert.h>

#include <QDateTime>
#include <QString>
#include <QStringList>
#include "ert/ecl/smspec_node.h"

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifReaderEclipseSummary::RifReaderEclipseSummary()
    : m_ecl_sum(nullptr), 
      m_ecl_SmSpec(nullptr)
{

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifReaderEclipseSummary::~RifReaderEclipseSummary()
{
    if (m_ecl_sum)
    {
        ecl_sum_free(m_ecl_sum);
        m_ecl_sum = nullptr;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RifReaderEclipseSummary::open(const QString& headerFileName, const QStringList& dataFileNames)
{
    assert(m_ecl_sum == NULL); 
    
    if (headerFileName.isEmpty() || dataFileNames.size() == 0) return false;

    assert(!headerFileName.isEmpty());
    assert(dataFileNames.size() > 0);

    stringlist_type* dataFiles = stringlist_alloc_new();
    for (int i = 0; i < dataFileNames.size(); i++)
    {
        stringlist_append_copy(dataFiles, RiaStringEncodingTools::toNativeEncoded(dataFileNames[i]).data());
    }

    std::string itemSeparatorInVariableNames = ":";
    m_ecl_sum = ecl_sum_fread_alloc(RiaStringEncodingTools::toNativeEncoded(headerFileName).data(), dataFiles, itemSeparatorInVariableNames.data(), false);

    stringlist_free(dataFiles);

    if (m_ecl_sum)
    {
        m_timeSteps.clear();
        m_ecl_SmSpec = ecl_sum_get_smspec(m_ecl_sum);

        for ( int time_index = 0; time_index < timeStepCount(); time_index++ )
        {
            time_t sim_time = ecl_sum_iget_sim_time(m_ecl_sum, time_index);
            m_timeSteps.push_back(sim_time);
        }

        buildMetaData();

        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifEclipseSummaryAddress addressFromErtSmSpecNode(const smspec_node_type * ertSumVarNode)
{
    if (   smspec_node_get_var_type(ertSumVarNode) == ECL_SMSPEC_INVALID_VAR
        || !smspec_node_is_valid(ertSumVarNode)) 
    {
        return RifEclipseSummaryAddress();
    }

    RifEclipseSummaryAddress::SummaryVarCategory sumCategory(RifEclipseSummaryAddress::SUMMARY_INVALID);
    std::string        quantityName;
    int                regionNumber(-1);
    int                regionNumber2(-1);
    std::string        wellGroupName;
    std::string        wellName;
    int                wellSegmentNumber(-1);
    std::string        lgrName;
    int                cellI(-1);
    int                cellJ(-1);
    int                cellK(-1);
    int                aquiferNumber(-1);

    quantityName = smspec_node_get_keyword(ertSumVarNode);

    switch (smspec_node_get_var_type(ertSumVarNode))
    {
        case ECL_SMSPEC_AQUIFER_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_AQUIFER;
            aquiferNumber = smspec_node_get_num(ertSumVarNode);
        }
        break;
        case ECL_SMSPEC_WELL_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_WELL;
            wellName = smspec_node_get_wgname(ertSumVarNode);
        }
        break;
        case ECL_SMSPEC_REGION_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_REGION;
            regionNumber = smspec_node_get_num(ertSumVarNode);
        }
        break;
        case ECL_SMSPEC_FIELD_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_FIELD;
        }
        break;
        case ECL_SMSPEC_GROUP_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_WELL_GROUP;
            wellGroupName = smspec_node_get_wgname(ertSumVarNode);
        }
        break;
        case ECL_SMSPEC_BLOCK_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_BLOCK;
            
            const int* ijk = smspec_node_get_ijk(ertSumVarNode);
            cellI = ijk[0];
            cellJ = ijk[1];
            cellK = ijk[2];
        }
        break;
        case ECL_SMSPEC_COMPLETION_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_WELL_COMPLETION;
            wellName = smspec_node_get_wgname(ertSumVarNode);
            const int* ijk = smspec_node_get_ijk(ertSumVarNode);
            cellI = ijk[0];
            cellJ = ijk[1];
            cellK = ijk[2];
        }
        break;
        case ECL_SMSPEC_LOCAL_BLOCK_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_BLOCK_LGR;
            lgrName = smspec_node_get_lgr_name(ertSumVarNode);
            const int* ijk = smspec_node_get_lgr_ijk(ertSumVarNode);
            cellI = ijk[0];
            cellJ = ijk[1];
            cellK = ijk[2];
        }
        break;
        case ECL_SMSPEC_LOCAL_COMPLETION_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_WELL_COMPLETION_LGR;
            wellName = smspec_node_get_wgname(ertSumVarNode);
            lgrName = smspec_node_get_lgr_name(ertSumVarNode);
            const int* ijk = smspec_node_get_lgr_ijk(ertSumVarNode);
            cellI = ijk[0];
            cellJ = ijk[1];
            cellK = ijk[2];
        }
        break;
        case ECL_SMSPEC_LOCAL_WELL_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_WELL_LGR;
            wellName = smspec_node_get_wgname(ertSumVarNode);
            lgrName = smspec_node_get_lgr_name(ertSumVarNode);
        }
        break;
        case ECL_SMSPEC_NETWORK_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_NETWORK;
        }
        break;
        case ECL_SMSPEC_REGION_2_REGION_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_REGION_2_REGION;
            regionNumber = smspec_node_get_R1(ertSumVarNode);
            regionNumber2 = smspec_node_get_R2(ertSumVarNode);

        }
        break;
        case ECL_SMSPEC_SEGMENT_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_WELL_SEGMENT;
            wellSegmentNumber = smspec_node_get_num(ertSumVarNode);
        }
        break;
        case ECL_SMSPEC_MISC_VAR:
        {
            sumCategory = RifEclipseSummaryAddress::SUMMARY_MISC;
        }
        break;
        default:
            CVF_ASSERT(false);
        break;
    }

    return RifEclipseSummaryAddress(sumCategory, 
                                    quantityName, 
                                    regionNumber, 
                                    regionNumber2, 
                                    wellGroupName, 
                                    wellName, 
                                    wellSegmentNumber, 
                                    lgrName, 
                                    cellI, cellJ, cellK,
                                    aquiferNumber);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RifReaderEclipseSummary::values(const RifEclipseSummaryAddress& resultAddress, std::vector<double>* values) const
{
    assert(m_ecl_sum != NULL);

    int variableIndex = indexFromAddress(resultAddress);

    if ( variableIndex < 0 ) return false;

    values->clear();
    int tsCount = timeStepCount();
    values->reserve(timeStepCount());

    if (m_ecl_SmSpec)
    {
        const smspec_node_type* ertSumVarNode = ecl_smspec_iget_node(m_ecl_SmSpec, variableIndex);
        int paramsIndex = smspec_node_get_params_index(ertSumVarNode);

        for(int time_index = 0; time_index < tsCount; time_index++)
        {
            double value = ecl_sum_iget(m_ecl_sum, time_index, paramsIndex);
            values->push_back(value);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
int RifReaderEclipseSummary::timeStepCount() const
{
    assert(m_ecl_sum != nullptr);

    if (m_ecl_SmSpec == nullptr) return 0;

    return ecl_sum_get_data_length(m_ecl_sum);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const std::vector<time_t>& RifReaderEclipseSummary::timeSteps(const RifEclipseSummaryAddress& resultAddress) const
{
    assert(m_ecl_sum != NULL);

    return m_timeSteps;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
int RifReaderEclipseSummary::indexFromAddress(const RifEclipseSummaryAddress& resultAddress) const
{
    auto it = m_resultAddressToErtNodeIdx.find(resultAddress);
    if (it != m_resultAddressToErtNodeIdx.end())
    {
        return it->second;
    }

    return -1;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RifReaderEclipseSummary::buildMetaData()
{
    m_allResultAddresses.clear();
    m_resultAddressToErtNodeIdx.clear();

    if(m_ecl_SmSpec)
    {
        int varCount = ecl_smspec_num_nodes(m_ecl_SmSpec);
        for(int i = 0; i < varCount; i++)
        {
            const smspec_node_type * ertSumVarNode = ecl_smspec_iget_node(m_ecl_SmSpec, i);
            RifEclipseSummaryAddress addr = addressFromErtSmSpecNode(ertSumVarNode);
            m_allResultAddresses.push_back(addr);
            m_resultAddressToErtNodeIdx[addr] = i;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::string RifReaderEclipseSummary::unitName(const RifEclipseSummaryAddress& resultAddress) const
{
    if (!m_ecl_SmSpec) return "";

    int variableIndex = indexFromAddress(resultAddress);

    if(variableIndex < 0) return "";

    const smspec_node_type * ertSumVarNode = ecl_smspec_iget_node(m_ecl_SmSpec, variableIndex);
    return smspec_node_get_unit(ertSumVarNode);
}

#if 0
//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RifReaderEclipseSummary::populateVectorFromStringList(stringlist_type* stringList, std::vector<std::string>* strings)
{
    assert(stringList && strings);

    for (int i = 0; i < stringlist_get_size(stringList); i++)
    {
        strings->push_back(stringlist_iget(stringList, i));
    }
}

#endif
