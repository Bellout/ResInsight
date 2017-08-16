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

#include "RicFishbonesTransmissibilityCalculationFeatureImp.h"

#include "RicExportCompletionDataSettingsUi.h"
#include "RicWellPathExportCompletionDataFeature.h"

#include "RigActiveCellInfo.h"
#include "RigCompletionData.h"
#include "RigEclipseCaseData.h"
#include "RigMainGrid.h"
#include "RigWellPath.h"

#include "RimFishboneWellPath.h"
#include "RimFishboneWellPathCollection.h"
#include "RimFishbonesCollection.h"
#include "RimFishbonesMultipleSubs.h"
#include "RimWellPath.h"
#include "RimWellPathCompletions.h"

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicFishbonesTransmissibilityCalculationFeatureImp::findFishboneLateralsWellBoreParts(std::map<size_t, std::vector<WellBorePartForTransCalc> >& wellBorePartsInCells, const RimWellPath* wellPath, const RicExportCompletionDataSettingsUi& settings)
{
    // Generate data
    const RigEclipseCaseData* caseData = settings.caseToApply()->eclipseCaseData();
    std::vector<WellSegmentLocation> locations = RicWellPathExportCompletionDataFeature::findWellSegmentLocations(settings.caseToApply, wellPath);

    RiaEclipseUnitTools::UnitSystem unitSystem = caseData->unitsType();
    bool isMainBore = false;

    std::vector<RigCompletionData> completionData;

    for (const WellSegmentLocation& location : locations)
    {
        for (const WellSegmentLateral& lateral : location.laterals)
        {
            for (const WellSegmentLateralIntersection& intersection : lateral.intersections)
            {
                double diameter = location.fishbonesSubs->holeDiameter(unitSystem);
                QString completionMetaData = (location.fishbonesSubs->name() + QString(": Sub: %1 Lateral: %2").arg(location.subIndex).arg(lateral.lateralIndex));
                WellBorePartForTransCalc wellBorePart = WellBorePartForTransCalc(intersection.lengthsInCell, 
                                                                                 diameter / 2, 
                                                                                 location.fishbonesSubs->skinFactor(), 
                                                                                 isMainBore,
                                                                                 completionMetaData);

                wellBorePartsInCells[intersection.cellIndex].push_back(wellBorePart);

            }
        }
    }

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RigCompletionData> RicFishbonesTransmissibilityCalculationFeatureImp::generateFishboneCompdatValuesUsingAdjustedCellVolume(const RimWellPath* wellPath, 
                                                                                                                                               const RicExportCompletionDataSettingsUi& settings)
{
    std::map<size_t, std::vector<WellBorePartForTransCalc> > wellBorePartsInCells; //wellBore = main bore or fishbone lateral
    findFishboneLateralsWellBoreParts(wellBorePartsInCells, wellPath, settings);
    findFishboneImportedLateralsWellBoreParts(wellBorePartsInCells, wellPath, settings);
    if (!wellBorePartsInCells.empty())
    {
        //Don't include main bore if there are no fishbones
        findMainWellBoreParts(wellBorePartsInCells, wellPath, settings);
    }

    std::vector<RigCompletionData> completionData;

    RigMainGrid* grid = settings.caseToApply->eclipseCaseData()->mainGrid();
    const RigActiveCellInfo* activeCellInfo = settings.caseToApply->eclipseCaseData()->activeCellInfo(RiaDefines::MATRIX_MODEL);

    for (auto cellAndWellBoreParts : wellBorePartsInCells)
    {
        size_t cellIndex = cellAndWellBoreParts.first;
        std::vector<WellBorePartForTransCalc> wellBoreParts = cellAndWellBoreParts.second;
        size_t i, j, k;
        grid->ijkFromCellIndex(cellIndex, &i, &j, &k);

        bool cellIsActive = activeCellInfo->isActive(cellIndex);
        if (!cellIsActive) continue;

        //finding main bore and number of laterals
        size_t numberOfLaterals = 0;
        CellDirection mainBoreDirection = DIR_I;
        for (auto wellBorePart : wellBoreParts)
        {
            if (!wellBorePart.isMainBore)
            {
                numberOfLaterals++;
            }
            else
            {
                mainBoreDirection = RicWellPathExportCompletionDataFeature::calculateDirectionInCell(settings.caseToApply,
                                                                                                     cellIndex,
                                                                                                     wellBorePart.lengthsInCell);
            }
        }
        
        for (WellBorePartForTransCalc wellBorePart : wellBoreParts)
        {
            RigCompletionData completion(wellPath->completions()->wellNameForExport(), IJKCellIndex(i, j, k));

            double transmissibility = 0.0;
            if (wellBorePart.isMainBore)
            {
                //No change in transmissibility for main bore
                transmissibility = RicWellPathExportCompletionDataFeature::calculateTransmissibility(settings.caseToApply,
                                                                                                            wellPath,
                                                                                                            wellBorePart.lengthsInCell,
                                                                                                            wellBorePart.skinFactor,
                                                                                                            wellBorePart.wellRadius,
                                                                                                            cellIndex);

            }
            else
            {
                //Adjust transmissibility for fishbone laterals
                transmissibility = RicWellPathExportCompletionDataFeature::calculateTransmissibility(settings.caseToApply,
                                                                                                     wellPath,
                                                                                                     wellBorePart.lengthsInCell,
                                                                                                     wellBorePart.skinFactor,
                                                                                                     wellBorePart.wellRadius,
                                                                                                     cellIndex,
                                                                                                     numberOfLaterals,
                                                                                                     mainBoreDirection);

            }

            CellDirection direction = RicWellPathExportCompletionDataFeature::calculateDirectionInCell(settings.caseToApply, 
                                                                                                       cellIndex, 
                                                                                                       wellBorePart.lengthsInCell);

            completion.setTransAndWPImultBackgroundDataFromFishbone(transmissibility,  
                                                                    wellBorePart.skinFactor, 
                                                                    wellBorePart.wellRadius *2, 
                                                                    direction,
                                                                    wellBorePart.isMainBore);

            completion.addMetadata(wellBorePart.metaData, QString::number(transmissibility));
            
            completionData.push_back(completion);
        }
    }
    return completionData;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicFishbonesTransmissibilityCalculationFeatureImp::findFishboneImportedLateralsWellBoreParts(std::map<size_t, std::vector<WellBorePartForTransCalc> >& wellBorePartsInCells, const RimWellPath* wellPath, const RicExportCompletionDataSettingsUi& settings)
{
    RiaEclipseUnitTools::UnitSystem unitSystem = settings.caseToApply->eclipseCaseData()->unitsType();
    std::vector<size_t> wellPathCells = RicWellPathExportCompletionDataFeature::findIntersectingCells(settings.caseToApply()->eclipseCaseData(), wellPath->wellPathGeometry()->m_wellPathPoints);
    bool isMainBore = false;

    double diameter = wellPath->fishbonesCollection()->wellPathCollection()->holeDiameter(unitSystem);
    for (const RimFishboneWellPath* fishbonesPath : wellPath->fishbonesCollection()->wellPathCollection()->wellPaths())
    {
        std::vector<WellPathCellIntersectionInfo> intersectedCells = RigWellPathIntersectionTools::findCellsIntersectedByPath(settings.caseToApply->eclipseCaseData(), fishbonesPath->coordinates());
        for (auto& cell : intersectedCells)
        {
            if (std::find(wellPathCells.begin(), wellPathCells.end(), cell.cellIndex) != wellPathCells.end()) continue;

            double skinFactor = wellPath->fishbonesCollection()->wellPathCollection()->skinFactor();
            QString completionMetaData = fishbonesPath->name();
            WellBorePartForTransCalc wellBorePart = WellBorePartForTransCalc(cell.internalCellLengths, 
                                                                             diameter / 2, 
                                                                             skinFactor, 
                                                                             isMainBore,
                                                                             completionMetaData);
            wellBorePartsInCells[cell.cellIndex].push_back(wellBorePart); 
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicFishbonesTransmissibilityCalculationFeatureImp::findMainWellBoreParts(std::map<size_t, std::vector<WellBorePartForTransCalc>>& wellBorePartsInCells,
                                                                              const RimWellPath* wellPath,
                                                                              const RicExportCompletionDataSettingsUi& settings)
{
    RiaEclipseUnitTools::UnitSystem unitSystem = settings.caseToApply->eclipseCaseData()->unitsType();
    bool isMainBore = true;
    double holeDiameter = wellPath->fishbonesCollection()->mainBoreDiameter(unitSystem);
    double FishboneStartMD = wellPath->fishbonesCollection()->startMD();

    std::vector<double> wellPathMD = wellPath->wellPathGeometry()->m_measuredDepths;
    double wellPathEndMD = 0.0;
    if (wellPathMD.size() > 1) wellPathEndMD = wellPathMD.back();

    std::vector<cvf::Vec3d> fishbonePerfWellPathCoords = wellPath->wellPathGeometry()->clippedPointSubset(wellPath->fishbonesCollection()->startMD(),
                                                                                                          wellPathEndMD);

    std::vector<WellPathCellIntersectionInfo> intersectedCellsIntersectionInfo = RigWellPathIntersectionTools::findCellsIntersectedByPath(settings.caseToApply->eclipseCaseData(),
                                                                                                                                          fishbonePerfWellPathCoords);

    for (auto& cell : intersectedCellsIntersectionInfo)
    {
        double skinFactor = wellPath->fishbonesCollection()->mainBoreSkinFactor();
        QString completionMetaData = wellPath->name() + " main bore";
        WellBorePartForTransCalc wellBorePart = WellBorePartForTransCalc(cell.internalCellLengths,
                                                                         holeDiameter / 2,
                                                                         skinFactor,
                                                                         isMainBore,
                                                                         completionMetaData);

        wellBorePartsInCells[cell.cellIndex].push_back(wellBorePart);
    }
}
