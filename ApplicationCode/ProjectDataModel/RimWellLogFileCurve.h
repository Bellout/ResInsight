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

#pragma once

#include "RimWellLogCurve.h"

#include "cafPdmPtrField.h"
#include "cafPdmField.h"

#include <vector>

class RimWellPath;
class RimWellLogFileChannel;
class RimWellLogFile;

//==================================================================================================
///  
///  
//==================================================================================================
class RimWellLogFileCurve : public RimWellLogCurve
{
    CAF_PDM_HEADER_INIT;

public:
    RimWellLogFileCurve();
    virtual ~RimWellLogFileCurve();

    void setWellPath(RimWellPath* wellPath);
    RimWellPath* wellPath() const;
    void setWellLogChannelName(const QString& name);
    void setWellLogFile(RimWellLogFile* wellLogFile);

    // Overrides from RimWellLogPlotCurve
    virtual QString wellName() const override;
    virtual QString wellLogChannelName() const override;
    
    RimWellLogFile* wellLogFile() const;

protected:
    // Overrides from RimWellLogPlotCurve
    virtual QString createCurveAutoName() override;
    virtual void onLoadDataAndUpdate(bool updateParentPlot) override;

    // Pdm overrrides
    virtual void fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue) override;
    virtual void defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering) override;
    virtual void defineUiTreeOrdering(caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName = "") override;
    virtual QList<caf::PdmOptionItemInfo> calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool * useOptionsOnly) override;
    virtual void initAfterRead() override;

    bool        isRftPlotChild() const;

protected:
    caf::PdmPtrField<RimWellPath*>      m_wellPath;
    caf::PdmPtrField<RimWellLogFile*>   m_wellLogFile;
    caf::PdmField<QString>              m_wellLogChannnelName;
    caf::PdmField<QString>              m_wellLogChannnelUnit;
};


