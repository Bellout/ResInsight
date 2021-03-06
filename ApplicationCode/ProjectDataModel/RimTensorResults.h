/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2018 Statoil ASA
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

#include "cafPdmChildField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

#include "RigFemResultPosEnum.h"
#include "RimLegendConfig.h"

#include <QList>

#include <vector>

class RigFemResultAddress;
class RimLegendConfig;

//==================================================================================================
///  
///  
//==================================================================================================
class RimTensorResults : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    enum TensorColors
    {
        WHITE_GRAY_BLACK,
        ORANGE_BLUE_WHITE,
        MAGENTA_BROWN_GRAY,
        RESULT_COLORS
    };

    enum ScaleMethod
    {
        RESULT,
        CONSTANT
    };

public:
    RimTensorResults();
    virtual ~RimTensorResults();

    RigFemResultAddress selectedTensorResult() const;
    void                setShowTensors(bool enableTensors);
    bool                showTensors() const;
    bool                showPrincipal1() const;
    bool                showPrincipal2() const;
    bool                showPrincipal3() const;
    float               threshold() const;
    float               sizeScale() const;
    TensorColors        vectorColors() const;
    ScaleMethod         scaleMethod() const;

    void                mappingRange(double *min, double* max) const;

    static RigFemResultPosEnum resultPositionType();
    QString                    resultFieldName() const;
    static QString             uiFieldName(const QString& fieldName);

    caf::PdmChildField<RimLegendConfig*> arrowColorLegendConfig;

private:
    std::vector<std::string>                getResultMetaDataForUIFieldSetting();
    virtual void                            fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue) override;
    virtual caf::PdmFieldHandle*            objectToggleField() override;
    virtual QList<caf::PdmOptionItemInfo>   calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool* useOptionsOnly) override;
    virtual void                            defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering) override;
    virtual void                            initAfterRead() override;
    virtual void                            defineEditorAttribute(const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute) override;
    virtual void                            defineUiTreeOrdering(caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName = "") override;

    static QString                          fieldNameFromUi(const QString& uiFieldName);

private:
    caf::PdmField<bool>                              m_showTensors;

    caf::PdmField<QString>                           m_resultFieldName;

    caf::PdmField<QString>                           m_resultFieldNameUiField;

    caf::PdmField<bool>                              m_principal1;
    caf::PdmField<bool>                              m_principal2;
    caf::PdmField<bool>                              m_principal3;

    caf::PdmField<float>                             m_threshold;

    caf::PdmField<caf::AppEnum<TensorColors>>        m_vectorColor;

    caf::PdmField<caf::AppEnum<ScaleMethod>>         m_scaleMethod;
    caf::PdmField<float>                             m_sizeScale;
    caf::PdmField<RimLegendConfig::RangeModeEnum>    m_rangeMode;
};
