/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017 -     Statoil ASA
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
#include "cafPdmChildArrayField.h"
#include "cafPdmField.h"
#include "cafPdmFieldCvfVec3d.h"
#include "cafPdmFieldHandle.h"
#include "cafPdmObject.h"
#include "cafPdmPtrField.h"

#include "cvfBase.h"
#include "cvfObject.h"
#include "cvfVector3.h"

#include <vector>
#include "RigFractureCell.h"
#include "RigFractureGrid.h"

class RigStimPlanFractureDefinition;
class RimStimPlanLegendConfig;
class RigFractureCell;
class RigFractureGrid;

//==================================================================================================
///  
///  
//==================================================================================================
class RimStimPlanFractureTemplate : public RimFractureTemplate 
{
     CAF_PDM_HEADER_INIT;

public:
    RimStimPlanFractureTemplate(void);
    virtual ~RimStimPlanFractureTemplate(void);
    
    int                                     activeTimeStepIndex() { return m_activeTimeStepIndex; }
    bool                                    showStimPlanMesh()    { return m_showStimPlanMesh;}

    void                                    loadDataAndUpdate(); 

    void                                    setFileName(const QString& fileName);
    const QString&                          fileName();
    QString                                 fileNameWithOutPath();

    void                                    fractureTriangleGeometry(std::vector<cvf::Vec3f>* nodeCoords, 
                                                                     std::vector<cvf::uint>* triangleIndices, 
                                                                     RimUnitSystem::UnitSystem  neededUnit) override;
    std::vector<cvf::Vec3f>                 fractureBorderPolygon(RimUnitSystem::UnitSystem neededUnit);
    void sortPolygon(std::vector<cvf::Vec3f> &polygon);

    std::vector<double>                     getStimPlanTimeValues();
    std::vector<std::pair<QString, QString> > getStimPlanPropertyNamesUnits() const;
    void                                    computeMinMax(const QString& resultName, const QString& unitName, double* minValue, double* maxValue) const;
    void                                    setDefaultsBasedOnXMLfile();
    std::vector<std::vector<double>>        getDataAtTimeIndex(const QString& resultName, const QString& unitName, size_t timeStepIndex) const;
    
    void                                    setupStimPlanCells();
    const RigFractureGrid*                  fractureGrid() const;

    //Functions used by upscaling only
    void                                    getStimPlanDataAsPolygonsAndValues(std::vector<std::vector<cvf::Vec3d> > &cellsAsPolygons, std::vector<double> &parameterValue, const QString& resultName, const QString& unitName, size_t timeStepIndex);
    std::vector<cvf::Vec3d>                 getStimPlanRowPolygon(size_t i);
    std::vector<cvf::Vec3d>                 getStimPlanColPolygon(size_t j);

protected:
    virtual void                            fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue) override;
    virtual QList<caf::PdmOptionItemInfo>   calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool* useOptionsOnly) override;
    virtual void                            defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering);
    virtual void                            defineEditorAttribute(const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute * attribute) override;

private:
    void                                    updateUiTreeName();

    static cvf::ref<RigStimPlanFractureDefinition> readStimPlanXMLFile(const QString& stimPlanFileName, QString * errorMessage);
    static size_t                           readStimplanGridAndTimesteps(QXmlStreamReader &xmlStream, RigStimPlanFractureDefinition* stimPlanFileData);

    static double                           getAttributeValueDouble(QXmlStreamReader &xmlStream, QString parameterName);
    static QString                          getAttributeValueString(QXmlStreamReader &xmlStream, QString parameterName);
    static void                             getGriddingValues(QXmlStreamReader &xmlStream, std::vector<double>& gridValues, size_t& startNegValues);

    static std::vector<std::vector<double>> getAllDepthDataAtTimeStep(QXmlStreamReader &xmlStream, size_t startingNegValuesXs);
    std::vector<double>                     adjustedDepthCoordsAroundWellPathPosition() const;

    bool                                    setPropertyForPolygonDefault();
    void                                    setDepthOfWellPathAtFracture();
    QString                                 getUnitForStimPlanParameter(QString parameterName);

    caf::PdmField<int>                      m_activeTimeStepIndex;
    caf::PdmField<bool>                     m_showStimPlanMesh;

    caf::PdmField<double>                   m_wellPathDepthAtFracture;
    caf::PdmField<QString>                  m_parameterForPolygon;

    caf::PdmField<QString>                  m_stimPlanFileName;
    cvf::ref<RigStimPlanFractureDefinition> m_stimPlanFractureDefinitionData;
    cvf::ref<RigFractureGrid>               m_fractureGrid;

};