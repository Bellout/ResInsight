/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2016-     Statoil ASA
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

#include "RicNewSimWellFractureAtPosFeature.h"

#include "RiaApplication.h"

#include "RimCase.h"
#include "RimEclipseWell.h"
#include "RimEllipseFractureTemplate.h"
#include "RimFractureDefinitionCollection.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimSimWellFracture.h"
#include "RimSimWellFractureCollection.h"
#include "RimView.h"

#include "RiuMainWindow.h"
#include "RiuSelectionManager.h"
#include "RivSimWellPipeSourceInfo.h"
 
#include "cafSelectionManager.h"

#include "cvfAssert.h"

#include <QAction>

CAF_CMD_SOURCE_INIT(RicNewSimWellFractureAtPosFeature, "RicNewSimWellFractureAtPosFeature");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicNewSimWellFractureAtPosFeature::onActionTriggered(bool isChecked)
{
    RimView* activeView = RiaApplication::instance()->activeReservoirView();
    if (!activeView) return;

    RiuSelectionManager* riuSelManager = RiuSelectionManager::instance();
    RiuSelectionItem* selItem = riuSelManager->selectedItem(RiuSelectionManager::RUI_TEMPORARY);

    RiuSimWellSelectionItem* simWellItem = nullptr;
    if (selItem->type() == RiuSelectionItem::SIMWELL_SELECTION_OBJECT)
    {
        simWellItem = static_cast<RiuSimWellSelectionItem*>(selItem);
        if (!simWellItem) return;
    }
    
    RimEclipseWell* simWell = simWellItem->m_simWell;
    caf::PdmObjectHandle* objHandle = dynamic_cast<caf::PdmObjectHandle*>(simWell);
    if (!objHandle) return;

    RimEclipseWell* simWellObject = nullptr;
    objHandle->firstAncestorOrThisOfType(simWellObject);
    if (!simWellObject) return;

    RimSimWellFractureCollection* fractureCollection = simWellObject->simwellFractureCollection();
    if (!fractureCollection) return;

    RimSimWellFracture* fracture = new RimSimWellFracture();
    fractureCollection->simwellFractures.push_back(fracture);

    fracture->setClosestWellCoord(simWellItem->m_domainCoord, simWellItem->m_branchIndex);

    RimOilField* oilfield = nullptr;
    objHandle->firstAncestorOrThisOfType(oilfield);
    if (!oilfield) return;

    std::vector<RimFracture* > oldFractures;
    oilfield->descendantsIncludingThisOfType(oldFractures);
    QString fracNum = QString("%1").arg(oldFractures.size(), 2, 10, QChar('0'));

    fracture->name = QString("Fracture_") + fracNum;

    if (oilfield->fractureDefinitionCollection->fractureDefinitions.size() > 0)
    {
        RimEllipseFractureTemplate* fracDef = oilfield->fractureDefinitionCollection->fractureDefinitions[0];
        fracture->setFractureTemplate(fracDef);

        fracture->azimuth = fracDef->azimuthAngle();
        fracture->perforationLength = fracDef->perforationLength();
    }


    fractureCollection->updateConnectedEditors();
    RiuMainWindow::instance()->selectAsCurrentItem(fracture);

    activeView->scheduleCreateDisplayModelAndRedraw();


}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicNewSimWellFractureAtPosFeature::setupActionLook(QAction* actionToSetup)
{
    actionToSetup->setIcon(QIcon(":/FractureSymbol16x16.png"));
    actionToSetup->setText("New Fracture");
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicNewSimWellFractureAtPosFeature::isCommandEnabled()
{
    caf::PdmUiItem* pdmUiItem = caf::SelectionManager::instance()->selectedItem();
    if (!pdmUiItem) return false;

    caf::PdmObjectHandle* objHandle = dynamic_cast<caf::PdmObjectHandle*>(pdmUiItem);
    if (!objHandle) return false;

    RimEclipseWell* eclipseWell = nullptr;
    objHandle->firstAncestorOrThisOfType(eclipseWell);

    if (eclipseWell)
    {
        return true;
    }

    return false;
}