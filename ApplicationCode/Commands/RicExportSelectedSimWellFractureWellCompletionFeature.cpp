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

#include "RicExportSelectedSimWellFractureWellCompletionFeature.h"


#include "RiaApplication.h"

#include "RifEclipseExportTools.h"
#include "RifEclipseExportTools.h"

#include "RimEclipseCase.h"
#include "RimEclipseView.h"
#include "RimEclipseWell.h"
#include "RimEclipseWellCollection.h"
#include "RimFracture.h"
#include "RimFractureExportSettings.h"

#include "RiuMainWindow.h"

#include "cafPdmObjectHandle.h"
#include "cafPdmUiPropertyViewDialog.h"
#include "cafSelectionManager.h"

#include "cvfAssert.h"

#include <QAction>
#include <QMessageBox>
#include <QString>

CAF_CMD_SOURCE_INIT(RicExportSelectedSimWellFractureWellCompletionFeature, "RicExportSelectedSimWellFractureWellCompletionFeature");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicExportSelectedSimWellFractureWellCompletionFeature::onActionTriggered(bool isChecked)
{

    std::vector<RimEclipseWell*> selection;
    caf::SelectionManager::instance()->objectsByType(&selection);



    std::vector<RimFracture*> fractures;
    for (RimEclipseWell* well : selection)
    {

        std::vector<RimFracture*> fracListForWell;
        well->descendantsIncludingThisOfType(fracListForWell);
        for (RimFracture* fracture : fracListForWell)
        {
            fractures.push_back(fracture);
        }
    }


    caf::PdmUiItem* pdmUiItem = caf::SelectionManager::instance()->selectedItem();
    if (!pdmUiItem) return;

    caf::PdmObjectHandle* objHandle = dynamic_cast<caf::PdmObjectHandle*>(pdmUiItem);
    if (!objHandle) return;

    RimEclipseView* eclipseWiew = nullptr;
    objHandle->firstAncestorOrThisOfType(eclipseWiew);

    RimFractureExportSettings exportSettings;

    RiaApplication* app = RiaApplication::instance();
    QString projectFolder = app->currentProjectPath();
    if (projectFolder.isEmpty())
    {
        projectFolder = eclipseWiew->eclipseCase()->locationOnDisc();
    }

    QString outputFileName = projectFolder + "/Fractures";
    exportSettings.fileName = outputFileName;

    caf::PdmUiPropertyViewDialog propertyDialog(RiuMainWindow::instance(), &exportSettings, "Export Fracture Well Completion Data", "");
    if (propertyDialog.exec() == QDialog::Accepted)
    {
        bool isOk = RifEclipseExportTools::writeFracturesToTextFile(exportSettings.fileName, fractures);

        if (!isOk)
        {
            QMessageBox::critical(NULL, "File export", "Failed to exported current result to " + exportSettings.fileName);
        }
    }


}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicExportSelectedSimWellFractureWellCompletionFeature::setupActionLook(QAction* actionToSetup)
{
    actionToSetup->setIcon(QIcon(":/FractureTemplate16x16.png"));
    actionToSetup->setText("Export Fracture Well Completion Data for Selected wells");
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicExportSelectedSimWellFractureWellCompletionFeature::isCommandEnabled()
{
    return true;
}