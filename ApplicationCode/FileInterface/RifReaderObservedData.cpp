/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017- Statoil ASA
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

#include "RifReaderObservedData.h"

#include "RifEclipseSummaryAddress.h"
#include "RifCsvUserDataParser.h"

#include "SummaryPlotCommands/RicPasteAsciiDataToSummaryPlotFeatureUi.h"

#include <QDateTime>
#include <QTextStream>

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifReaderObservedData::RifReaderObservedData()
{
    
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifReaderObservedData::~RifReaderObservedData()
{

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RifReaderObservedData::open(const QString& headerFileName,
                                 const QString& identifierName,
                                 RifEclipseSummaryAddress::SummaryVarCategory summaryCategory)
{
    AsciiDataParseOptions parseOptions;
    parseOptions.dateFormat = "yyyy-MM-dd";
    parseOptions.cellSeparator = "\t";
    parseOptions.locale = QLocale::Norwegian;

    QString data;
    QTextStream out(&data);
    out << "Date" << "\t" << "Oil" << "\t" << "PW" << "\n";
    out << "1993-02-23" << "\t" << "10" << "\t" << "1" << "\n";
    out << "1993-06-15" << "\t" << "20" << "\t" << "2" << "\n";
    out << "1994-02-26" << "\t" << "30" << "\t" << "3" << "\n";
    out << "1994-05-23" << "\t" << "40" << "\t" << "4" << "\n";


    m_asciiParser = std::unique_ptr<RifCsvUserDataParser>(new RifCsvUserDataPastedTextParser(data));

    m_timeSteps.clear();
    if (m_asciiParser->parse(parseOptions))
    {
        if (m_asciiParser && m_asciiParser->dateTimeColumn())
        {
            for (QDateTime timeStep : m_asciiParser->dateTimeColumn()->dateTimeValues)
            {
                time_t t = timeStep.toTime_t();
                m_timeSteps.push_back(t);
            }

            m_allResultAddresses.clear();
            for (auto s : m_asciiParser->tableData().columnInfos())
            {
                m_allResultAddresses.push_back(s.summaryAddress);
            }
        }

        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RifReaderObservedData::values(const RifEclipseSummaryAddress& resultAddress, std::vector<double>* values) const
{
    size_t columnIndex = m_allResultAddresses.size();

    for (size_t i = 0; i < m_allResultAddresses.size(); i++)
    {
        if (resultAddress == m_allResultAddresses[i])
        {
            columnIndex = i;
        }
    }

    if (columnIndex != m_allResultAddresses.size())
    {
        const Column* col = m_asciiParser->columnInfo(columnIndex);
        if (col && col->dataType == Column::NUMERIC)
        {
            for (auto& v : col->values)
            {
                values->push_back(v);
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const std::vector<time_t>& RifReaderObservedData::timeSteps(const RifEclipseSummaryAddress& resultAddress) const
{
    return m_timeSteps;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RifEclipseSummaryAddress RifReaderObservedData::address(const QString& quantity,
                                                        const QString& identifierName, 
                                                        RifEclipseSummaryAddress::SummaryVarCategory summaryCategory)
{
    std::string        quantityName = quantity.toStdString();
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

    switch (summaryCategory)
    {
    case RifEclipseSummaryAddress::SUMMARY_WELL_GROUP:
        wellGroupName = identifierName.toStdString();
        break;
    case RifEclipseSummaryAddress::SUMMARY_WELL:
        wellName = identifierName.toStdString();
        break;
    case RifEclipseSummaryAddress::SUMMARY_WELL_LGR:
        lgrName = identifierName.toStdString();
        break;
    default:
        break;
    }

    return RifEclipseSummaryAddress(summaryCategory,
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
std::string RifReaderObservedData::unitName(const RifEclipseSummaryAddress& resultAddress) const
{
    std::string str = "";
    return str;
}
