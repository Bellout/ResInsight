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

#include "RimStimPlanFractureTemplate.h"

#include "RiaApplication.h"
#include "RiaFractureDefines.h"
#include "RiaLogging.h"

#include "RifStimPlanXmlReader.h"

#include "RigStimPlanFractureDefinition.h"
#include "RigFractureGrid.h"

#include "RigFractureCell.h"
#include "RimEclipseView.h"
#include "RimFracture.h"
#include "RimFractureContainment.h"
#include "RimProject.h"
#include "RimStimPlanColors.h"
#include "RimStimPlanLegendConfig.h"
#include "RimTools.h"

#include "RivWellFracturePartMgr.h"

#include "cafPdmObject.h"
#include "cafPdmUiDoubleSliderEditor.h"
#include "cafPdmUiFilePathEditor.h"

#include "cvfVector3.h"

#include <QFileInfo>

#include <algorithm>
#include <vector>
#include <cmath>

static std::vector<double> EMPTY_DOUBLE_VECTOR;

CAF_PDM_SOURCE_INIT(RimStimPlanFractureTemplate, "RimStimPlanFractureTemplate");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimStimPlanFractureTemplate::RimStimPlanFractureTemplate()
{
    CAF_PDM_InitObject("Fracture Template", ":/FractureTemplate16x16.png", "", "");

    CAF_PDM_InitField(&m_stimPlanFileName,          "StimPlanFileName", QString(""), "File Name", "", "", "");
    m_stimPlanFileName.uiCapability()->setUiEditorTypeName(caf::PdmUiFilePathEditor::uiEditorTypeName());

    CAF_PDM_InitField(&m_wellPathDepthAtFracture,   "WellPathDepthAtFracture", 0.0, "Well/Fracture Intersection Depth", "", "", "");
    m_wellPathDepthAtFracture.uiCapability()->setUiEditorTypeName(caf::PdmUiDoubleSliderEditor::uiEditorTypeName());

    CAF_PDM_InitField(&m_borderPolygonResultName,   "BorderPolygonResultName", QString(""), "Parameter", "", "", "");
    m_borderPolygonResultName.uiCapability()->setUiHidden(true);

    CAF_PDM_InitField(&m_activeTimeStepIndex,           "ActiveTimeStepIndex", 0, "Active TimeStep Index", "", "", "");
    CAF_PDM_InitField(&m_conductivityResultNameOnFile,  "ConductivityResultName", QString(""), "Active Conductivity Result Name", "", "", "");

    CAF_PDM_InitField(&m_showStimPlanMesh_OBSOLETE, "ShowStimPlanMesh", true, "", "", "", "");
    m_showStimPlanMesh_OBSOLETE.uiCapability()->setUiHidden(true);

    m_fractureGrid = new RigFractureGrid();
    m_readError    = false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimStimPlanFractureTemplate::~RimStimPlanFractureTemplate()
{
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
int RimStimPlanFractureTemplate::activeTimeStepIndex()
{
    return m_activeTimeStepIndex;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{
    RimFractureTemplate::fieldChangedByUi(changedField, oldValue, newValue);

    if (&m_stimPlanFileName == changedField)
    {
        m_readError = false;
        loadDataAndUpdate();
        setDefaultsBasedOnXMLfile();
    }

    if (&m_activeTimeStepIndex == changedField)
    {
        //Changes to this parameters should change all fractures with this fracture template attached. 
        RimProject* proj;
        this->firstAncestorOrThisOfType(proj);
        if (proj)
        {
            std::vector<RimFracture*> fractures;
            proj->descendantsIncludingThisOfType(fractures);
            for (RimFracture* fracture : fractures)
            {
                if (fracture->fractureTemplate() == this)
                {
                    fracture->setStimPlanTimeIndexToPlot(m_activeTimeStepIndex);
                }
            }
            proj->createDisplayModelAndRedrawAllViews();
        }
    }

    if (&m_wellPathDepthAtFracture == changedField 
        || &m_borderPolygonResultName == changedField 
        || &m_activeTimeStepIndex == changedField 
        || &m_stimPlanFileName == changedField
        || &m_conductivityResultNameOnFile == changedField)
    {
        updateFractureGrid();

        RimProject* proj;
        this->firstAncestorOrThisOfType(proj);
        if (proj)
        {
            proj->createDisplayModelAndRedrawAllViews();
        }
    }

    if (changedField == &m_scaleApplyButton)
    {
        m_scaleApplyButton = false;
        reload();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::setFileName(const QString& fileName)
{
    m_stimPlanFileName = fileName;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const QString& RimStimPlanFractureTemplate::fileName()
{
    return m_stimPlanFileName();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::updateFilePathsFromProjectPath(const QString& newProjectPath, const QString& oldProjectPath)
{
    m_stimPlanFileName = RimTools::relocateFile(m_stimPlanFileName(), newProjectPath, oldProjectPath, nullptr, nullptr);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::setDefaultsBasedOnXMLfile()
{
    if (m_stimPlanFractureDefinitionData.isNull()) return;

    setDepthOfWellPathAtFracture();
    setPerforationLength();
    RiaLogging::info(QString("Setting well/fracture intersection depth at %1").arg(m_wellPathDepthAtFracture));
    m_activeTimeStepIndex = static_cast<int>(m_stimPlanFractureDefinitionData->totalNumberTimeSteps() - 1);
    bool polygonPropertySet = setBorderPolygonResultNameToDefault();

    if (polygonPropertySet) RiaLogging::info(QString("Calculating polygon outline based on %1 at timestep %2").arg(m_borderPolygonResultName).arg(m_stimPlanFractureDefinitionData->timeSteps()[m_activeTimeStepIndex]));
    else                    RiaLogging::info(QString("Property for polygon calculation not set."));

    if (!m_stimPlanFractureDefinitionData->conductivityResultNames().isEmpty())
    {
        m_conductivityResultNameOnFile = m_stimPlanFractureDefinitionData->conductivityResultNames().front();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimStimPlanFractureTemplate::setBorderPolygonResultNameToDefault()
{
    // first option: Width
    for (std::pair<QString, QString> property : uiResultNamesWithUnit())
    {
        if (property.first == "WIDTH")
        {
            m_borderPolygonResultName = property.first;
            return true;
        }
    }
    
    // if width not found, use conductivity
    if (hasConductivity())
    {
        m_borderPolygonResultName = m_stimPlanFractureDefinitionData->conductivityResultNames().first();
        return true;
    }

    // else: Set to first property
    if (!uiResultNamesWithUnit().empty())
    {
        m_borderPolygonResultName = uiResultNamesWithUnit()[0].first;
        return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::loadDataAndUpdate()
{
    QString errorMessage;

    if (m_readError) return;

    m_stimPlanFractureDefinitionData = RifStimPlanXmlReader::readStimPlanXMLFile( m_stimPlanFileName(),
                                                                                 m_conductivityScaleFactor(),
                                                                                 m_widthScaleFactor(),
                                                                                 m_heightScaleFactor(),
                                                                                 -m_wellPathDepthAtFracture(),
                                                                                 RifStimPlanXmlReader::MIRROR_AUTO,
                                                                                 fractureTemplateUnit(),
                                                                                 &errorMessage);
    if (errorMessage.size() > 0) RiaLogging::error(errorMessage);

    if (m_stimPlanFractureDefinitionData.notNull())
    {
        setDefaultConductivityResultIfEmpty();

        if (fractureTemplateUnit() == RiaEclipseUnitTools::UNITS_UNKNOWN)
        {
            setFractureTemplateUnit(m_stimPlanFractureDefinitionData->unitSet());
        }

        m_readError = false;
    }
    else
    {
        setFractureTemplateUnit(RiaEclipseUnitTools::UNITS_UNKNOWN); 
        m_readError = true;
    }

    updateFractureGrid();

    // Todo: Must update all views using this fracture template
    RimEclipseView* activeView = dynamic_cast<RimEclipseView*>(RiaApplication::instance()->activeReservoirView());
    if (activeView) activeView->fractureColors()->loadDataAndUpdate();

    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimStimPlanFractureTemplate::calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool* useOptionsOnly)
{
    QList<caf::PdmOptionItemInfo> options;

    options = RimFractureTemplate::calculateValueOptions(fieldNeedingOptions, useOptionsOnly);

    if (fieldNeedingOptions == &m_borderPolygonResultName)
    {
        for (std::pair<QString, QString> nameUnit : uiResultNamesWithUnit())
        {
            options.push_back(caf::PdmOptionItemInfo(nameUnit.first, nameUnit.first));
        }
    }
    else if (fieldNeedingOptions == &m_activeTimeStepIndex)
    {
        std::vector<double> timeValues = timeSteps();
        int                 index      = 0;
        for (double value : timeValues)
        {
            options.push_back(caf::PdmOptionItemInfo(QString::number(value), index));
            index++;
        }
    }
    else if (fieldNeedingOptions == &m_conductivityResultNameOnFile)
    {
        if (m_stimPlanFractureDefinitionData.notNull())
        {
            QStringList conductivityResultNames = m_stimPlanFractureDefinitionData->conductivityResultNames();
            for (const auto& resultName : conductivityResultNames)
            {
                options.push_back(caf::PdmOptionItemInfo(resultName, resultName));
            }
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::setDepthOfWellPathAtFracture()
{
    if (!m_stimPlanFractureDefinitionData.isNull())
    {
        double firstTvd = m_stimPlanFractureDefinitionData->topPerfTvd();
        double lastTvd = m_stimPlanFractureDefinitionData->bottomPerfTvd();

        if (firstTvd != HUGE_VAL && lastTvd != HUGE_VAL)
        {
            m_wellPathDepthAtFracture = (firstTvd + lastTvd) / 2;
        }
        else
        {
            firstTvd = m_stimPlanFractureDefinitionData->minDepth();
            lastTvd = m_stimPlanFractureDefinitionData->maxDepth();
            m_wellPathDepthAtFracture = (firstTvd + lastTvd) / 2;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::setPerforationLength()
{
    if (!m_stimPlanFractureDefinitionData.isNull())
    {
        double firstTvd = m_stimPlanFractureDefinitionData->topPerfTvd();
        double lastTvd = m_stimPlanFractureDefinitionData->bottomPerfTvd();

        if (firstTvd != HUGE_VAL && lastTvd != HUGE_VAL)
        {
            m_perforationLength = std::round(cvf::Math::abs(firstTvd - lastTvd));
        }
    }

    if (fractureTemplateUnit() == RiaEclipseUnitTools::UNITS_METRIC && m_perforationLength < 10)
    {
        m_perforationLength = 10;
    }
    else if (fractureTemplateUnit() == RiaEclipseUnitTools::UNITS_FIELD && m_perforationLength < RiaEclipseUnitTools::meterToFeet(10))
    {
        m_perforationLength = std::round(RiaEclipseUnitTools::meterToFeet(10));
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimStimPlanFractureTemplate::getUnitForStimPlanParameter(QString parameterName)
{
    QString unit;
    bool found = false;
    bool foundMultiple = false;

    for (std::pair<QString, QString> nameUnit : uiResultNamesWithUnit())
    {
        if (nameUnit.first == parameterName)
        {
            unit =  nameUnit.second;
            if (found) foundMultiple = true;
            found = true;
        }
    }

    if (foundMultiple)  RiaLogging::error(QString("Multiple units found for same parameter"));
    if (!found)         RiaLogging::error(QString("Requested unit / parameter not found for %1 template").arg(name()));
    return unit;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
FractureWidthAndConductivity RimStimPlanFractureTemplate::widthAndConductivityAtWellPathIntersection() const
{
    FractureWidthAndConductivity values;

    if (m_fractureGrid.notNull())
    {
        std::pair<size_t, size_t> wellCellIJ = m_fractureGrid->fractureCellAtWellCenter();
        size_t wellCellIndex = m_fractureGrid->getGlobalIndexFromIJ(wellCellIJ.first, wellCellIJ.second);
        const RigFractureCell& wellCell = m_fractureGrid->cellFromIndex(wellCellIndex);

        double conductivity = wellCell.getConductivtyValue();

        std::vector<std::pair<QString, QString> > propertyNamesUnitsOnFile = m_stimPlanFractureDefinitionData->getStimPlanPropertyNamesUnits();

        QString propertyNameForFractureWidth;
        {
            QString widthParameterName;
            QString effWidthParameterName;
            for (const auto& nameUnit : propertyNamesUnitsOnFile)
            {
                if (effWidthParameterName.isEmpty() && nameUnit.first.contains("effective width", Qt::CaseInsensitive))
                {
                    effWidthParameterName = nameUnit.first;
                }

                if (widthParameterName.isEmpty() && nameUnit.first.contains("width", Qt::CaseInsensitive))
                {
                    widthParameterName = nameUnit.first;
                }
            }

            if (!effWidthParameterName.isEmpty())
            {
                propertyNameForFractureWidth = effWidthParameterName;
            }
            else
            {
                propertyNameForFractureWidth = widthParameterName;
            }
        }

        if (!propertyNameForFractureWidth.isEmpty())
        {
            for (const auto& nameUnit : propertyNamesUnitsOnFile)
            {
                if (nameUnit.first == propertyNameForFractureWidth)
                {
                    double widthInRequiredUnit = HUGE_VAL;
                    {
                        auto resultValues = m_stimPlanFractureDefinitionData->fractureGridResults(nameUnit.first, nameUnit.second, m_activeTimeStepIndex);

                        double widthInFileUnitSystem = resultValues[wellCellIndex];

                        if (fractureTemplateUnit() == RiaEclipseUnitTools::UNITS_METRIC)
                        {
                            QString unitText = nameUnit.second;

                            widthInRequiredUnit = RiaEclipseUnitTools::convertToMeter(widthInFileUnitSystem, unitText);
                        }
                        else if (fractureTemplateUnit() == RiaEclipseUnitTools::UNITS_FIELD)
                        {
                            QString unitText = nameUnit.second;

                            widthInRequiredUnit = RiaEclipseUnitTools::convertToFeet(widthInFileUnitSystem, unitText);
                        }
                    }

                    if (widthInRequiredUnit != HUGE_VAL && fabs(widthInRequiredUnit) > 1e-20)
                    {
                        values.m_width = widthInRequiredUnit;
                        values.m_permeability = conductivity / widthInRequiredUnit;
                    }
                }
            }
        }
    }

    return values;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::setDefaultConductivityResultIfEmpty()
{
    if (m_conductivityResultNameOnFile().isEmpty())
    {
        if (!m_stimPlanFractureDefinitionData->conductivityResultNames().isEmpty())
        {
            m_conductivityResultNameOnFile = m_stimPlanFractureDefinitionData->conductivityResultNames().front();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimStimPlanFractureTemplate::mapUiResultNameToFileResultName(const QString& uiResultName) const
{
    QString fileResultName;

    if (uiResultName == RiaDefines::conductivityResultName())
    {
        fileResultName = m_conductivityResultNameOnFile();
    }
    else
    {
        fileResultName = uiResultName;
    }

    return fileResultName;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimStimPlanFractureTemplate::showStimPlanMesh() const
{
    return m_showStimPlanMesh_OBSOLETE();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::convertToUnitSystem(RiaEclipseUnitTools::UnitSystem neededUnit)
{
    if (m_fractureTemplateUnit() == neededUnit) return;

    setFractureTemplateUnit(neededUnit);
    RimFractureTemplate::convertToUnitSystem(neededUnit);

    m_readError = false;
    loadDataAndUpdate();

    if (m_stimPlanFractureDefinitionData.isNull()) return;

    if (neededUnit == RiaEclipseUnitTools::UNITS_FIELD)
    {
        m_wellPathDepthAtFracture = RiaEclipseUnitTools::meterToFeet(m_wellPathDepthAtFracture);
    }
    else if (neededUnit == RiaEclipseUnitTools::UNITS_METRIC)
    {
        m_wellPathDepthAtFracture = RiaEclipseUnitTools::feetToMeter(m_wellPathDepthAtFracture);
    }

    m_activeTimeStepIndex = static_cast<int>(m_stimPlanFractureDefinitionData->totalNumberTimeSteps() - 1);
    bool polygonPropertySet = setBorderPolygonResultNameToDefault();

    if (polygonPropertySet) RiaLogging::info(QString("Calculating polygon outline based on %1 at timestep %2").arg(m_borderPolygonResultName).arg(m_stimPlanFractureDefinitionData->timeSteps()[m_activeTimeStepIndex]));
    else                    RiaLogging::info(QString("Property for polygon calculation not set."));

    if (!m_stimPlanFractureDefinitionData->conductivityResultNames().isEmpty())
    {
        m_conductivityResultNameOnFile = m_stimPlanFractureDefinitionData->conductivityResultNames().front();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::reload()
{
    loadDataAndUpdate();

    RimProject* proj;
    this->firstAncestorOrThisOfType(proj);
    if (proj)
    {
        proj->createDisplayModelAndRedrawAllViews();
    }

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<double> RimStimPlanFractureTemplate::timeSteps()
{
    if (m_stimPlanFractureDefinitionData.notNull())
    {
        return m_stimPlanFractureDefinitionData->timeSteps();
    }

    return std::vector<double>();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<std::pair<QString, QString> > RimStimPlanFractureTemplate::uiResultNamesWithUnit() const
{
    std::vector<std::pair<QString, QString> > propertyNamesAndUnits;
    
    if (m_stimPlanFractureDefinitionData.notNull())
    {
        QString conductivityUnit = "mD/s";

        std::vector<std::pair<QString, QString> > tmp;

        std::vector<std::pair<QString, QString> > propertyNamesUnitsOnFile = m_stimPlanFractureDefinitionData->getStimPlanPropertyNamesUnits();
        for (const auto& nameUnitPair : propertyNamesUnitsOnFile)
        {
            if (nameUnitPair.first.contains(RiaDefines::conductivityResultName(), Qt::CaseInsensitive))
            {
                conductivityUnit = nameUnitPair.second;
            }
            else
            {
                tmp.push_back(nameUnitPair);
            }
        }

        propertyNamesAndUnits.push_back(std::make_pair(RiaDefines::conductivityResultName(), conductivityUnit));

        for (const auto& nameUnitPair : tmp)
        {
            propertyNamesAndUnits.push_back(nameUnitPair);
        }
    }

    return propertyNamesAndUnits;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<std::vector<double>> RimStimPlanFractureTemplate::resultValues(const QString& uiResultName, const QString& unitName, size_t timeStepIndex) const
{
    if (m_stimPlanFractureDefinitionData.notNull())
    {
        QString fileResultName = mapUiResultNameToFileResultName(uiResultName);

        return m_stimPlanFractureDefinitionData->getDataAtTimeIndex(fileResultName, unitName, timeStepIndex);
    }

    return std::vector<std::vector<double>>();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<double> RimStimPlanFractureTemplate::fractureGridResults(const QString& uiResultName, const QString& unitName, size_t timeStepIndex) const
{
    if (m_stimPlanFractureDefinitionData.notNull())
    {
        QString fileResultName = mapUiResultNameToFileResultName(uiResultName);

        return m_stimPlanFractureDefinitionData->fractureGridResults(fileResultName, unitName, timeStepIndex);
    }

   return std::vector<double>();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimStimPlanFractureTemplate::hasConductivity() const
{
    if (m_stimPlanFractureDefinitionData.notNull() &&
        !m_stimPlanFractureDefinitionData->conductivityResultNames().isEmpty())
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
double RimStimPlanFractureTemplate::resultValueAtIJ(const QString& uiResultName, const QString& unitName, size_t timeStepIndex, size_t i, size_t j)
{
    auto values = resultValues(uiResultName, unitName, timeStepIndex);

    if (values.empty()) return HUGE_VAL;

    size_t adjustedI = i + 1;
    size_t adjustedJ = j + 1;

    if (adjustedI >= fractureGrid()->iCellCount() || adjustedJ >= fractureGrid()->jCellCount())
    {

        return HUGE_VAL;
    }

    return values[adjustedJ][adjustedI];
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::appendDataToResultStatistics(const QString& uiResultName, const QString& unit,
                                                                MinMaxAccumulator& minMaxAccumulator,
                                                                PosNegAccumulator& posNegAccumulator) const
{
    if (m_stimPlanFractureDefinitionData.notNull())
    {
        QString fileResultName = mapUiResultNameToFileResultName(uiResultName);

        m_stimPlanFractureDefinitionData->appendDataToResultStatistics(fileResultName, unit, minMaxAccumulator, posNegAccumulator);
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const RigFractureGrid* RimStimPlanFractureTemplate::fractureGrid() const
{
    return m_fractureGrid.p();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::updateFractureGrid()
{
    m_fractureGrid = nullptr;

    if (m_stimPlanFractureDefinitionData.notNull())
    {
        m_fractureGrid = m_stimPlanFractureDefinitionData->createFractureGrid(m_conductivityResultNameOnFile,
                                                                              m_activeTimeStepIndex,
                                                                              m_wellPathDepthAtFracture,
                                                                              m_fractureTemplateUnit());
    }
}



//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::fractureTriangleGeometry(std::vector<cvf::Vec3f>* nodeCoords, 
                                                           std::vector<cvf::uint>* triangleIndices)
{

    if (m_stimPlanFractureDefinitionData.isNull())
    {
        loadDataAndUpdate();
    }
    else	
    {
        m_stimPlanFractureDefinitionData->createFractureTriangleGeometry(m_wellPathDepthAtFracture,
                                                                         name(),
                                                                         nodeCoords,
                                                                         triangleIndices);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<cvf::Vec3f> RimStimPlanFractureTemplate::fractureBorderPolygon()
{
    // Not implemented
    return std::vector<cvf::Vec3f>();
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    uiOrdering.add(&m_name);
    uiOrdering.add(&m_id);

    caf::PdmUiGroup* fileGroup = uiOrdering.addNewGroup("Input");
    fileGroup->add(&m_stimPlanFileName);
    fileGroup->add(&m_activeTimeStepIndex);
    fileGroup->add(&m_wellPathDepthAtFracture);

    caf::PdmUiGroup* geometryGroup = uiOrdering.addNewGroup("Geometry");
    geometryGroup->add(&m_orientationType);
    geometryGroup->add(&m_azimuthAngle);

    caf::PdmUiGroup* trGr = uiOrdering.addNewGroup("Fracture Truncation");
    m_fractureContainment()->defineUiOrdering(uiConfigName, *trGr);

    caf::PdmUiGroup* propertyGroup = uiOrdering.addNewGroup("Properties");
    propertyGroup->add(&m_conductivityResultNameOnFile);
    propertyGroup->add(&m_conductivityType);
    propertyGroup->add(&m_skinFactor);
    propertyGroup->add(&m_perforationLength);
    propertyGroup->add(&m_perforationEfficiency);
    propertyGroup->add(&m_wellDiameter);

    RimFractureTemplate::defineUiOrdering(uiConfigName, uiOrdering);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimStimPlanFractureTemplate::defineEditorAttribute(const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute * attribute)
{
    RimFractureTemplate::defineEditorAttribute(field, uiConfigName, attribute);

    if (field == &m_stimPlanFileName)
    {
        caf::PdmUiFilePathEditorAttribute* myAttr = dynamic_cast<caf::PdmUiFilePathEditorAttribute*>(attribute);
        if (myAttr)
        {
            myAttr->m_fileSelectionFilter = "StimPlan Xml Files(*.xml);;All Files (*.*)";
        }
    }

    if (field == &m_wellPathDepthAtFracture)
    {
        if ( !m_stimPlanFractureDefinitionData.isNull() && (m_stimPlanFractureDefinitionData->yCount() > 0) )
        {
            caf::PdmUiDoubleSliderEditorAttribute* myAttr = dynamic_cast<caf::PdmUiDoubleSliderEditorAttribute*>(attribute);
            if ( myAttr )
            {
                myAttr->m_minimum = m_stimPlanFractureDefinitionData->minDepth();
                myAttr->m_maximum = m_stimPlanFractureDefinitionData->maxDepth();
            }
        }
    }
}
