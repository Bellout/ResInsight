/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017     Statoil ASA
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

#pragma once

#include "RiaPorosityModel.h"

#include "cvfBase.h"
#include "cvfObject.h"
#include "cvfMatrix4.h"

#include <vector>

class QString;

class RimEclipseCase;
class RigFractureCell;
class RigEclipseCaseData;
class RigResultAccessor;

//==================================================================================================
///
//==================================================================================================

class RigEclipseToStimPlanCellTransmissibilityCalculator
{
public:
    explicit RigEclipseToStimPlanCellTransmissibilityCalculator(RimEclipseCase* caseToApply,
                                                                cvf::Mat4d fractureTransform,
                                                                double skinFactor,
                                                                double cDarcy,
                                                                const RigFractureCell& stimPlanCell);

    const std::vector<size_t>&  globalIndeciesToContributingEclipseCells();
    const std::vector<double>&  contributingEclipseCellTransmissibilities();

private:
    void                        calculateStimPlanCellsMatrixTransmissibility();
    std::vector<size_t>         getPotentiallyFracturedCellsForPolygon(std::vector<cvf::Vec3d> polygon);

    static cvf::ref<RigResultAccessor>
        loadResultAndCreateResultAccessor(RimEclipseCase* eclipseCase,
                                          RiaDefines::PorosityModelType porosityModel,
                                          const QString& uiResultName);

private:
    RimEclipseCase*                     m_case;
    double                              m_cDarcy;
    double                              m_fractureSkinFactor;
    cvf::Mat4d                          m_fractureTransform;
    const RigFractureCell&              m_stimPlanCell;

    std::vector<size_t>                 m_globalIndeciesToContributingEclipseCells;
    std::vector<double>                 m_contributingEclipseCellTransmissibilities;
};

