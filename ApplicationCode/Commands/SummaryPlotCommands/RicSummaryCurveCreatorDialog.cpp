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

#include "RicSummaryCurveCreatorDialog.h"

#include "RiaApplication.h"

#include "RicSummaryCurveCreator.h"
#include "RicSummaryCurveCreatorSplitterUi.h"

#include "RiuMainPlotWindow.h"
#include "RiuTools.h"

#include <QVBoxLayout>

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RicSummaryCurveCreatorDialog::RicSummaryCurveCreatorDialog(QWidget* parent)
    : QDialog(parent, RiuTools::defaultDialogFlags())
{
    m_curveCreatorSplitterUi = new RicSummaryCurveCreatorSplitterUi(this);

    QWidget* propertyWidget = m_curveCreatorSplitterUi->getOrCreateWidget(this);

    QVBoxLayout* dummy = new QVBoxLayout(this);
    dummy->setContentsMargins(0, 0, 0, 0);
    dummy->addWidget(propertyWidget);

    setWindowTitle("Plot Editor");
    resize(1200, 800);
    connect(m_curveCreatorSplitterUi, SIGNAL(signalCloseButtonPressed()), this, SLOT(accept()));
    
    connect(this, SIGNAL(finished(int)), this, SLOT(slotDialogFinished()));
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RicSummaryCurveCreatorDialog::~RicSummaryCurveCreatorDialog()
{
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicSummaryCurveCreatorDialog::updateFromSummaryPlot(RimSummaryPlot* summaryPlot)
{
    m_curveCreatorSplitterUi->updateFromSummaryPlot(summaryPlot);
    m_curveCreatorSplitterUi->updateUi();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicSummaryCurveCreatorDialog::slotDialogFinished()
{
    RiuMainPlotWindow* plotwindow = RiaApplication::instance()->mainPlotWindow();
    if (plotwindow)
    {
        plotwindow->cleanUpTemporaryWidgets();
    }
}
