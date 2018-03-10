/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017-     Statoil ASA
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

#include "RicCreateSummaryCaseCollectionFeature.h"

#include "RiaApplication.h"

#include "RimSummaryCase.h"
#include "RimSummaryCaseCollection.h"
#include "RimSummaryCaseMainCollection.h"

#include "RiuPlotMainWindowTools.h"

#include "cafPdmObject.h"
#include "cafSelectionManager.h"

#include <QAction>

CAF_CMD_SOURCE_INIT(RicCreateSummaryCaseCollectionFeature, "RicCreateSummaryCaseCollectionFeature");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicCreateSummaryCaseCollectionFeature::isCommandEnabled()
{
    std::vector<RimSummaryCase*> selection;
    caf::SelectionManager::instance()->objectsByType(&selection);

    if (selection.size() == 0)
    {
        return false;
    }

    for (RimSummaryCase* summaryCase : selection)
    {
        if (summaryCase->isObservedData())
        {
            return false;
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicCreateSummaryCaseCollectionFeature::onActionTriggered(bool isChecked)
{
    std::vector<RimSummaryCase*> selection;
    caf::SelectionManager::instance()->objectsByType(&selection);
    if (selection.size() == 0) return;

    RimSummaryCaseMainCollection* summaryCaseMainCollection = nullptr;
    selection[0]->firstAncestorOrThisOfTypeAsserted(summaryCaseMainCollection);

    summaryCaseMainCollection->addCaseCollection(selection);
    summaryCaseMainCollection->updateConnectedEditors();
    
    RiuPlotMainWindowTools::showPlotMainWindow();
    RiuPlotMainWindowTools::selectAsCurrentItem(summaryCaseMainCollection->summaryCaseCollections().back()->allSummaryCases().front());
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicCreateSummaryCaseCollectionFeature::setupActionLook(QAction* actionToSetup)
{
    actionToSetup->setText("Group Summary Cases");
    actionToSetup->setIcon(QIcon(":/Folder.png"));
}
