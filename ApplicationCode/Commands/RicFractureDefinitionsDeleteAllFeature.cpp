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

#include "RicFractureDefinitionsDeleteAllFeature.h"

#include "RimFractureDefinitionCollection.h"

#include "cafSelectionManager.h"

#include <QAction>

namespace caf
{

CAF_CMD_SOURCE_INIT(RicFractureDefinitionsDeleteAllFeature, "RicFractureDefinitionsDeleteAllFeature");


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicFractureDefinitionsDeleteAllFeature::isCommandEnabled()
{
    std::vector<RimFractureDefinitionCollection*> objects;
    caf::SelectionManager::instance()->objectsByType(&objects);

    if (objects.size() == 1)
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicFractureDefinitionsDeleteAllFeature::onActionTriggered(bool isChecked)
{
    std::vector<RimFractureDefinitionCollection*> objects;
    caf::SelectionManager::instance()->objectsByType(&objects);

    RimFractureDefinitionCollection* fractureDefinitionCollection = nullptr;
    if (objects.size() > 0)
    {
        fractureDefinitionCollection = objects[0];
    }

    fractureDefinitionCollection->deleteFractureDefinitions();
    
    fractureDefinitionCollection->uiCapability()->updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicFractureDefinitionsDeleteAllFeature::setupActionLook(QAction* actionToSetup)
{
    actionToSetup->setText("Delete All Fracture Definitions");
    actionToSetup->setIcon(QIcon(":/Erase.png"));
}

} // end namespace caf