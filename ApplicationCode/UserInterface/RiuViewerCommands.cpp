/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2015-     Statoil ASA
//  Copyright (C) 2015-     Ceetron Solutions AS
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

#include "RiuViewerCommands.h"

#include "RiaApplication.h"
#include "RiaColorTables.h"

#include "RicEclipsePropertyFilterNewExec.h"
#include "RicGeoMechPropertyFilterNewExec.h"
#include "RicViewerEventInterface.h"
#include "WellPathCommands/RicIntersectionViewerEventHandler.h"
#include "WellPathCommands/RicWellPathViewerEventHandler.h"

#include "RigEclipseCaseData.h"
#include "RigFault.h"
#include "RigFemPartCollection.h"
#include "RigFemPartGrid.h"
#include "RigGeoMechCaseData.h"
#include "RigMainGrid.h"

#include "RiaDefines.h"
#include "RimCellEdgeColors.h"
#include "RimContextCommandBuilder.h"
#include "RimEclipseCase.h"
#include "RimEclipseCellColors.h"
#include "RimEclipseFaultColors.h"
#include "RimEclipseView.h"
#include "RimFaultInView.h"
#include "RimFaultInViewCollection.h"
#include "RimFracture.h"
#include "RimGeoMechCase.h"
#include "RimGeoMechCellColors.h"
#include "RimGeoMechView.h"
#include "RimIntersection.h"
#include "RimIntersectionBox.h"
#include "RimLegendConfig.h"
#include "RimSimWellInView.h"
#include "RimTernaryLegendConfig.h"
#include "RimViewController.h"
#include "RimWellPath.h"
#include "RimPerforationInterval.h"
#include "RimStimPlanFractureTemplate.h"
#include "RimEllipseFractureTemplate.h"
#include "Rim2dIntersectionView.h"

#include "RiuMainWindow.h"
#include "RiuSelectionManager.h"
#include "RiuViewer.h"

#include "RivFemPartGeometryGenerator.h"
#include "RivFemPickSourceInfo.h"
#include "RivIntersectionBoxSourceInfo.h"
#include "RivIntersectionSourceInfo.h"
#include "RivObjectSourceInfo.h"
#include "RivSimWellPipeSourceInfo.h"
#include "RivSourceInfo.h"
#include "RivTernarySaturationOverlayItem.h"
#include "RivWellPathSourceInfo.h"
#include "RivWellFracturePartMgr.h"

#include "cafCmdExecCommandManager.h"
#include "cafCmdFeatureManager.h"
#include "cafDisplayCoordTransform.h"
#include "cafSelectionManager.h"
#include "cafCmdFeatureMenuBuilder.h"

#include "cvfDrawableGeo.h"
#include "cvfHitItemCollection.h"
#include "cvfOverlayAxisCross.h"
#include "cvfOverlayScalarMapperLegend.h"
#include "cvfPart.h"
#include "cvfTransform.h"

#include <QMenu>
#include <QMouseEvent>
#include <QStatusBar>
#include <array>



//==================================================================================================
//
// RiaViewerCommands
//
//==================================================================================================

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RiuViewerCommands::RiuViewerCommands(RiuViewer* ownerViewer) 
    : QObject(ownerViewer), 
      m_viewer(ownerViewer), 
      m_currentGridIdx(-1),
      m_currentCellIndex(-1),
      m_currentPickPositionInDomainCoords(cvf::Vec3d::UNDEFINED)
{
    {
        m_viewerEventHandlers.push_back(dynamic_cast<RicViewerEventInterface*>(RicIntersectionViewerEventHandler::instance()));
    }

    {
        m_viewerEventHandlers.push_back(dynamic_cast<RicViewerEventInterface*>(RicWellPathViewerEventHandler::instance()));
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RiuViewerCommands::~RiuViewerCommands()
{

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuViewerCommands::setOwnerView(Rim3dView * owner)
{
    m_reservoirView = owner;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuViewerCommands::displayContextMenu(QMouseEvent* event)
{
    m_currentGridIdx = cvf::UNDEFINED_SIZE_T;
    m_currentCellIndex = cvf::UNDEFINED_SIZE_T;

    int winPosX = event->x();
    int winPosY = event->y();

    QMenu menu;
    caf::CmdFeatureMenuBuilder menuBuilder;

    uint firstPartTriangleIndex = cvf::UNDEFINED_UINT;
    cvf::Vec3d localIntersectionPoint(cvf::Vec3d::ZERO);
    cvf::Vec3d globalIntersectionPoint(cvf::Vec3d::ZERO);

    const cvf::Part* firstHitPart = nullptr;

    m_currentPickPositionInDomainCoords = cvf::Vec3d::UNDEFINED;

    cvf::HitItemCollection hitItems;
    if (m_viewer->rayPick(winPosX, winPosY, &hitItems))
    {
        std::vector<std::pair<const cvf::Part*, cvf::uint>> partAndTriangleIndexPairs;
        extractIntersectionData(hitItems, &localIntersectionPoint, &globalIntersectionPoint,
                                &partAndTriangleIndexPairs, nullptr, nullptr);

        for (const auto& partTringleIndex : partAndTriangleIndexPairs)
        {
            if (!firstHitPart)
            {
                auto part = partTringleIndex.first;
                const RivObjectSourceInfo* objectSourceInfo = dynamic_cast<const RivObjectSourceInfo*>(part->sourceInfo());
                if (objectSourceInfo && dynamic_cast<RimPerforationInterval*>(objectSourceInfo->object()))
                { 
                    // Skip picking on perforation interval, display well path context menu
                    continue;
                }

                firstHitPart = part;
                firstPartTriangleIndex = partTringleIndex.second;
                break;
            }
        }

        cvf::Vec3d displayModelOffset = cvf::Vec3d::ZERO;

        Rim3dView* activeView = RiaApplication::instance()->activeReservoirView();
        CVF_ASSERT(activeView);

        cvf::ref<caf::DisplayCoordTransform> transForm = activeView->displayCoordTransform();
        m_currentPickPositionInDomainCoords = transForm->transformToDomainCoord(globalIntersectionPoint);
    }

    if (firstHitPart && firstPartTriangleIndex != cvf::UNDEFINED_UINT)
    {
        const RivSourceInfo* rivSourceInfo = dynamic_cast<const RivSourceInfo*>(firstHitPart->sourceInfo());
        const RivFemPickSourceInfo* femSourceInfo = dynamic_cast<const RivFemPickSourceInfo*>(firstHitPart->sourceInfo());
        const RivIntersectionSourceInfo* crossSectionSourceInfo = dynamic_cast<const RivIntersectionSourceInfo*>(firstHitPart->sourceInfo());
        const RivIntersectionBoxSourceInfo* intersectionBoxSourceInfo = dynamic_cast<const RivIntersectionBoxSourceInfo*>(firstHitPart->sourceInfo());

        if (rivSourceInfo || femSourceInfo || crossSectionSourceInfo || intersectionBoxSourceInfo)
        {
            if (rivSourceInfo)
            {
                if (!rivSourceInfo->hasCellFaceMapping()) return;

                // Set the data regarding what was hit

                m_currentGridIdx = rivSourceInfo->gridIndex();
                m_currentCellIndex = rivSourceInfo->m_cellFaceFromTriangleMapper->cellIndex(firstPartTriangleIndex);
                m_currentFaceIndex = rivSourceInfo->m_cellFaceFromTriangleMapper->cellFace(firstPartTriangleIndex);
            }
            else if (femSourceInfo)
            {
                m_currentGridIdx = femSourceInfo->femPartIndex();
                m_currentCellIndex = femSourceInfo->triangleToElmMapper()->elementIndex(firstPartTriangleIndex);
            }
            else if (crossSectionSourceInfo)
            {
                findCellAndGridIndex(crossSectionSourceInfo, firstPartTriangleIndex, &m_currentCellIndex, &m_currentGridIdx);
                m_currentFaceIndex = cvf::StructGridInterface::NO_FACE;

                RiuSelectionItem* selItem = new RiuGeneralSelectionItem(crossSectionSourceInfo->crossSection());
                RiuSelectionManager::instance()->setSelectedItem(selItem, RiuSelectionManager::RUI_TEMPORARY);
                
                menuBuilder << "RicHideIntersectionFeature";
                menuBuilder.addSeparator();
                menuBuilder << "RicNewIntersectionViewFeature";
                menuBuilder.addSeparator();
            }
            else if (intersectionBoxSourceInfo)
            {
                findCellAndGridIndex(intersectionBoxSourceInfo, firstPartTriangleIndex, &m_currentCellIndex, &m_currentGridIdx);
                m_currentFaceIndex = cvf::StructGridInterface::NO_FACE;

                RiuSelectionItem* selItem = new RiuGeneralSelectionItem(intersectionBoxSourceInfo->intersectionBox());
                RiuSelectionManager::instance()->setSelectedItem(selItem, RiuSelectionManager::RUI_TEMPORARY);

                menuBuilder << "RicHideIntersectionBoxFeature";
                menuBuilder.addSeparator();
            }

            // IJK -slice commands
            RimViewController* viewController = nullptr;
            if (m_reservoirView) viewController = m_reservoirView->viewController();    

            if (!viewController || !viewController->isRangeFiltersControlled())
            {
                size_t i, j, k;
                ijkFromCellIndex(m_currentGridIdx, m_currentCellIndex, &i, &j, &k);

                QVariantList iSliceList;
                iSliceList.push_back(0);
                iSliceList.push_back(CVF_MAX(static_cast<int>(i + 1), 1));
                    
                QVariantList jSliceList;
                jSliceList.push_back(1); 
                jSliceList.push_back(CVF_MAX(static_cast<int>(j + 1), 1));

                QVariantList kSliceList;
                kSliceList.push_back(2); 
                kSliceList.push_back(CVF_MAX(static_cast<int>(k + 1), 1));

                menuBuilder.subMenuStart("Range Filter Slice", QIcon(":/CellFilter_Range.png"));

                menuBuilder.addCmdFeatureWithUserData("RicNewSliceRangeFilterFeature", "I-slice Range Filter", iSliceList);
                menuBuilder.addCmdFeatureWithUserData("RicNewSliceRangeFilterFeature", "J-slice Range Filter", jSliceList);
                menuBuilder.addCmdFeatureWithUserData("RicNewSliceRangeFilterFeature", "K-slice Range Filter", kSliceList);

                menuBuilder.subMenuEnd();
            }

            menuBuilder << "RicEclipsePropertyFilterNewInViewFeature";
            menuBuilder << "RicGeoMechPropertyFilterNewInViewFeature";

            menuBuilder.addSeparator();

            menuBuilder.subMenuStart("Intersections", QIcon(":/IntersectionXPlane16x16.png"));

            menuBuilder << "RicNewPolylineIntersectionFeature";
            menuBuilder << "RicNewAzimuthDipIntersectionFeature";
            menuBuilder << "RicIntersectionBoxAtPosFeature";
            
            menuBuilder << "RicIntersectionBoxXSliceFeature";
            menuBuilder << "RicIntersectionBoxYSliceFeature";
            menuBuilder << "RicIntersectionBoxZSliceFeature";

            menuBuilder.subMenuEnd();

            menuBuilder.addSeparator();


            RimEclipseView* eclipseView = dynamic_cast<RimEclipseView*>(m_reservoirView.p());
            if (eclipseView)
            {
                // Hide faults command
                const RigFault* fault = eclipseView->mainGrid()->findFaultFromCellIndexAndCellFace(m_currentCellIndex, m_currentFaceIndex);
                if (fault)
                {
                    menuBuilder.addSeparator();

                    QString faultName = fault->name();

                    QVariantList hideFaultList;
                    qulonglong currentCellIndex = m_currentCellIndex;
                    hideFaultList.push_back(currentCellIndex);
                    hideFaultList.push_back(m_currentFaceIndex);

                    menuBuilder.addCmdFeatureWithUserData("RicEclipseHideFaultFeature", QString("Hide ") + faultName, hideFaultList);
                }
            }
        }
    }

    // Well log curve creation commands
    if (firstHitPart && firstHitPart->sourceInfo())
    {
        const RivWellPathSourceInfo* wellPathSourceInfo = dynamic_cast<const RivWellPathSourceInfo*>(firstHitPart->sourceInfo());
        if (wellPathSourceInfo)
        {
            RimWellPath* wellPath = wellPathSourceInfo->wellPath();
            if (wellPath)
            {
                if (firstPartTriangleIndex != cvf::UNDEFINED_UINT)
                {
                    double measuredDepth = wellPathSourceInfo->measuredDepth(firstPartTriangleIndex, m_currentPickPositionInDomainCoords);
                    cvf::Vec3d trueVerticalDepth = wellPathSourceInfo->trueVerticalDepth(firstPartTriangleIndex, globalIntersectionPoint);
                    RiuSelectionItem* selItem = new RiuWellPathSelectionItem(wellPathSourceInfo, trueVerticalDepth, measuredDepth);
                    RiuSelectionManager::instance()->setSelectedItem(selItem, RiuSelectionManager::RUI_TEMPORARY);
                }

                //TODO: Update so these also use RiuWellPathSelectionItem 
                caf::SelectionManager::instance()->setSelectedItem(wellPath);

                menuBuilder << "RicNewWellLogCurveExtractionFeature";
                menuBuilder << "RicNewWellLogFileCurveFeature";
                
                menuBuilder.addSeparator();

                menuBuilder.subMenuStart("Well Plots", QIcon(":/SummaryPlot16x16.png"));

                menuBuilder << "RicNewRftPlotFeature";
                menuBuilder << "RicNewPltPlotFeature";

                menuBuilder.addSeparator();

                menuBuilder << "RicShowWellAllocationPlotFeature";

                menuBuilder.subMenuEnd();

                menuBuilder.addSeparator();

                menuBuilder.subMenuStart("Completions", QIcon(":/FishBoneGroup16x16.png"));

#ifdef USE_PROTOTYPE_FEATURE_FRACTURES
                menuBuilder << "RicNewWellPathFractureAtPosFeature";
#endif // USE_PROTOTYPE_FEATURE_FRACTURES
                menuBuilder << "RicNewFishbonesSubsAtMeasuredDepthFeature";
                menuBuilder << "RicNewPerforationIntervalAtMeasuredDepthFeature";

                menuBuilder.subMenuEnd();

                menuBuilder.addSeparator();

                menuBuilder << "RicNewWellPathIntersectionFeature";
            }
        }

        const RivSimWellPipeSourceInfo* eclipseWellSourceInfo = dynamic_cast<const RivSimWellPipeSourceInfo*>(firstHitPart->sourceInfo());
        if (eclipseWellSourceInfo)
        {
            RimSimWellInView* well = eclipseWellSourceInfo->well();
            if (well)
            {
                caf::SelectionManager::instance()->setSelectedItem(well);

                RiuSelectionItem* selItem = new RiuSimWellSelectionItem(eclipseWellSourceInfo->well(), m_currentPickPositionInDomainCoords, eclipseWellSourceInfo->branchIndex());
                RiuSelectionManager::instance()->setSelectedItem(selItem, RiuSelectionManager::RUI_TEMPORARY);

                menuBuilder << "RicNewWellLogCurveExtractionFeature";
                menuBuilder << "RicNewWellLogRftCurveFeature";

                menuBuilder.addSeparator();

                menuBuilder.subMenuStart("Well Plots", QIcon(":/SummaryPlot16x16.png"));

                menuBuilder << "RicNewRftPlotFeature";
                menuBuilder << "RicNewPltPlotFeature";

                menuBuilder.addSeparator();
                
                menuBuilder << "RicPlotProductionRateFeature";
                menuBuilder << "RicShowWellAllocationPlotFeature";

                menuBuilder.subMenuEnd();

                menuBuilder.addSeparator();
                menuBuilder << "RicShowContributingWellsFeature";
                menuBuilder.addSeparator();
#ifdef USE_PROTOTYPE_FEATURE_FRACTURES
                menuBuilder << "RicNewSimWellFractureAtPosFeature";
#endif // USE_PROTOTYPE_FEATURE_FRACTURES
                menuBuilder.addSeparator();
                menuBuilder << "RicNewSimWellIntersectionFeature";
            }
        }
    }

    // View Link commands
    if (!firstHitPart)
    {
        menuBuilder << "RicLinkViewFeature";
        menuBuilder << "RicShowLinkOptionsFeature";
        menuBuilder << "RicSetMasterViewFeature";
        menuBuilder << "RicUnLinkViewFeature";
    }

    menuBuilder.addSeparator();
    menuBuilder << "RicNewGridTimeHistoryCurveFeature";
    menuBuilder << "RicShowFlowCharacteristicsPlotFeature";
    menuBuilder << "RicSaveEclipseInputActiveVisibleCellsFeature";
    menuBuilder << "RicShowGridStatisticsFeature";
    menuBuilder << "RicSelectColorResult";

    menuBuilder.appendToMenu(&menu);

    if (!menu.isEmpty())
    {
        menu.exec(event->globalPos());
    }

    // Delete items in temporary selection
    RiuSelectionManager::instance()->deleteAllItems(RiuSelectionManager::RUI_TEMPORARY);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuViewerCommands::handlePickAction(int winPosX, int winPosY, Qt::KeyboardModifiers keyboardModifiers)
{
    if (handleOverlayItemPicking(winPosX, winPosY))
    {
        return;
    }

    size_t gridIndex = cvf::UNDEFINED_SIZE_T;
    size_t cellIndex = cvf::UNDEFINED_SIZE_T;
    size_t nncIndex = cvf::UNDEFINED_SIZE_T;
    cvf::StructGridInterface::FaceType face = cvf::StructGridInterface::NO_FACE;
    int gmFace = -1;
    bool intersectionHit = false;
    std::array<cvf::Vec3f, 3> intersectionTriangleHit;

    cvf::Vec3d localIntersectionPoint(cvf::Vec3d::ZERO);
    cvf::Vec3d globalIntersectionPoint(cvf::Vec3d::ZERO);

    // Extract all the above information from the pick
    {
        const cvf::Part* firstHitPart = nullptr;
        uint firstPartTriangleIndex = cvf::UNDEFINED_UINT;

        const cvf::Part* firstNncHitPart = nullptr;
        uint nncPartTriangleIndex = cvf::UNDEFINED_UINT;

        cvf::HitItemCollection hitItems;
        if (m_viewer->rayPick(winPosX, winPosY, &hitItems))
        {
            std::vector<std::pair<const cvf::Part*, cvf::uint>> partAndTriangleIndexPairs;
            extractIntersectionData(hitItems, &localIntersectionPoint, &globalIntersectionPoint, &partAndTriangleIndexPairs, &firstNncHitPart, &nncPartTriangleIndex);

            if (!partAndTriangleIndexPairs.empty())
            {
                RicViewerEventObject viewerEventObject(globalIntersectionPoint, partAndTriangleIndexPairs, m_reservoirView);
                for (size_t i = 0; i < m_viewerEventHandlers.size(); i++)
                {
                    if (m_viewerEventHandlers[i]->handleEvent(viewerEventObject))
                    {
                        return;
                    }
                }

                firstHitPart = partAndTriangleIndexPairs.front().first;
                firstPartTriangleIndex = partAndTriangleIndexPairs.front().second;
            }
        }

        if (firstHitPart && firstHitPart->sourceInfo())
        {
            const RivObjectSourceInfo* rivObjectSourceInfo = dynamic_cast<const RivObjectSourceInfo*>(firstHitPart->sourceInfo());
            const RivSourceInfo* rivSourceInfo = dynamic_cast<const RivSourceInfo*>(firstHitPart->sourceInfo());
            const RivFemPickSourceInfo* femSourceInfo = dynamic_cast<const RivFemPickSourceInfo*>(firstHitPart->sourceInfo());
            const RivIntersectionSourceInfo* crossSectionSourceInfo = dynamic_cast<const RivIntersectionSourceInfo*>(firstHitPart->sourceInfo());
            const RivIntersectionBoxSourceInfo* intersectionBoxSourceInfo = dynamic_cast<const RivIntersectionBoxSourceInfo*>(firstHitPart->sourceInfo());
            const RivSimWellPipeSourceInfo* eclipseWellSourceInfo = dynamic_cast<const RivSimWellPipeSourceInfo*>(firstHitPart->sourceInfo());

            if (rivObjectSourceInfo)
            {
#ifdef USE_PROTOTYPE_FEATURE_FRACTURES
                RiuMainWindow::instance()->selectAsCurrentItem(rivObjectSourceInfo->object());

                RimFracture* fracture = dynamic_cast<RimFracture*>(rivObjectSourceInfo->object());
                RimStimPlanFractureTemplate* stimPlanTempl = fracture ? dynamic_cast<RimStimPlanFractureTemplate*>(fracture->fractureTemplate()) : nullptr;
                RimEllipseFractureTemplate* ellipseTempl = fracture ? dynamic_cast<RimEllipseFractureTemplate*>(fracture->fractureTemplate()) : nullptr;
                if (stimPlanTempl || ellipseTempl)
                {
                    // Set fracture resultInfo text
                    QString resultInfoText;

                    cvf::ref<caf::DisplayCoordTransform> transForm = m_reservoirView->displayCoordTransform();
                    cvf::Vec3d domainCoord = transForm->transformToDomainCoord(globalIntersectionPoint);

                    RimEclipseView* eclView = dynamic_cast<RimEclipseView*>(m_reservoirView.p());
                    RivWellFracturePartMgr* partMgr = fracture->fracturePartManager();
                    if (eclView) resultInfoText = partMgr->resultInfoText(*eclView, domainCoord);

                    // Set intersection point result text
                    QString intersectionPointText;

                    intersectionPointText.sprintf("Intersection point : Global [E: %.2f, N: %.2f, Depth: %.2f]", domainCoord.x(), domainCoord.y(), -domainCoord.z());
                    resultInfoText.append(intersectionPointText);

                    // Display result info text
                    RiuMainWindow::instance()->setResultInfo(resultInfoText);
                }
#endif //USE_PROTOTYPE_FEATURE_FRACTURES
            }
            
            if (rivSourceInfo)
            {
                gridIndex = rivSourceInfo->gridIndex();
                if (rivSourceInfo->hasCellFaceMapping())
                {
                    CVF_ASSERT(rivSourceInfo->m_cellFaceFromTriangleMapper.notNull());

                    cellIndex = rivSourceInfo->m_cellFaceFromTriangleMapper->cellIndex(firstPartTriangleIndex);
                    face = rivSourceInfo->m_cellFaceFromTriangleMapper->cellFace(firstPartTriangleIndex);
                }
            }
            else if (femSourceInfo)
            {
                gridIndex = femSourceInfo->femPartIndex();
                cellIndex = femSourceInfo->triangleToElmMapper()->elementIndex(firstPartTriangleIndex);
                gmFace = femSourceInfo->triangleToElmMapper()->elementFace(firstPartTriangleIndex);

            }
            else if (crossSectionSourceInfo)
            {
                findCellAndGridIndex(crossSectionSourceInfo, firstPartTriangleIndex, &cellIndex, &gridIndex);
                intersectionHit = true;
                intersectionTriangleHit = crossSectionSourceInfo->triangle(firstPartTriangleIndex);

                RiuMainWindow::instance()->selectAsCurrentItem(crossSectionSourceInfo->crossSection());

            }
            else if (intersectionBoxSourceInfo)
            {
                findCellAndGridIndex(intersectionBoxSourceInfo, firstPartTriangleIndex, &cellIndex, &gridIndex);
                intersectionHit = true;
                intersectionTriangleHit = intersectionBoxSourceInfo->triangle(firstPartTriangleIndex);

                RiuMainWindow::instance()->selectAsCurrentItem(intersectionBoxSourceInfo->intersectionBox());

            }
            else if (eclipseWellSourceInfo)
            {
                RiuMainWindow::instance()->selectAsCurrentItem(eclipseWellSourceInfo->well());
            }
        }

        if (firstNncHitPart && firstNncHitPart->sourceInfo())
        {
            const RivSourceInfo* rivSourceInfo = dynamic_cast<const RivSourceInfo*>(firstNncHitPart->sourceInfo());
            if (rivSourceInfo)
            {
                if (nncPartTriangleIndex < rivSourceInfo->m_NNCIndices->size())
                {
                    nncIndex = rivSourceInfo->m_NNCIndices->get(nncPartTriangleIndex);
                }
            }
        }
    }

    if (cellIndex == cvf::UNDEFINED_SIZE_T)
    {
        RiuSelectionManager::instance()->deleteAllItems();
    }
    else 
    {
        bool appendToSelection = false;
        if (keyboardModifiers & Qt::ControlModifier)
        {
            appendToSelection = true;
        }

        std::vector<RiuSelectionItem*> items;
        RiuSelectionManager::instance()->selectedItems(items);

        const caf::ColorTable& colorTable = RiaColorTables::selectionPaletteColors();

        cvf::Color3f curveColor = colorTable.cycledColor3f(items.size());

        if (!appendToSelection)
        {
            curveColor = colorTable.cycledColor3f(0);
        }

        RiuSelectionItem* selItem = nullptr;
        {
            Rim2dIntersectionView* intersectionView = dynamic_cast<Rim2dIntersectionView*>(m_reservoirView.p());
            RimEclipseView* eclipseView = dynamic_cast<RimEclipseView*>(m_reservoirView.p());
            RimGeoMechView* geomView = dynamic_cast<RimGeoMechView*>(m_reservoirView.p());

            if (intersectionView)
            {
                intersectionView->intersection()->firstAncestorOrThisOfType(eclipseView);
                intersectionView->intersection()->firstAncestorOrThisOfType(geomView);
            }

            if (eclipseView)
            {
                selItem = new RiuEclipseSelectionItem(eclipseView, gridIndex, cellIndex, nncIndex, curveColor, face, localIntersectionPoint);
            }

            if (geomView)
            {
                if(intersectionHit)   selItem = new RiuGeoMechSelectionItem(geomView, gridIndex, cellIndex, curveColor, gmFace, localIntersectionPoint, intersectionTriangleHit);
                else                  selItem = new RiuGeoMechSelectionItem(geomView, gridIndex, cellIndex, curveColor, gmFace, localIntersectionPoint);
            }

            if (intersectionView) selItem = new Riu2dIntersectionSelectionItem(selItem);
        }

        if (appendToSelection)
        {
            RiuSelectionManager::instance()->appendItemToSelection(selItem);
        }
        else if(selItem)
        {
            RiuSelectionManager::instance()->setSelectedItem(selItem);
        }
    }

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::Vec3d RiuViewerCommands::lastPickPositionInDomainCoords() const
{
    return m_currentPickPositionInDomainCoords;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuViewerCommands::findCellAndGridIndex(const RivIntersectionSourceInfo* crossSectionSourceInfo, cvf::uint firstPartTriangleIndex, size_t* cellIndex, size_t* gridIndex)
{
    CVF_ASSERT(cellIndex && gridIndex);

    RimCase* ownerCase = m_reservoirView->ownerCase();
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(ownerCase);
    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(ownerCase);
    if (eclipseCase)
    {
        //RimEclipseView* eclipseView = dynamic_cast<RimEclipseView*>(m_reservoirView.p());
        RimEclipseView* eclipseView;
        crossSectionSourceInfo->crossSection()->firstAncestorOrThisOfType(eclipseView);

        size_t globalCellIndex = crossSectionSourceInfo->triangleToCellIndex()[firstPartTriangleIndex];

        const RigCell& cell = eclipseView->mainGrid()->globalCellArray()[globalCellIndex];

        *cellIndex = cell.gridLocalCellIndex();
        *gridIndex = cell.hostGrid()->gridIndex();
    }
    else if (geomCase)
    {
        *cellIndex = crossSectionSourceInfo->triangleToCellIndex()[firstPartTriangleIndex];
        *gridIndex = 0;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuViewerCommands::findCellAndGridIndex(const RivIntersectionBoxSourceInfo* intersectionBoxSourceInfo, cvf::uint firstPartTriangleIndex, size_t* cellIndex, size_t* gridIndex)
{
    CVF_ASSERT(cellIndex && gridIndex);

    RimEclipseView* eclipseView = dynamic_cast<RimEclipseView*>(m_reservoirView.p());
    RimGeoMechView* geomView = dynamic_cast<RimGeoMechView*>(m_reservoirView.p());
    if (eclipseView)
    {
        size_t globalCellIndex = intersectionBoxSourceInfo->triangleToCellIndex()[firstPartTriangleIndex];

        const RigCell& cell = eclipseView->mainGrid()->globalCellArray()[globalCellIndex];
        *cellIndex = cell.gridLocalCellIndex();
        *gridIndex = cell.hostGrid()->gridIndex();
    }
    else if (geomView)
    {
        *cellIndex = intersectionBoxSourceInfo->triangleToCellIndex()[firstPartTriangleIndex];
        *gridIndex = 0;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuViewerCommands::extractIntersectionData(const cvf::HitItemCollection& hitItems, 
                                                cvf::Vec3d* localIntersectionPoint,
                                                cvf::Vec3d*globalIntersectionPoint,
                                                std::vector<std::pair<const cvf::Part*, cvf::uint>>* partAndTriangleIndexPairs,
                                                const cvf::Part** nncPart, uint* nncPartFaceHit)
{
    CVF_ASSERT(hitItems.count() > 0);

    double pickDepthThresholdSquared = 0.05 *0.05;
    {
        RimEclipseView* eclipseView = dynamic_cast<RimEclipseView*>(m_reservoirView.p());

        if (eclipseView && eclipseView->mainGrid())
        {
            double characteristicCellSize = eclipseView->mainGrid()->characteristicIJCellSize();
            pickDepthThresholdSquared = characteristicCellSize / 100.0;
            pickDepthThresholdSquared = pickDepthThresholdSquared * pickDepthThresholdSquared;
        }
    }

    size_t firstNonNncPartIndex = cvf::UNDEFINED_SIZE_T;
    cvf::Vec3d firstItemIntersectionPoint = hitItems.item(0)->intersectionPoint();

    // Check if we have a close hit item with NNC data

    for (size_t i = 0; i < hitItems.count(); i++)
    {
        const cvf::HitItem* hitItemCandidate = hitItems.item(i);
        cvf::Vec3d diff = firstItemIntersectionPoint - hitItemCandidate->intersectionPoint();

        const cvf::Part* pickedPartCandidate = hitItemCandidate->part();
        bool isNncpart = false;

        if (pickedPartCandidate && pickedPartCandidate->sourceInfo())
        {
            const RivSourceInfo* rivSourceInfo = dynamic_cast<const RivSourceInfo*>(pickedPartCandidate->sourceInfo());
            if (rivSourceInfo && rivSourceInfo->hasNNCIndices())
            {

                // Hit items are ordered by distance from eye
                if (diff.lengthSquared() < pickDepthThresholdSquared)
                {
                    if (nncPart)
                    {
                        *nncPart = pickedPartCandidate;
                    }

                    const cvf::HitDetailDrawableGeo* detail = dynamic_cast<const cvf::HitDetailDrawableGeo*>(hitItemCandidate->detail());
                    if (detail && nncPartFaceHit)
                    {
                        *nncPartFaceHit = detail->faceIndex();
                    }

                    isNncpart = true;
                }
            }
        }

        if (!isNncpart && firstNonNncPartIndex == cvf::UNDEFINED_SIZE_T)
        {
            firstItemIntersectionPoint = hitItemCandidate->intersectionPoint();
            firstNonNncPartIndex = i;
        }

        if (firstNonNncPartIndex != cvf::UNDEFINED_SIZE_T && nncPart && *nncPart)
        {
            break;
        }
    }

    if (firstNonNncPartIndex != cvf::UNDEFINED_SIZE_T)
    {
        {
            const cvf::HitItem* firstNonNncHitItem = hitItems.item(firstNonNncPartIndex);
            const cvf::Part* pickedPart = firstNonNncHitItem->part();
            CVF_ASSERT(pickedPart);

            const cvf::Transform* xf = pickedPart->transform();
            const cvf::Vec3d& globalPickedPoint = firstNonNncHitItem->intersectionPoint();

            if (globalIntersectionPoint)
            {
                *globalIntersectionPoint = globalPickedPoint;
            }

            if (localIntersectionPoint)
            {
                if (xf)
                {
                    *localIntersectionPoint = globalPickedPoint.getTransformedPoint(xf->worldTransform().getInverted());
                }
                else
                {
                    *localIntersectionPoint = globalPickedPoint;
                }
            }
        }

        for (size_t i = firstNonNncPartIndex; i < hitItems.count(); i++)
        {
            const cvf::HitItem* hitItem = hitItems.item(i);
            const cvf::Part* pickedPart = hitItem->part();

            cvf::uint faceIndex = cvf::UNDEFINED_UINT;
            const cvf::HitDetailDrawableGeo* detail = dynamic_cast<const cvf::HitDetailDrawableGeo*>(hitItem->detail());
            if (detail)
            {
                faceIndex = detail->faceIndex();
            }

            partAndTriangleIndexPairs->push_back(std::make_pair(pickedPart, faceIndex));
        }
    }
    else
    {
        if (localIntersectionPoint && nncPart && *nncPart)
        {
            const cvf::Vec3d& globalPickedPoint = firstItemIntersectionPoint;

            if (globalIntersectionPoint)
            {
                *globalIntersectionPoint = globalPickedPoint;
            }

            const cvf::Transform* xf = (*nncPart)->transform();
            if (xf)
            {
                *localIntersectionPoint = globalPickedPoint.getTransformedPoint(xf->worldTransform().getInverted());
            }
            else
            {
                *localIntersectionPoint = globalPickedPoint;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuViewerCommands::ijkFromCellIndex(size_t gridIdx, size_t cellIndex,  size_t* i, size_t* j, size_t* k)
{
    RimEclipseView* eclipseView = dynamic_cast<RimEclipseView*>(m_reservoirView.p());
    RimGeoMechView* geomView = dynamic_cast<RimGeoMechView*>(m_reservoirView.p());

    if (eclipseView && eclipseView->eclipseCase())
    {
        eclipseView->eclipseCase()->eclipseCaseData()->grid(gridIdx)->ijkFromCellIndex(cellIndex, i, j, k);
    }
    
    if (geomView && geomView->geoMechCase())
    {
        geomView->geoMechCase()->geoMechData()->femParts()->part(gridIdx)->structGrid()->ijkFromCellIndex(cellIndex, i, j, k);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RiuViewerCommands::handleOverlayItemPicking(int winPosX, int winPosY)
{
    cvf::OverlayItem* pickedOverlayItem = m_viewer->overlayItem(winPosX, winPosY);
    if (!pickedOverlayItem)
    {
        pickedOverlayItem = m_viewer->pickFixedPositionedLegend(winPosX, winPosY);
    }

    if (pickedOverlayItem)
    {
        caf::PdmObject* objToSelect = nullptr;

        RimEclipseView* eclipseView = dynamic_cast<RimEclipseView*>(m_reservoirView.p());
        if (eclipseView)
        {
            if (eclipseView->cellResult()->legendConfig()->legend() == pickedOverlayItem)
            {
                objToSelect = eclipseView->cellResult()->legendConfig();
            }
            else if (eclipseView->cellResult()->ternaryLegendConfig()->legend() == pickedOverlayItem)
            {
                objToSelect = eclipseView->cellResult()->ternaryLegendConfig();
            }
            else if (eclipseView->faultResultSettings()->customFaultResult()->legendConfig()->legend() == pickedOverlayItem)
            {
                objToSelect = eclipseView->faultResultSettings()->customFaultResult()->legendConfig();
            }
            else if (eclipseView->faultResultSettings()->customFaultResult()->ternaryLegendConfig()->legend() == pickedOverlayItem)
            {
                objToSelect = eclipseView->faultResultSettings()->customFaultResult()->ternaryLegendConfig();
            }
            else if (eclipseView->cellEdgeResult()->legendConfig()->legend() == pickedOverlayItem)
            {
                objToSelect = eclipseView->cellEdgeResult()->legendConfig();
            }
        }

        RimGeoMechView* geomView = dynamic_cast<RimGeoMechView*>(m_reservoirView.p());
        if (geomView)
        {
            if (geomView->cellResult()->legendConfig()->legend() == pickedOverlayItem)
            {
                objToSelect = geomView->cellResult()->legendConfig();
            }
        }

        if (objToSelect)
        {
            RiuMainWindow::instance()->selectAsCurrentItem(objToSelect);

            return true;
        }
    }

    return false;
}
