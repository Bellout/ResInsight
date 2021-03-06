//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################


#include "cvfScalarMapperDiscreteLog.h"
#include "cvfScalarMapperDiscreteLinear.h"
#include "cvfMath.h"

#include <assert.h>
#include <cmath>

namespace cvf {

//==================================================================================================
///
/// \class cvf::ScalarMapperDiscreteLog
/// \ingroup Render
///
/// Maps scalar values to texture coordinates/colors using discrete logarithmic mapping
//==================================================================================================

ScalarMapperDiscreteLog::ScalarMapperDiscreteLog()
:   m_logRange(0.0),
    m_logRangeMin(0.0),
    m_hasNegativeRange(false)
{
    m_decadeLevelCount = 2; 
}



//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
Color3ub ScalarMapperDiscreteLog::mapToColor(double scalarValue) const
{
    assert(m_sortedLevels.size() > 0);

    double discVal = ScalarMapperDiscreteLinear::discretizeToLevelBelow(scalarValue, m_sortedLevels);
    std::set<double>::reverse_iterator it = m_sortedLevels.rbegin();

    if (m_sortedLevels.size() > 1) it++;

    double levelUnderMax = *it;
    double normDiscVal = normalizedValue(discVal);
    double normSemiMaxVal = normalizedValue(levelUnderMax);
    double adjustedNormVal = 0;
    if (normSemiMaxVal != 0) adjustedNormVal = normDiscVal/normSemiMaxVal;
    adjustedNormVal = cvf::Math::clamp(adjustedNormVal, 0.0, 1.0);

    return colorFromUserColorGradient(adjustedNormVal);
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
double ScalarMapperDiscreteLog::normalizedValue(double scalarValue) const
{
    if (m_hasNegativeRange) scalarValue = -1.0*scalarValue;

    double logValue;

    if (scalarValue <= 0) logValue = std::numeric_limits<double>::min_exponent10;
    else                  logValue = log10(scalarValue);

    if (m_logRange != 0)
    {
        return cvf::Math::clamp((logValue - m_logRangeMin)/m_logRange, 0.0, 1.0);
    }
    else
    {
        return 0;
    }
} 

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
double ScalarMapperDiscreteLog::domainValue(double normalizedPosition) const
{
    double logValue = normalizedPosition*m_logRange + m_logRangeMin;
    double domainVal = pow(10, logValue);

    if (m_hasNegativeRange)
        domainVal *= -1.0;

    return domainVal;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void ScalarMapperDiscreteLog::rangeUpdated()
{
    m_hasNegativeRange = false;

    double transformedRangeMax = m_rangeMax;
    double transformedRangeMin = m_rangeMin;

    if ( m_rangeMax <= 0 &&  m_rangeMin <= 0)
    { 
        m_hasNegativeRange = true;

        transformedRangeMax = -1.0*transformedRangeMax;
        transformedRangeMin = -1.0*transformedRangeMin;
    }

    double logRangeMax = (transformedRangeMax > 0) ? log10(transformedRangeMax): std::numeric_limits<double>::min_exponent10;
    m_logRangeMin      = (transformedRangeMin > 0) ? log10(transformedRangeMin): std::numeric_limits<double>::min_exponent10;

    m_logRange = logRangeMax - m_logRangeMin; 
}

} // namespace cvf
