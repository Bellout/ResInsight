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

#include "RimWellLogPlot.h"

#include "RigWellPathFormations.h"

#include "cafPdmObject.h"
#include "cafPdmField.h"
#include "cafPdmChildArrayField.h"
#include "cafPdmPtrField.h"

#include <QPointer>

#include <memory>
#include <vector>
#include <map>

class RigWellPath;
class RimCase;
class RimWellFlowRateCurve;
class RimWellLogCurve;
class RimWellPath;
class RiuPlotAnnotationTool;
class RiuWellLogTrack;
class RigEclipseWellLogExtractor;
class RimWellLogPlotCollection;
class RigGeoMechWellLogExtractor;
class RigResultAccessor;
class RigFemResultAddress;

class QwtPlotCurve;

struct CurveSamplingPointData
{
    std::vector<double> data;
    std::vector<double> md;
    std::vector<double> tvd;
};


//==================================================================================================
///  
///  
//==================================================================================================
class RimWellLogTrack : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;
public:
    RimWellLogTrack();
    virtual ~RimWellLogTrack();

    enum TrajectoryType { WELL_PATH, SIMULATION_WELL };
    enum FormationSource { CASE, WELL_PICK_FILTER };

    void setDescription(const QString& description);
    bool isVisible();
    void addCurve(RimWellLogCurve* curve);
    void insertCurve(RimWellLogCurve* curve, size_t index);
    void takeOutCurve(RimWellLogCurve* curve);
    void deleteAllCurves();

    size_t curveIndex(RimWellLogCurve* curve);
    size_t curveCount() { return curves.size(); }
    void setXAxisTitle(const QString& text);

    void setFormationWellPath(RimWellPath* wellPath);
    void setFormationSimWellName(const QString& simWellName);
    void setFormationBranchIndex(int branchIndex);
    void setFormationCase(RimCase* rimCase);
    void setFormationTrajectoryType(TrajectoryType trajectoryType);
    RimCase* formationNamesCase() const; 

    void recreateViewer();
    void detachAllCurves();

    void loadDataAndUpdate();
    
    void setAndUpdateWellPathFormationNamesData(RimCase* rimCase, RimWellPath* wellPath);
    
    void setAndUpdateSimWellFormationNamesAndBranchData(RimCase* rimCase, const QString& simWellName, int branchIndex, bool useBranchDetection);
    void setAndUpdateSimWellFormationNamesData(RimCase* rimCase, const QString& simWellName);
    
    void availableDepthRange(double* minimumDepth, double* maximumDepth);
    void updateXZoomAndParentPlotDepthZoom();
    void updateXZoom();

    void setShowFormations(bool on);

    RiuWellLogTrack* viewer();
    
    RimWellLogCurve* curveDefinitionFromCurve(const QwtPlotCurve* curve) const;

    void setLogarithmicScale(bool enable);

    std::map<int, std::vector<RimWellFlowRateCurve*>> visibleStackedCurves();

    QString description();
    std::vector<RimWellLogCurve* > curvesVector();

    void uiOrderingForRftPltFormations(caf::PdmUiOrdering& uiOrdering);
    void uiOrderingForVisibleXRange(caf::PdmUiOrdering& uiOrdering);

    void setFormationsForCaseWithSimWellOnly(bool caseWithSimWellOnly);

private:
    virtual void fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue) override;
    virtual QList<caf::PdmOptionItemInfo> calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool * useOptionsOnly) override;

    virtual caf::PdmFieldHandle* objectToggleField() override;
    virtual caf::PdmFieldHandle* userDescriptionField() override;
    virtual void defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering) override;

    void computeAndSetXRangeMinForLogarithmicScale();
    
    static void simWellOptionItems(QList<caf::PdmOptionItemInfo>* options, RimCase* eclCase);
    
    static RigEclipseWellLogExtractor* createSimWellExtractor(RimWellLogPlotCollection* wellLogCollection, RimCase* rimCase, const QString& simWellName, int branchIndex, bool useBranchDetection);
    static RigEclipseWellLogExtractor* createWellPathExtractor(RimWellLogPlotCollection* wellLogCollection, RimCase* rimCase, RimWellPath* wellPath);
    static RigGeoMechWellLogExtractor* createGeoMechExtractor(RimWellLogPlotCollection* wellLogCollection, RimCase* rimCase, RimWellPath* wellPath);

    static CurveSamplingPointData curveSamplingPointData(RigEclipseWellLogExtractor* extractor, RigResultAccessor* resultAccessor);
    static CurveSamplingPointData curveSamplingPointData(RigGeoMechWellLogExtractor* extractor, const RigFemResultAddress& resultAddress);

    static std::vector<QString> formationNameIndexToName(RimCase* rimCase, const std::vector<int>& formationNameInidces);

    static void findFormationNamesToPlot(const CurveSamplingPointData&           curveData,
                                         const std::vector<QString>&             formationNamesVector,
                                         RimWellLogPlot::DepthTypeEnum           depthType,
                                         std::vector<QString>*                   formationNamesToPlot,
                                         std::vector<std::pair<double, double>>* yValues);

    static std::vector<QString> formationNamesVector(RimCase* rimCase);

    void setFormationFieldsUiReadOnly(bool readOnly = true);

    void updateFormationNamesOnPlot();
    void removeFormationNames();
    void updateAxisScaleEngine();

private:
    QString m_xAxisTitle;

    caf::PdmField<bool> m_show;
    caf::PdmField<QString> m_userName;
    caf::PdmChildArrayField<RimWellLogCurve*> curves;
    caf::PdmField<double> m_visibleXRangeMin;
    caf::PdmField<double> m_visibleXRangeMax;
    caf::PdmField<bool>   m_isAutoScaleXEnabled;
    caf::PdmField<bool>   m_isLogarithmicScaleEnabled;

    caf::PdmField<bool>                                                m_showFormations;
    caf::PdmField<caf::AppEnum<FormationSource> >                      m_formationSource;
    caf::PdmPtrField<RimCase*>                                         m_formationCase;
    caf::PdmField<caf::AppEnum<TrajectoryType> >                       m_formationTrajectoryType;
    caf::PdmPtrField<RimWellPath*>                                     m_formationWellPathForSourceCase;
    caf::PdmPtrField<RimWellPath*>                                     m_formationWellPathForSourceWellPath;
    caf::PdmField<QString>                                             m_formationSimWellName;
    caf::PdmField<int>                                                 m_formationBranchIndex;
    caf::PdmField<caf::AppEnum<RigWellPathFormations::FormationLevel>> m_formationLevel;
    caf::PdmField<bool>                                                m_showformationFluids;

    caf::PdmField<bool> m_formationBranchDetection;

    bool m_formationsForCaseWithSimWellOnly;

    QPointer<RiuWellLogTrack> m_wellLogTrackPlotWidget;
    
    std::unique_ptr<RiuPlotAnnotationTool> m_annotationTool;
};
