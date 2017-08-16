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

#include "RicWellPathExportCompletionDataFeature.h"

#include "RiaApplication.h"
#include "RiaLogging.h"

#include "RicExportCompletionDataSettingsUi.h"
#include "RicExportFeatureImpl.h"
#include "RicFishbonesTransmissibilityCalculationFeatureImp.h"

#include "RigActiveCellInfo.h"
#include "RigEclipseCaseData.h"
#include "RigMainGrid.h"
#include "RigResultAccessorFactory.h"
#include "RigTransmissibilityEquations.h"
#include "RigWellLogExtractionTools.h"
#include "RigWellPath.h"
#include "RigWellPathIntersectionTools.h"

#include "RimFishboneWellPath.h"
#include "RimFishboneWellPathCollection.h"
#include "RimFishbonesCollection.h"
#include "RimFishbonesMultipleSubs.h"
#include "RimPerforationCollection.h"
#include "RimPerforationInterval.h"
#include "RimProject.h"
#include "RimReservoirCellResultsStorage.h"
#include "RimWellPath.h"
#include "RimWellPathCollection.h"
#include "RimWellPathCompletions.h"

#include "RiuMainWindow.h"

#include "cafPdmUiPropertyViewDialog.h"
#include "cafSelectionManager.h"

#include "cvfPlane.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

CAF_CMD_SOURCE_INIT(RicWellPathExportCompletionDataFeature, "RicWellPathExportCompletionDataFeature");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicWellPathExportCompletionDataFeature::isCommandEnabled()
{
    return !selectedWellPaths().empty();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicWellPathExportCompletionDataFeature::onActionTriggered(bool isChecked)
{
    std::vector<RimWellPath*> wellPaths = selectedWellPaths();

    CVF_ASSERT(wellPaths.size() > 0);

    RiaApplication* app = RiaApplication::instance();

    QString projectFolder = app->currentProjectPath();
    QString defaultDir = RiaApplication::instance()->lastUsedDialogDirectoryWithFallback("COMPLETIONS", projectFolder);
    
    bool onlyWellPathCollectionSelected = noWellPathsSelectedDirectly();

    RicExportCompletionDataSettingsUi exportSettings(onlyWellPathCollectionSelected);
    std::vector<RimCase*> cases;
    app->project()->allCases(cases);
    for (auto c : cases)
    {
        RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(c);
        if (eclipseCase != nullptr)
        {
            exportSettings.caseToApply = eclipseCase;
            break;
        }
    }

    exportSettings.folder = defaultDir;

    caf::PdmUiPropertyViewDialog propertyDialog(RiuMainWindow::instance(), &exportSettings, "Export Completion Data", "");
    RicExportFeatureImpl::configureForExport(&propertyDialog);

    if (propertyDialog.exec() == QDialog::Accepted)
    {
        RiaApplication::instance()->setLastUsedDialogDirectory("COMPLETIONS", exportSettings.folder);

        exportCompletions(wellPaths, exportSettings);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicWellPathExportCompletionDataFeature::setupActionLook(QAction* actionToSetup)
{
    actionToSetup->setText("Export Completion Data");
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RimWellPath*> RicWellPathExportCompletionDataFeature::selectedWellPaths()
{
    std::vector<RimWellPath*> wellPaths;
    caf::SelectionManager::instance()->objectsByType(&wellPaths);

    if (wellPaths.empty())
    {
        std::vector<RimWellPathCollection*> wellPathCollections;
        caf::SelectionManager::instance()->objectsByType(&wellPathCollections);

        for (auto wellPathCollection : wellPathCollections)
        {
            for (auto wellPath : wellPathCollection->wellPaths())
            {
                wellPaths.push_back(wellPath);
            }
        }
    }

    std::set<RimWellPath*> uniqueWellPaths(wellPaths.begin(), wellPaths.end());
    wellPaths.assign(uniqueWellPaths.begin(), uniqueWellPaths.end());
    return wellPaths;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicWellPathExportCompletionDataFeature::noWellPathsSelectedDirectly()
{
    std::vector<RimWellPath*> wellPaths;
    caf::SelectionManager::instance()->objectsByType(&wellPaths);

    if (wellPaths.empty()) return true;
    else return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicWellPathExportCompletionDataFeature::exportCompletions(const std::vector<RimWellPath*>& wellPaths, 
                                                               const RicExportCompletionDataSettingsUi& exportSettings)
{

    if (exportSettings.caseToApply() == nullptr)
    {
        RiaLogging::error("Export Completions Data: Cannot export completions data without specified eclipse case");
        return;
    }

    std::vector<RimWellPath*> usedWellPaths;
    if (exportSettings.wellSelection == RicExportCompletionDataSettingsUi::ALL_WELLS
        || exportSettings.wellSelection == RicExportCompletionDataSettingsUi::SELECTED_WELLS)
    {
        usedWellPaths = wellPaths;
    }
    else if (exportSettings.wellSelection == RicExportCompletionDataSettingsUi::CHECKED_WELLS)
    {
        for (auto wellPath : wellPaths)
        {
            if (wellPath->showWellPath)
            {
                usedWellPaths.push_back(wellPath);
            }
        }
    }

    {
        bool unitSystemMismatch = false;
        for (const RimWellPath* wellPath : usedWellPaths)
        {
            if (wellPath->unitSystem() != exportSettings.caseToApply->eclipseCaseData()->unitsType())
            {
                unitSystemMismatch = true;
                break;
            }
        }
        if (unitSystemMismatch)
        {
            RiaLogging::error("Well path unit systems must match unit system of chosen eclipse case.");
            return;
        }
    }
    
    std::map<IJKCellIndex, std::vector<RigCompletionData> > completionsPerEclipseCell;

    for (auto wellPath : usedWellPaths)
    {
        // Generate completion data

        if (exportSettings.includePerforations)
        {
            std::vector<RigCompletionData> perforationCompletionData = generatePerforationsCompdatValues(wellPath, exportSettings);
            appendCompletionData(&completionsPerEclipseCell, perforationCompletionData);
        }
        if (exportSettings.includeFishbones)
        {
            std::vector<RigCompletionData> fishbonesCompletionData = RicFishbonesTransmissibilityCalculationFeatureImp::generateFishboneCompdatValuesUsingAdjustedCellVolume(wellPath, exportSettings);
            appendCompletionData(&completionsPerEclipseCell, fishbonesCompletionData);
        }
    }

    const QString eclipseCaseName = exportSettings.caseToApply->caseUserDescription();

    if (exportSettings.fileSplit == RicExportCompletionDataSettingsUi::UNIFIED_FILE)
    {
        std::vector<RigCompletionData> completions;
        for (auto& data : completionsPerEclipseCell)
        {
            completions.push_back(combineEclipseCellCompletions(data.second, exportSettings));
        }

        const QString fileName = QString("UnifiedCompletions_%1").arg(eclipseCaseName);
        printCompletionsToFile(exportSettings.folder, fileName, completions, exportSettings.compdatExport);
    }
    else if (exportSettings.fileSplit == RicExportCompletionDataSettingsUi::SPLIT_ON_WELL)
    {
        for (auto wellPath : usedWellPaths)
        {
            std::map<IJKCellIndex, std::vector<RigCompletionData> > filteredWellCompletions = getCompletionsForWell(completionsPerEclipseCell, wellPath->completions()->wellNameForExport());
            std::vector<RigCompletionData> completions;
            for (auto& data : filteredWellCompletions)
            {
                completions.push_back(combineEclipseCellCompletions(data.second, exportSettings));
            }
            std::vector<RigCompletionData> wellCompletions;
            for (auto completion : completions)
            {
                if (completion.wellName() == wellPath->completions()->wellNameForExport())
                {
                    wellCompletions.push_back(completion);
                }
            }

            QString fileName = QString("%1_unifiedCompletions_%2").arg(wellPath->name()).arg(eclipseCaseName);
            printCompletionsToFile(exportSettings.folder, fileName, wellCompletions, exportSettings.compdatExport);
        }
    }
    else if (exportSettings.fileSplit == RicExportCompletionDataSettingsUi::SPLIT_ON_WELL_AND_COMPLETION_TYPE)
    {
        for (auto wellPath : usedWellPaths)
        {
            std::map<IJKCellIndex, std::vector<RigCompletionData> > filteredWellCompletions = getCompletionsForWell(completionsPerEclipseCell, wellPath->completions()->wellNameForExport());
            std::vector<RigCompletionData> completions;
            for (auto& data : filteredWellCompletions)
            {
                completions.push_back(combineEclipseCellCompletions(data.second, exportSettings));
            }
            {
                std::vector<RigCompletionData> fishbonesCompletions = getCompletionsForWellAndCompletionType(completions, wellPath->completions()->wellNameForExport(), RigCompletionData::FISHBONES);
                QString fileName = QString("%1_Fishbones_%2").arg(wellPath->name()).arg(eclipseCaseName);
                printCompletionsToFile(exportSettings.folder, fileName, fishbonesCompletions, exportSettings.compdatExport);
            }
            {
                std::vector<RigCompletionData> perforationCompletions = getCompletionsForWellAndCompletionType(completions, wellPath->completions()->wellNameForExport(), RigCompletionData::PERFORATION);
                QString fileName = QString("%1_Perforations_%2").arg(wellPath->name()).arg(eclipseCaseName);
                printCompletionsToFile(exportSettings.folder, fileName, perforationCompletions, exportSettings.compdatExport);
            }
        }
    }
}

//==================================================================================================
/// 
//==================================================================================================
RigCompletionData RicWellPathExportCompletionDataFeature::combineEclipseCellCompletions(const std::vector<RigCompletionData>& completions,
                                                                                        const RicExportCompletionDataSettingsUi& settings)
{
    CVF_ASSERT(!completions.empty());
    QString wellName = completions[0].wellName();
    IJKCellIndex cellIndexIJK = completions[0].cellIndex();
    RigMainGrid* grid = settings.caseToApply->eclipseCaseData()->mainGrid();
    size_t cellIndex = grid->cellIndexFromIJK(cellIndexIJK.i, cellIndexIJK.j, cellIndexIJK.k);
    RigCompletionData::CompletionType completionType = completions[0].completionType();

    //completion type, skin factor, well bore diameter and cell direction are taken from (first) main bore, 
    //if no main bore they are taken from first completion
    double skinfactor = completions[0].skinFactor();
    double wellBoreDiameter = completions[0].diameter();
    CellDirection cellDirection = completions[0].direction();
    
    for (const RigCompletionData& completion : completions)
    {
        if (completion.isMainBore())
        {
            skinfactor = completion.skinFactor();
            wellBoreDiameter = completion.diameter();
            cellDirection = completion.direction();
            break;
        }
    }


    RigCompletionData resultCompletion(wellName, cellIndexIJK);

    double totalTrans = 0.0;

    for (const RigCompletionData& completion : completions)
    {
        if (completion.completionType() != completions[0].completionType())
        {
            RiaLogging::error(QString("Cannot combine completions of different types in same cell [%1, %2, %3]").arg(cellIndexIJK.i).arg(cellIndexIJK.j).arg(cellIndexIJK.k));
            return resultCompletion; //Returning empty completion, should not be exported
        }

        if (completion.wellName() != completions[0].wellName())
        {
            RiaLogging::error(QString("Cannot combine completions from different wells in same cell [%1, %2, %3]").arg(cellIndexIJK.i).arg(cellIndexIJK.j).arg(cellIndexIJK.k));
            return resultCompletion; //Returning empty completion, should not be exported
        }
        
        if (completion.transmissibility() == HUGE_VAL)
        {
            RiaLogging::error(QString("Transmissibility calculation has failed for cell [%1, %2, %3]").arg(cellIndexIJK.i).arg(cellIndexIJK.j).arg(cellIndexIJK.k));
            return resultCompletion; //Returning empty completion, should not be exported
        }       

        if (settings.excludeMainBoreForFishbones && completionType == RigCompletionData::FISHBONES && completion.isMainBore())
        {
            continue;
        }

        totalTrans = totalTrans + completion.transmissibility();

        resultCompletion.m_metadata.reserve(resultCompletion.m_metadata.size() + completion.m_metadata.size());
        resultCompletion.m_metadata.insert(resultCompletion.m_metadata.end(), completion.m_metadata.begin(), completion.m_metadata.end());

    }


    if (settings.compdatExport == RicExportCompletionDataSettingsUi::TRANSMISSIBILITIES) 
    {
        resultCompletion.setCombinedValuesExplicitTrans(totalTrans, completionType);
    }

    if (settings.compdatExport == RicExportCompletionDataSettingsUi::WPIMULT_AND_DEFAULT_CONNECTION_FACTORS) 
    {
        //calculate trans for main bore - but as Eclipse will do it!      
        double transmissibilityEclipseCalculation = RicWellPathExportCompletionDataFeature::calculateTransmissibilityAsEclipseDoes(settings.caseToApply(),
                                                                                                                                   skinfactor,
                                                                                                                                   wellBoreDiameter / 2,
                                                                                                                                   cellIndex,
                                                                                                                                   cellDirection);

        double wpimult = totalTrans / transmissibilityEclipseCalculation;
        resultCompletion.setCombinedValuesImplicitTransWPImult(wpimult, cellDirection, skinfactor, wellBoreDiameter, completionType);
    }
        
    return resultCompletion;
}





//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicWellPathExportCompletionDataFeature::printCompletionsToFile(const QString& folderName, const QString& fileName, std::vector<RigCompletionData>& completions, RicExportCompletionDataSettingsUi::CompdatExportType exportType)
{
    //TODO: Check that completion is ready for export

    QDir exportFolder = QDir(folderName);

    if (!exportFolder.exists())
    {
        bool createdPath = exportFolder.mkpath(folderName);
        if (createdPath) RiaLogging::info("Created export folder " + folderName);
        else RiaLogging::error("Selected output folder does not exist, and could not be created.");
    }

    QString filePath = exportFolder.filePath(fileName);
    QFile exportFile(filePath);
        if (!exportFile.open(QIODevice::WriteOnly))
    {
        RiaLogging::error(QString("Export Completions Data: Could not open the file: %1").arg(filePath));
        return;
    }

    QTextStream stream(&exportFile);
    RifEclipseDataTableFormatter formatter(stream);

    // Sort by well name / cell index
    std::sort(completions.begin(), completions.end());

    // Print completion data
    generateCompdatTable(formatter, completions);

    
    if (exportType == RicExportCompletionDataSettingsUi::WPIMULT_AND_DEFAULT_CONNECTION_FACTORS)
    {
        generateWpimultTable(formatter, completions);
    }

    RiaLogging::info(QString("Successfully exported completion data to %1").arg(filePath));
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RigCompletionData> RicWellPathExportCompletionDataFeature::getCompletionsForWellAndCompletionType(const std::vector<RigCompletionData>& completions,
                                                                                                              const QString& wellName,
                                                                                                              RigCompletionData::CompletionType completionType)
{
    std::vector<RigCompletionData> filteredCompletions;
    for (auto completion : completions)
    {
        if (completion.wellName() == wellName && completion.completionType() == completionType)
        {
            filteredCompletions.push_back(completion);
        }
    }
    return filteredCompletions;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::map<IJKCellIndex, std::vector<RigCompletionData> > RicWellPathExportCompletionDataFeature::getCompletionsForWell(const std::map<IJKCellIndex, std::vector<RigCompletionData>>& cellToCompletionMap, const QString& wellName)
{
    std::map<IJKCellIndex, std::vector<RigCompletionData> > wellCompletions;

    for (const auto& it : cellToCompletionMap)
    {
        for (auto& completion : it.second)
        {
            if (completion.wellName() == wellName)
            {
                wellCompletions[it.first].push_back(completion);
            }
        }
    }

    return wellCompletions;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicWellPathExportCompletionDataFeature::generateCompdatTable(RifEclipseDataTableFormatter& formatter, const std::vector<RigCompletionData>& completionData)
{
    std::vector<RifEclipseOutputTableColumn> header = {
        RifEclipseOutputTableColumn("Well"),
        RifEclipseOutputTableColumn("I"),
        RifEclipseOutputTableColumn("J"),
        RifEclipseOutputTableColumn("K1"),
        RifEclipseOutputTableColumn("K2"),
        RifEclipseOutputTableColumn("Status"),
        RifEclipseOutputTableColumn("SAT"),
        RifEclipseOutputTableColumn("TR", RifEclipseOutputTableDoubleFormatting(RifEclipseOutputTableDoubleFormat::RIF_SCIENTIFIC)),
        RifEclipseOutputTableColumn("DIAM"),
        RifEclipseOutputTableColumn("KH"),
        RifEclipseOutputTableColumn("S"),
        RifEclipseOutputTableColumn("Df"),
        RifEclipseOutputTableColumn("DIR"),
        RifEclipseOutputTableColumn("r0")
    };

    formatter.keyword("COMPDAT");
    formatter.header(header);

    for (const RigCompletionData& data : completionData)
    {
        if (data.transmissibility() == 0.0 || data.wpimult()==0.0)
        {
            //Don't export completions without transmissibility
            continue;
        }

        for (const RigCompletionMetaData& metadata : data.metadata())
        {
            formatter.comment(QString("%1 : %2").arg(metadata.name).arg(metadata.comment));
        }
        formatter.add(data.wellName());
        formatter.addZeroBasedCellIndex(data.cellIndex().i).addZeroBasedCellIndex(data.cellIndex().j).addZeroBasedCellIndex(data.cellIndex().k).addZeroBasedCellIndex(data.cellIndex().k);
        switch (data.connectionState())
        {
        case OPEN:
            formatter.add("OPEN");
            break;
        case SHUT:
            formatter.add("SHUT");
            break;
        case AUTO:
            formatter.add("AUTO");
            break;
        }
        if (RigCompletionData::isDefaultValue(data.saturation())) formatter.add("1*"); else formatter.add(data.saturation());
        if (RigCompletionData::isDefaultValue(data.transmissibility()))
        {
            formatter.add("1*"); // Transmissibility

            if (RigCompletionData::isDefaultValue(data.diameter())) formatter.add("1*"); else formatter.add(data.diameter());
            if (RigCompletionData::isDefaultValue(data.kh())) formatter.add("1*"); else formatter.add(data.kh());
            if (RigCompletionData::isDefaultValue(data.skinFactor())) formatter.add("1*"); else formatter.add(data.skinFactor());
            if (RigCompletionData::isDefaultValue(data.dFactor())) formatter.add("1*"); else formatter.add(data.dFactor());

            switch (data.direction())
            {
            case DIR_I:
                formatter.add("'X'");
                break;
            case DIR_J:
                formatter.add("'Y'");
                break;
            case DIR_K:
                formatter.add("'Z'");
                break;
            default:
                formatter.add("'Z'");
                break;
            }
        }
        else
        {
            formatter.add(data.transmissibility());
        }

        formatter.rowCompleted();
    }
    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicWellPathExportCompletionDataFeature::generateWpimultTable(RifEclipseDataTableFormatter& formatter, const std::vector<RigCompletionData>& completionData)
{
    std::vector<RifEclipseOutputTableColumn> header = {
        RifEclipseOutputTableColumn("Well"),
        RifEclipseOutputTableColumn("Mult"),
        RifEclipseOutputTableColumn("I"),
        RifEclipseOutputTableColumn("J"),
        RifEclipseOutputTableColumn("K"),
    };
    formatter.keyword("WPIMULT");
    formatter.header(header);

    for (auto& completion : completionData)
    {
        if (completion.wpimult() == 0.0)
        {
            continue;
        }

        formatter.add(completion.wellName());
        formatter.add(completion.wpimult());
        formatter.addZeroBasedCellIndex(completion.cellIndex().i).addZeroBasedCellIndex(completion.cellIndex().j).addZeroBasedCellIndex(completion.cellIndex().k);
        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<RigCompletionData> RicWellPathExportCompletionDataFeature::generatePerforationsCompdatValues(const RimWellPath* wellPath, const RicExportCompletionDataSettingsUi& settings)
{
    RiaEclipseUnitTools::UnitSystem unitSystem = settings.caseToApply->eclipseCaseData()->unitsType();

    std::vector<RigCompletionData> completionData;
    const RigActiveCellInfo* activeCellInfo = settings.caseToApply->eclipseCaseData()->activeCellInfo(RiaDefines::MATRIX_MODEL);


    for (const RimPerforationInterval* interval : wellPath->perforationIntervalCollection()->perforations())
    {
        if (!interval->isActiveOnDate(settings.caseToApply->timeStepDates()[settings.timeStep])) continue;

        std::vector<cvf::Vec3d> perforationPoints = wellPath->wellPathGeometry()->clippedPointSubset(interval->startMD(), interval->endMD());
        std::vector<WellPathCellIntersectionInfo> intersectedCells = RigWellPathIntersectionTools::findCellsIntersectedByPath(settings.caseToApply->eclipseCaseData(), perforationPoints);
        for (auto& cell : intersectedCells)
        {
            bool cellIsActive = activeCellInfo->isActive(cell.cellIndex);
            if (!cellIsActive) continue;

            size_t i, j, k;
            settings.caseToApply->eclipseCaseData()->mainGrid()->ijkFromCellIndex(cell.cellIndex, &i, &j, &k);
            RigCompletionData completion(wellPath->completions()->wellNameForExport(), IJKCellIndex(i, j, k));
            CellDirection direction = calculateDirectionInCell(settings.caseToApply, cell.cellIndex, cell.internalCellLengths);

            double transmissibility = RicWellPathExportCompletionDataFeature::calculateTransmissibility(settings.caseToApply,
                                                                                                        wellPath,
                                                                                                        cell.internalCellLengths,
                                                                                                        interval->skinFactor(),
                                                                                                        interval->diameter(unitSystem) / 2,
                                                                                                        cell.cellIndex);
              


            completion.setTransAndWPImultBackgroundDataFromPerforation(transmissibility, 
                                                                       interval->skinFactor(), 
                                                                       interval->diameter(unitSystem),
                                                                       direction);
            completion.addMetadata("Perforation", QString("StartMD: %1 - EndMD: %2").arg(interval->startMD()).arg(interval->endMD()) + QString(" : ") + QString::number(transmissibility));
            completionData.push_back(completion);
        }
    }

    return completionData;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<size_t> RicWellPathExportCompletionDataFeature::findIntersectingCells(const RigEclipseCaseData* caseData, const std::vector<cvf::Vec3d>& coords)
{
    std::set<size_t> cells;

    std::vector<HexIntersectionInfo> intersections = RigWellPathIntersectionTools::getIntersectedCells(caseData->mainGrid(), coords);
    for (auto intersection : intersections)
    {
        cells.insert(intersection.m_hexIndex);
    }

    // Ensure only unique cells are included
    std::vector<size_t> cellsVector;
    cellsVector.assign(cells.begin(), cells.end());
    // Sort cells
    std::sort(cellsVector.begin(), cellsVector.end());
    return cellsVector;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportCompletionDataFeature::markWellPathCells(const std::vector<size_t>& wellPathCells, std::vector<WellSegmentLocation>* locations)
{
    std::set<size_t> wellPathCellSet(wellPathCells.begin(), wellPathCells.end());
    for (WellSegmentLocation& location : *locations)
    {
        for (WellSegmentLateral& lateral : location.laterals)
        {
            for (WellSegmentLateralIntersection& intersection : lateral.intersections)
            {
                if (wellPathCellSet.find(intersection.cellIndex) != wellPathCellSet.end())
                {
                    intersection.mainBoreCell = true;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicWellPathExportCompletionDataFeature::wellSegmentLocationOrdering(const WellSegmentLocation& first, const WellSegmentLocation& second)
{
    return first.measuredDepth < second.measuredDepth;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicWellPathExportCompletionDataFeature::isPointBetween(const cvf::Vec3d& startPoint, const cvf::Vec3d& endPoint, const cvf::Vec3d& pointToCheck)
{
    cvf::Plane plane;
    plane.setFromPointAndNormal(pointToCheck, endPoint - startPoint);
    return plane.side(startPoint) != plane.side(endPoint);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<WellSegmentLocation> RicWellPathExportCompletionDataFeature::findWellSegmentLocations(const RimEclipseCase* caseToApply, const RimWellPath* wellPath)
{
    std::vector<RimFishbonesMultipleSubs*> fishbonesSubs;
    for (RimFishbonesMultipleSubs* subs : wellPath->fishbonesCollection()->fishbonesSubs())
    {
        fishbonesSubs.push_back(subs);
    }
    return findWellSegmentLocations(caseToApply, wellPath, fishbonesSubs);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<WellSegmentLocation> RicWellPathExportCompletionDataFeature::findWellSegmentLocations(const RimEclipseCase* caseToApply, const RimWellPath* wellPath, const std::vector<RimFishbonesMultipleSubs*>& fishbonesSubs)
{
    std::vector<WellSegmentLocation> wellSegmentLocations;
    for (RimFishbonesMultipleSubs* subs : fishbonesSubs)
    {
        for (auto& sub : subs->installedLateralIndices())
        {
            double measuredDepth = subs->measuredDepth(sub.subIndex);
            cvf::Vec3d position = wellPath->wellPathGeometry()->interpolatedPointAlongWellPath(measuredDepth);
            WellSegmentLocation location = WellSegmentLocation(subs, measuredDepth, -position.z(), sub.subIndex);
            for (size_t lateralIndex : sub.lateralIndices)
            {
                location.laterals.push_back(WellSegmentLateral(lateralIndex));
            }
            wellSegmentLocations.push_back(location);
        }
    }
    std::sort(wellSegmentLocations.begin(), wellSegmentLocations.end(), wellSegmentLocationOrdering);

    assignBranchAndSegmentNumbers(caseToApply, &wellSegmentLocations);

    return wellSegmentLocations;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicWellPathExportCompletionDataFeature::calculateLateralIntersections(const RimEclipseCase* caseToApply, WellSegmentLocation* location, int* branchNum, int* segmentNum)
{
    for (WellSegmentLateral& lateral : location->laterals)
    {
        lateral.branchNumber = ++(*branchNum);
        std::vector<cvf::Vec3d> lateralCoords = location->fishbonesSubs->coordsForLateral(location->subIndex, lateral.lateralIndex);
        std::vector<WellPathCellIntersectionInfo> intersections = RigWellPathIntersectionTools::findCellsIntersectedByPath(caseToApply->eclipseCaseData(), lateralCoords);

        auto intersection = intersections.cbegin();
        double length = 0;
        double depth = 0;
        cvf::Vec3d startPoint = lateralCoords[0];
        int attachedSegmentNumber = location->icdSegmentNumber;

        for (size_t i = 1; i < lateralCoords.size() && intersection != intersections.cend(); ++i)
        {
            if (isPointBetween(startPoint, lateralCoords[i], intersection->endPoint))
            {
                length += (intersection->endPoint - startPoint).length();
                depth += intersection->endPoint.z() - startPoint.z();

                WellSegmentLateralIntersection lateralIntersection(++(*segmentNum), attachedSegmentNumber, intersection->cellIndex, length, depth);
                lateralIntersection.lengthsInCell = intersection->internalCellLengths;
                lateral.intersections.push_back(lateralIntersection);

                length = 0;
                depth = 0;
                startPoint = intersection->endPoint;
                attachedSegmentNumber = *segmentNum;
                ++intersection;
            }
            else
            {
                length += (lateralCoords[i] - startPoint).length();
                depth += lateralCoords[i].z() - startPoint.z();
                startPoint = lateralCoords[i];
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicWellPathExportCompletionDataFeature::assignBranchAndSegmentNumbers(const RimEclipseCase* caseToApply, std::vector<WellSegmentLocation>* locations)
{
    int segmentNumber = 1;
    int branchNumber = 1;
    // First loop over the locations so that each segment on the main stem is an incremental number
    for (WellSegmentLocation& location : *locations)
    {
        location.segmentNumber = ++segmentNumber;
        location.icdBranchNumber = ++branchNumber;
        location.icdSegmentNumber = ++segmentNumber;
    }
    // Then assign branch and segment numbers to each lateral parts
    for (WellSegmentLocation& location : *locations)
    {
        calculateLateralIntersections(caseToApply, &location, &branchNumber, &segmentNumber);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicWellPathExportCompletionDataFeature::appendCompletionData(std::map<IJKCellIndex, std::vector<RigCompletionData> >* completionData, const std::vector<RigCompletionData>& data)
{
    for (auto& completion : data)
    {
        auto it = completionData->find(completion.cellIndex());
        if (it != completionData->end())
        {
            it->second.push_back(completion);
        }
        else
        {
            completionData->insert(std::pair<IJKCellIndex, std::vector<RigCompletionData> >(completion.cellIndex(), std::vector<RigCompletionData> {completion}));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
CellDirection RicWellPathExportCompletionDataFeature::calculateDirectionInCell(RimEclipseCase* eclipseCase, size_t cellIndex, const cvf::Vec3d& lengthsInCell)
{
    RigEclipseCaseData* eclipseCaseData = eclipseCase->eclipseCaseData();

    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "DX");
    cvf::ref<RigResultAccessor> dxAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "DX");
    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "DY");
    cvf::ref<RigResultAccessor> dyAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "DY");
    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "DZ");
    cvf::ref<RigResultAccessor> dzAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "DZ");

    double xLengthFraction = abs(lengthsInCell.x() / dxAccessObject->cellScalarGlobIdx(cellIndex));
    double yLengthFraction = abs(lengthsInCell.y() / dyAccessObject->cellScalarGlobIdx(cellIndex));
    double zLengthFraction = abs(lengthsInCell.z() / dzAccessObject->cellScalarGlobIdx(cellIndex));

    if (xLengthFraction > yLengthFraction && xLengthFraction > zLengthFraction)
    {
        return CellDirection::DIR_I;
    }
    else if (yLengthFraction > xLengthFraction && yLengthFraction > zLengthFraction)
    {
        return CellDirection::DIR_J;
    }
    else
    {
        return CellDirection::DIR_K;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
double RicWellPathExportCompletionDataFeature::calculateTransmissibility(RimEclipseCase* eclipseCase, 
                                                                         const RimWellPath* wellPath, 
                                                                         const cvf::Vec3d& internalCellLengths, 
                                                                         double skinFactor, 
                                                                         double wellRadius, 
                                                                         size_t cellIndex,
                                                                         size_t volumeScaleConstant,
                                                                         CellDirection directionForVolumeScaling)
{
    RigEclipseCaseData* eclipseCaseData = eclipseCase->eclipseCaseData();

    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "DX");
    cvf::ref<RigResultAccessor> dxAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "DX");
    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "DY");
    cvf::ref<RigResultAccessor> dyAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "DY");
    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "DZ");
    cvf::ref<RigResultAccessor> dzAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "DZ");

    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "PERMX");
    cvf::ref<RigResultAccessor> permxAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "PERMX");
    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "PERMY");
    cvf::ref<RigResultAccessor> permyAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "PERMY");
    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "PERMZ");
    cvf::ref<RigResultAccessor> permzAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "PERMZ");

    double dx = dxAccessObject->cellScalarGlobIdx(cellIndex);
    double dy = dyAccessObject->cellScalarGlobIdx(cellIndex);
    double dz = dzAccessObject->cellScalarGlobIdx(cellIndex);
    double permx = permxAccessObject->cellScalarGlobIdx(cellIndex);
    double permy = permxAccessObject->cellScalarGlobIdx(cellIndex);
    double permz = permxAccessObject->cellScalarGlobIdx(cellIndex);

    double darcy = RiaEclipseUnitTools::darcysConstant(wellPath->unitSystem());

    if (volumeScaleConstant != 1)
    {
        if (directionForVolumeScaling == CellDirection::DIR_I) dx = dx / volumeScaleConstant;
        if (directionForVolumeScaling == CellDirection::DIR_J) dy = dy / volumeScaleConstant;
        if (directionForVolumeScaling == CellDirection::DIR_K) dz = dz / volumeScaleConstant;
    }

    double transx = RigTransmissibilityEquations::wellBoreTransmissibilityComponent(internalCellLengths.x(), permy, permz, dy, dz, wellRadius, skinFactor, darcy);
    double transy = RigTransmissibilityEquations::wellBoreTransmissibilityComponent(internalCellLengths.y(), permx, permz, dx, dz, wellRadius, skinFactor, darcy);
    double transz = RigTransmissibilityEquations::wellBoreTransmissibilityComponent(internalCellLengths.z(), permy, permx, dy, dx, wellRadius, skinFactor, darcy);

    return RigTransmissibilityEquations::totalConnectionFactor(transx, transy, transz);
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
double RicWellPathExportCompletionDataFeature::calculateTransmissibilityAsEclipseDoes(RimEclipseCase* eclipseCase,
                                                                                      double skinFactor,
                                                                                      double wellRadius,
                                                                                      size_t cellIndex,
                                                                                      CellDirection direction)
{
    RigEclipseCaseData* eclipseCaseData = eclipseCase->eclipseCaseData();

    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "DX");
    cvf::ref<RigResultAccessor> dxAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "DX");
    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "DY");
    cvf::ref<RigResultAccessor> dyAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "DY");
    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "DZ");
    cvf::ref<RigResultAccessor> dzAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "DZ");

    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "PERMX");
    cvf::ref<RigResultAccessor> permxAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "PERMX");
    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "PERMY");
    cvf::ref<RigResultAccessor> permyAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "PERMY");
    eclipseCase->results(RiaDefines::MATRIX_MODEL)->findOrLoadScalarResult(RiaDefines::STATIC_NATIVE, "PERMZ");
    cvf::ref<RigResultAccessor> permzAccessObject = RigResultAccessorFactory::createFromUiResultName(eclipseCaseData, 0, RiaDefines::MATRIX_MODEL, 0, "PERMZ");

    double dx = dxAccessObject->cellScalarGlobIdx(cellIndex);
    double dy = dyAccessObject->cellScalarGlobIdx(cellIndex);
    double dz = dzAccessObject->cellScalarGlobIdx(cellIndex);
    double permx = permxAccessObject->cellScalarGlobIdx(cellIndex);
    double permy = permxAccessObject->cellScalarGlobIdx(cellIndex);
    double permz = permxAccessObject->cellScalarGlobIdx(cellIndex);

    RiaEclipseUnitTools::UnitSystem units = eclipseCaseData->unitsType();
    double darcy = RiaEclipseUnitTools::darcysConstant(units);

    double trans = cvf::UNDEFINED_DOUBLE;
    if (direction == CellDirection::DIR_I)
    {
        trans = RigTransmissibilityEquations::wellBoreTransmissibilityComponent(dx, permy, permz, dy, dz, wellRadius, skinFactor, darcy);
    }
    else if (direction == CellDirection::DIR_J)
    {
        trans = RigTransmissibilityEquations::wellBoreTransmissibilityComponent(dy, permx, permz, dx, dz, wellRadius, skinFactor, darcy);
    }
    else if (direction == CellDirection::DIR_K)
    {
        trans = RigTransmissibilityEquations::wellBoreTransmissibilityComponent(dz, permy, permx, dy, dx, wellRadius, skinFactor, darcy);
    }

    return trans;
}
