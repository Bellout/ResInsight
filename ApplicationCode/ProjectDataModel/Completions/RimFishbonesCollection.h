/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017     Statoil ASA
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

#include "RimCheckableNamedObject.h"

#include "RiaEclipseUnitTools.h"

#include "cafPdmChildArrayField.h"
#include "cafPdmChildField.h"
#include "cafPdmFieldCvfColor.h"    

#include "cvfColor3.h"

class RimFishbonesMultipleSubs;
class RimFishboneWellPathCollection;

//==================================================================================================
//
// 
//
//==================================================================================================
class RimFishbonesCollection : public RimCheckableNamedObject
{
    CAF_PDM_HEADER_INIT;

public:
    enum PressureDropType {
        HYDROSTATIC,
        HYDROSTATIC_FRICTION,
        HYDROSTATIC_FRICTION_ACCELERATION
    };

    typedef caf::AppEnum<PressureDropType> PressureDropEnum;

    enum LengthAndDepthType {
        ABS,
        INC
    };

    typedef caf::AppEnum<LengthAndDepthType> LengthAndDepthEnum;

    RimFishbonesCollection();

    RimFishboneWellPathCollection* wellPathCollection() const;
    void                           appendFishbonesSubs(RimFishbonesMultipleSubs* subs);

    caf::PdmChildArrayField<RimFishbonesMultipleSubs*>  fishbonesSubs;

    void         recalculateStartMD();
    double       startMD() const { return m_startMD; }
    double       mainBoreSkinFactor() const { return m_skinFactor; }
    double       mainBoreDiameter(RiaEclipseUnitTools::UnitSystem unitSystem) const;
    double       linerDiameter(RiaEclipseUnitTools::UnitSystem unitSystem) const;
    double       roughnessFactor(RiaEclipseUnitTools::UnitSystem unitSystem) const;

    PressureDropEnum    pressureDrop() const { return m_pressureDrop(); }
    LengthAndDepthEnum  lengthAndDepth() const { return m_lengthAndDepth(); }

    void         setUnitSystemSpecificDefaults();

protected:
    virtual void fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue) override;
    virtual void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering );

private:
    cvf::Color3f nextFishbonesColor() const;

private:
    caf::PdmChildField<RimFishboneWellPathCollection*>  m_wellPathCollection;

    caf::PdmField<double>                               m_startMD;
    caf::PdmField<double>                               m_skinFactor;
    caf::PdmField<double>                               m_mainBoreDiameter;
    caf::PdmField<double>                               m_linerDiameter;
    caf::PdmField<double>                               m_roughnessFactor;

    caf::PdmField<PressureDropEnum>                     m_pressureDrop;
    caf::PdmField<LengthAndDepthEnum>                   m_lengthAndDepth;

    bool                                                manuallyModifiedStartMD;
};
