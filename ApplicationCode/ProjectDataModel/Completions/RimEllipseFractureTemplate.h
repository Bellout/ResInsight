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

#pragma once

#include "RimFractureTemplate.h"

#include "cafAppEnum.h"
#include "cafPdmField.h"
#include "cafPdmFieldHandle.h"
#include "cafPdmObject.h"
#include "cafPdmFieldCvfVec3d.h"

#include "cvfBase.h"
#include "cvfObject.h"
#include "cvfVector3.h"

#include <vector>

class RigFractureGrid;

//==================================================================================================
///  
///  
//==================================================================================================
class RimEllipseFractureTemplate : public RimFractureTemplate 
{
     CAF_PDM_HEADER_INIT;

public:
    RimEllipseFractureTemplate();
    virtual ~RimEllipseFractureTemplate();

    void                    fractureTriangleGeometry(std::vector<cvf::Vec3f>* nodeCoords,
                                                     std::vector<cvf::uint>* polygonIndices) override;

    std::vector<cvf::Vec3f> fractureBorderPolygon() override;
    void                    changeUnits();
    const RigFractureGrid*  fractureGrid() const override;
    void                    setDefaultValuesFromUnit();
    double                  conductivity() const;
    
    void                    appendDataToResultStatistics(const QString&     uiResultName,
                                                         const QString&     unit,
                                                         MinMaxAccumulator& minMaxAccumulator,
                                                         PosNegAccumulator& posNegAccumulator) const override;

    void                    convertToUnitSystem(RiaEclipseUnitTools::UnitSystem neededUnit) override;

    void                    loadDataAndUpdate() override;
    std::vector<std::pair<QString, QString>> uiResultNamesWithUnit() const override;
    virtual void            reload() override;

protected:
    void                    defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering) override;
    void                    fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue) override;

private:
    void                   assignConductivityToCellsInsideEllipse();


    FractureWidthAndConductivity widthAndConductivityAtWellPathIntersection() const override;

private:
    cvf::ref<RigFractureGrid>   m_fractureGrid;

    caf::PdmField<double>        m_halfLength;
    caf::PdmField<double>        m_height;
    caf::PdmField<double>        m_width;
    caf::PdmField<double>        m_userDefinedEffectivePermeability;
};
