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

#pragma once


#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmPtrField.h"

class RimCalculationCollection;
class RimCalculation;

//==================================================================================================
/// 
//==================================================================================================
class RicSummaryCurveCalculator : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RicSummaryCurveCalculator();

    static QString  calculatedSummariesGroupName();
    static QString  calulationGroupName();

    RimCalculation* currentCalculation() const;

    void            parseExpression();
    void            calculate() const;

private:
    virtual void fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue) override;
    virtual void defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering) override;
    virtual QList<caf::PdmOptionItemInfo> calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool* useOptionsOnly) override;
    virtual void defineEditorAttribute(const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute) override;

private:
    static RimCalculationCollection* calculationCollection();

    // TODO : Move to a common caf helper class
    static void assignPushButtonEditor(caf::PdmFieldHandle* fieldHandle);
    static void assignPushButtonEditorText(caf::PdmUiEditorAttribute* attribute, const QString& text);

private:
    caf::PdmPtrField<RimCalculation*>   m_currentCalculation;
    
    caf::PdmField<bool>                 m_newCalculation;
    caf::PdmField<bool>                 m_deleteCalculation;
};