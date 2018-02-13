/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2018     Statoil ASA
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

#include "RiaFractureDefines.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaDefines::conductivityResultName()
{
    return "CONDUCTIVITY";
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RiaDefines::unitStringConductivity(RiaEclipseUnitTools::UnitSystem unitSystem)
{
    switch (unitSystem)
    {
        case RiaEclipseUnitTools::UNITS_METRIC:     return "md-m";
        case RiaEclipseUnitTools::UNITS_FIELD:      return "md-ft";
        default:                                    return "";
    }
}
