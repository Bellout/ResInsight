/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017 Statoil ASA
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

#include "cvfBase.h"
#include "cvfVector3.h"

#include <vector>
#include <map>
#include <set>

class RigCompletionData;
class RimWellPath;
class RicExportCompletionDataSettingsUi;
class RigEclipseCaseData;

struct WellBorePartForTransCalc;


//==================================================================================================
/// 
//==================================================================================================
class RicFishbonesTransmissibilityCalculationFeatureImp
{
public:
    static std::vector<RigCompletionData> generateFishboneCompdatValuesUsingAdjustedCellVolume(const RimWellPath* wellPath, 
                                                                                               const RicExportCompletionDataSettingsUi& settings);


private:
    static void findFishboneLateralsWellBoreParts(std::map<size_t, 
                                                  std::vector<WellBorePartForTransCalc>>& wellBorePartsInCells,
                                                  const RimWellPath* wellPath, 
                                                  const RicExportCompletionDataSettingsUi& settings);

    static void findFishboneImportedLateralsWellBoreParts(std::map<size_t, std::vector<WellBorePartForTransCalc>>& wellBorePartsInCells,
                                                          const RimWellPath* wellPath, 
                                                          const RicExportCompletionDataSettingsUi& settings);

    static void findMainWellBoreParts(std::map<size_t, std::vector<WellBorePartForTransCalc>>& wellBorePartsInCells,
                                      const RimWellPath* wellPath, 
                                      const RicExportCompletionDataSettingsUi& settings);

    static std::set<size_t> findIntersectedCells(const RigEclipseCaseData* caseData, const std::vector<cvf::Vec3d>& coords);
};
