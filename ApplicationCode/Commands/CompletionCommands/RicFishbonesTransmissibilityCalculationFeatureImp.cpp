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

#include "RigEclipseCaseData.h"
#include "RicExportCompletionDataSettingsUi.h"
#include "RicWellPathExportCompletionDataFeature.h"
#include "RimWellPath.h"
#include "RigWellPath.h"
#include "RimFishboneWellPath.h"
#include "RimFishbonesCollection.h"
#include "RigActiveCellInfo.h"
#include "RigMainGrid.h"
#include "RimFishbonesMultipleSubs.h"
#include "RimFishboneWellPathCollection.h"
#include "RimWellPathCompletions.h"

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RigCompletionData> RicFishbonesTransmissibilityCalculationFeatureImp::generateFishboneLateralsCompdatValues(const RimWellPath* wellPath, const RicExportCompletionDataSettingsUi& settings)
{
    // Generate data
    const RigEclipseCaseData* caseData =  settings.caseToApply()->eclipseCaseData();
    std::vector<WellSegmentLocation> locations = RicWellPathExportCompletionDataFeature::findWellSegmentLocations(settings.caseToApply, wellPath);

    // Filter out cells where main bore is present
    if (settings.removeLateralsInMainBoreCells())
    {
        std::vector<size_t> wellPathCells = RicWellPathExportCompletionDataFeature::findIntersectingCells(caseData, wellPath->wellPathGeometry()->m_wellPathPoints);
        RicWellPathExportCompletionDataFeature::markWellPathCells(wellPathCells, &locations);
    }

    RigMainGrid* grid = settings.caseToApply->eclipseCaseData()->mainGrid();

    std::vector<RigCompletionData> completionData;

    for (const WellSegmentLocation& location : locations)
    {
        for (const WellSegmentLateral& lateral : location.laterals)
        {
            for (const WellSegmentLateralIntersection& intersection : lateral.intersections)
            {
                if (intersection.mainBoreCell && settings.removeLateralsInMainBoreCells()) continue;

                size_t i, j, k;
                grid->ijkFromCellIndex(intersection.cellIndex, &i, &j, &k);
                RigCompletionData completion(wellPath->completions()->wellNameForExport(), IJKCellIndex(i, j, k));
                completion.addMetadata(location.fishbonesSubs->name(), QString("Sub: %1 Lateral: %2").arg(location.subIndex).arg(lateral.lateralIndex));
                double diameter = location.fishbonesSubs->holeDiameter() / 1000;
                if (settings.computeTransmissibility())
                {
                    double transmissibility = RicWellPathExportCompletionDataFeature::calculateTransmissibility(settings.caseToApply,
                                                                        wellPath,
                                                                        intersection.lengthsInCell,
                                                                        location.fishbonesSubs->skinFactor(),
                                                                        diameter / 2,
                                                                        intersection.cellIndex);
                    completion.setFromFishbone(transmissibility, location.fishbonesSubs->skinFactor());
                }
                else {
                    CellDirection direction = RicWellPathExportCompletionDataFeature::calculateDirectionInCell(settings.caseToApply, intersection.cellIndex, intersection.lengthsInCell);
                    completion.setFromFishbone(diameter, direction);
                }
                completionData.push_back(completion);
            }
        }
    }

    return completionData;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicFishbonesTransmissibilityCalculationFeatureImp::findFishboneLateralsWellBoreParts(std::map<size_t, std::vector<WellBorePartForTransCalc> >& wellBorePartsInCells, const RimWellPath* wellPath, const RicExportCompletionDataSettingsUi& settings)
{
    // Generate data
    const RigEclipseCaseData* caseData = settings.caseToApply()->eclipseCaseData();
    std::vector<WellSegmentLocation> locations = RicWellPathExportCompletionDataFeature::findWellSegmentLocations(settings.caseToApply, wellPath);

    // Filter out cells where main bore is present
    if (settings.removeLateralsInMainBoreCells())
    {
        std::vector<size_t> wellPathCells = RicWellPathExportCompletionDataFeature::findIntersectingCells(caseData, wellPath->wellPathGeometry()->m_wellPathPoints);
        RicWellPathExportCompletionDataFeature::markWellPathCells(wellPathCells, &locations);
    }

    RigMainGrid* grid = settings.caseToApply->eclipseCaseData()->mainGrid();

    std::vector<RigCompletionData> completionData;

    for (const WellSegmentLocation& location : locations)
    {
        for (const WellSegmentLateral& lateral : location.laterals)
        {
            for (const WellSegmentLateralIntersection& intersection : lateral.intersections)
            {
                if (intersection.mainBoreCell && settings.removeLateralsInMainBoreCells()) continue;

                double diameter = location.fishbonesSubs->holeDiameter() / 1000;
                QString completionMetaData = (location.fishbonesSubs->name(), QString("Sub: %1 Lateral: %2").arg(location.subIndex).arg(lateral.lateralIndex));
                WellBorePartForTransCalc wellBorePart = WellBorePartForTransCalc(intersection.lengthsInCell, 
                                                                                 diameter / 2, 
                                                                                 location.fishbonesSubs->skinFactor(), 
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
    findMainWellBoreParts(wellBorePartsInCells, wellPath, settings);

    std::vector<RigCompletionData> completionData;

    RigMainGrid* grid = settings.caseToApply->eclipseCaseData()->mainGrid();
    const RigActiveCellInfo* activeCellInfo = settings.caseToApply->eclipseCaseData()->activeCellInfo(RifReaderInterface::MATRIX_RESULTS);

    for (auto cellAndWellBoreParts : wellBorePartsInCells)
    {
        size_t cellIndex = cellAndWellBoreParts.first;
        std::vector<WellBorePartForTransCalc> wellBoreParts = cellAndWellBoreParts.second;
        size_t i, j, k;
        grid->ijkFromCellIndex(cellIndex, &i, &j, &k);

        bool cellIsActive = activeCellInfo->isActive(cellIndex);
        if (!cellIsActive) continue;
        
        size_t NumberOfCellContributions = wellBoreParts.size();
        //Simplest implementation possible, this can be improved later
        QString directionToSplitCellVolume = "DX";

        for (WellBorePartForTransCalc wellBorePart : wellBoreParts)
        {
            RigCompletionData completion(wellPath->completions()->wellNameForExport(), IJKCellIndex(i, j, k));
            completion.addMetadata(wellBorePart.metaData, "");
            if (settings.computeTransmissibility())
            {
                double transmissibility = RicWellPathExportCompletionDataFeature::calculateTransmissibility(settings.caseToApply,
                                                                                                            wellPath,
                                                                                                            wellBorePart.lengthsInCell,
                                                                                                            wellBorePart.skinFactor,
                                                                                                            wellBorePart.wellRadius,
                                                                                                            cellIndex,
                                                                                                            NumberOfCellContributions,
                                                                                                            directionToSplitCellVolume);
                completion.setFromFishbone(transmissibility, wellBorePart.skinFactor);
            }
            else 
            {
                CellDirection direction = RicWellPathExportCompletionDataFeature::calculateDirectionInCell(settings.caseToApply, cellIndex, wellBorePart.lengthsInCell);
                completion.setFromFishbone(wellBorePart.wellRadius*2, direction);
            }
            completionData.push_back(completion);
        }
    }
    return completionData;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RigCompletionData> RicFishbonesTransmissibilityCalculationFeatureImp::generateFishbonesImportedLateralsCompdatValues(const RimWellPath* wellPath, 
                                                                                                                                 const RicExportCompletionDataSettingsUi& settings)
{
    std::vector<RigCompletionData> completionData;

    std::vector<size_t> wellPathCells = RicWellPathExportCompletionDataFeature::findIntersectingCells(settings.caseToApply()->eclipseCaseData(), wellPath->wellPathGeometry()->m_wellPathPoints);

    double diameter = wellPath->fishbonesCollection()->wellPathCollection()->holeDiameter() / 1000;
    for (const RimFishboneWellPath* fishbonesPath : wellPath->fishbonesCollection()->wellPathCollection()->wellPaths())
    {
        std::vector<WellPathCellIntersectionInfo> intersectedCells = RigWellPathIntersectionTools::findCellsIntersectedByPath(settings.caseToApply->eclipseCaseData(), fishbonesPath->coordinates());
        for (auto& cell : intersectedCells)
        {
            if (settings.removeLateralsInMainBoreCells && std::find(wellPathCells.begin(), wellPathCells.end(), cell.cellIndex) != wellPathCells.end()) continue;

            size_t i, j, k;
            settings.caseToApply->eclipseCaseData()->mainGrid()->ijkFromCellIndex(cell.cellIndex, &i, &j, &k);
            RigCompletionData completion(wellPath->completions()->wellNameForExport(), IJKCellIndex(i, j, k));
            completion.addMetadata(fishbonesPath->name(), "");
            if (settings.computeTransmissibility())
            {
                double skinFactor = wellPath->fishbonesCollection()->wellPathCollection()->skinFactor();
                double transmissibility = RicWellPathExportCompletionDataFeature::calculateTransmissibility(settings.caseToApply(),
                                                                                                            wellPath,
                                                                                                            cell.internalCellLengths,
                                                                                                            skinFactor,
                                                                                                            diameter / 2,
                                                                                                            cell.cellIndex);
                completion.setFromFishbone(transmissibility, skinFactor);
            }
            else {
                CellDirection direction = RicWellPathExportCompletionDataFeature::calculateDirectionInCell(settings.caseToApply, cell.cellIndex, cell.internalCellLengths);
                completion.setFromFishbone(diameter, direction);
            }
            completionData.push_back(completion);
        }
    }
    return completionData;
}


void RicFishbonesTransmissibilityCalculationFeatureImp::findFishboneImportedLateralsWellBoreParts(std::map<size_t, std::vector<WellBorePartForTransCalc> >& wellBorePartsInCells, const RimWellPath* wellPath, const RicExportCompletionDataSettingsUi& settings)
{
    std::vector<size_t> wellPathCells = RicWellPathExportCompletionDataFeature::findIntersectingCells(settings.caseToApply()->eclipseCaseData(), wellPath->wellPathGeometry()->m_wellPathPoints);

    double diameter = wellPath->fishbonesCollection()->wellPathCollection()->holeDiameter() / 1000;
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
                                                                             completionMetaData);
            wellBorePartsInCells[cell.cellIndex].push_back(wellBorePart); 
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicFishbonesTransmissibilityCalculationFeatureImp::findMainWellBoreParts(std::map<size_t, std::vector<WellBorePartForTransCalc>>& wellBorePartsInCells, const RimWellPath* wellPath, const RicExportCompletionDataSettingsUi& settings)
{
    double holeDiameter = wellPath->fishbonesCollection()->wellPathCollection()->holeDiameter();
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
        double skinFactor = wellPath->fishbonesCollection()->wellPathCollection()->skinFactor();
        QString completionMetaData = wellPath->name() + " main bore";
        WellBorePartForTransCalc wellBorePart = WellBorePartForTransCalc(cell.internalCellLengths,
                                                                         holeDiameter / 2,
                                                                         skinFactor,
                                                                         completionMetaData);

        wellBorePartsInCells[cell.cellIndex].push_back(wellBorePart);
    }
}