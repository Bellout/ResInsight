/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-     Statoil ASA
//  Copyright (C) 2013-     Ceetron Solutions AS
//  Copyright (C) 2011-2012 Ceetron AS
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

#include "RimEclipseResultDefinition.h"

#include "RigActiveCellInfo.h"
#include "RigCaseCellResultsData.h"
#include "RigEclipseCaseData.h"
#include "RigFlowDiagResultAddress.h"

#include "RimCellEdgeColors.h"
#include "RimEclipseCase.h"
#include "RimEclipseCellColors.h"
#include "RimEclipseFaultColors.h"
#include "RimEclipsePropertyFilter.h"
#include "RimEclipseResultCase.h"
#include "RimEclipseView.h"
#include "RimFlowDiagSolution.h"
#include "RimIntersectionCollection.h"
#include "RimPlotCurve.h"
#include "RimReservoirCellResultsStorage.h"
#include "Rim3dView.h"
#include "RimViewLinker.h"
#include "RimGridTimeHistoryCurve.h"
#include "RimWellLogExtractionCurve.h"

#include "cafPdmUiListEditor.h"
#include "cafUtils.h"

namespace caf
{
    template<>
    void RimEclipseResultDefinition::FlowTracerSelectionEnum::setUp()
    {
        addItem(RimEclipseResultDefinition::FLOW_TR_INJ_AND_PROD,   "FLOW_TR_INJ_AND_PROD", "All Injectors and Producers");
        addItem(RimEclipseResultDefinition::FLOW_TR_PRODUCERS,      "FLOW_TR_PRODUCERS",    "All Producers");
        addItem(RimEclipseResultDefinition::FLOW_TR_INJECTORS,      "FLOW_TR_INJECTORS",    "All Injectors");
        addItem(RimEclipseResultDefinition::FLOW_TR_BY_SELECTION,   "FLOW_TR_BY_SELECTION", "By Selection");

        setDefault(RimEclipseResultDefinition::FLOW_TR_INJ_AND_PROD);
    }
}


CAF_PDM_SOURCE_INIT(RimEclipseResultDefinition, "ResultDefinition");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimEclipseResultDefinition::RimEclipseResultDefinition() 
{
    CAF_PDM_InitObject("Result Definition", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_resultType,     "ResultType",           "Type", "", "", "");
    m_resultType.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_porosityModel,  "PorosityModelType",    "Porosity", "", "", "");
    m_porosityModel.uiCapability()->setUiHidden(true);

    CAF_PDM_InitField(&m_resultVariable, "ResultVariable", RiaDefines::undefinedResultName(), "Variable", "", "", "" );
    m_resultVariable.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_flowSolution, "FlowDiagSolution", "Solution", "", "", "");
    m_flowSolution.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_selectedTracers, "SelectedTracers", "Tracers", "", "", "");
    m_selectedTracers.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_selectedSouringTracers, "SelectedSouringTracers", "Tracers", "", "", "");
    m_selectedSouringTracers.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_flowTracerSelectionMode, "FlowTracerSelectionMode", "Tracers", "", "", "");
    CAF_PDM_InitFieldNoDefault(&m_phaseSelection, "PhaseSelection", "Phases", "", "", "");

    // Ui only fields

    CAF_PDM_InitFieldNoDefault(&m_resultTypeUiField,     "MResultType",           "Type", "", "", "");
    m_resultTypeUiField.xmlCapability()->setIOReadable(false);
    m_resultTypeUiField.xmlCapability()->setIOWritable(false);

    CAF_PDM_InitFieldNoDefault(&m_porosityModelUiField,  "MPorosityModelType",    "Porosity", "", "", "");
    m_porosityModelUiField.xmlCapability()->setIOReadable(false);
    m_porosityModelUiField.xmlCapability()->setIOWritable(false);

    CAF_PDM_InitField(&m_resultVariableUiField, "MResultVariable", RiaDefines::undefinedResultName(), "Property", "", "", "" );
    m_resultVariableUiField.xmlCapability()->setIOReadable(false);
    m_resultVariableUiField.xmlCapability()->setIOWritable(false);
    m_resultVariableUiField.uiCapability()->setUiEditorTypeName(caf::PdmUiListEditor::uiEditorTypeName());


    CAF_PDM_InitFieldNoDefault(&m_flowSolutionUiField, "MFlowDiagSolution", "Solution", "", "", "");
    m_flowSolutionUiField.xmlCapability()->setIOReadable(false);
    m_flowSolutionUiField.xmlCapability()->setIOWritable(false);
    m_flowSolutionUiField.uiCapability()->setUiHidden(true); // For now since there are only one to choose from
 
    CAF_PDM_InitFieldNoDefault(&m_selectedTracersUiField, "MSelectedTracers", " ", "", "", "");
    m_selectedTracersUiField.xmlCapability()->setIOReadable(false);
    m_selectedTracersUiField.xmlCapability()->setIOWritable(false);
    m_selectedTracersUiField.uiCapability()->setUiEditorTypeName(caf::PdmUiListEditor::uiEditorTypeName());
 
    CAF_PDM_InitFieldNoDefault(&m_selectedSouringTracersUiField, "MSelectedSouringTracers", "Tracers", "", "", "");
    m_selectedSouringTracersUiField.xmlCapability()->setIOReadable(false);
    m_selectedSouringTracersUiField.xmlCapability()->setIOWritable(false);
    m_selectedSouringTracersUiField.uiCapability()->setUiEditorTypeName(caf::PdmUiListEditor::uiEditorTypeName());

    CAF_PDM_InitFieldNoDefault(&m_selectedTracersUiFieldFilter, "SelectedTracersFilter", "Filter", "", "", "");
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimEclipseResultDefinition::~RimEclipseResultDefinition()
{
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::simpleCopy(const RimEclipseResultDefinition* other)
{
    this->setResultVariable(other->resultVariable());
    this->setPorosityModel(other->porosityModel());
    this->setResultType(other->resultType());
    this->setFlowSolution(other->m_flowSolution());
    this->setSelectedTracers(other->m_selectedTracers());
    this->setSelectedSouringTracers(other->m_selectedSouringTracers());
    m_flowTracerSelectionMode = other->m_flowTracerSelectionMode();
    m_phaseSelection = other->m_phaseSelection;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::setEclipseCase(RimEclipseCase* eclipseCase)
{
     m_eclipseCase = eclipseCase;
   
     assignFlowSolutionFromCase();
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RigCaseCellResultsData* RimEclipseResultDefinition::currentGridCellResults() const
{
    if (!m_eclipseCase ) return nullptr;

    return m_eclipseCase->results(m_porosityModel());
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{
    if (   &m_flowSolutionUiField  == changedField 
        || &m_resultTypeUiField    == changedField
        || &m_porosityModelUiField == changedField )
    {
        // If the user are seeing the list with the actually selected result, 
        // select that result in the list. Otherwise select nothing.

        QStringList varList = getResultNamesForCurrentUiResultType();

        bool isFlowDiagFieldsRelevant = (m_resultType() == RiaDefines::FLOW_DIAGNOSTICS);
        

        if (   ( m_flowSolutionUiField() == m_flowSolution() || !isFlowDiagFieldsRelevant)
            &&  m_resultTypeUiField()    == m_resultType()
            &&  m_porosityModelUiField() == m_porosityModel() )
        {
            if (varList.contains(resultVariable()))
            {
                m_resultVariableUiField = resultVariable();
            }

            if (isFlowDiagFieldsRelevant) m_selectedTracersUiField = m_selectedTracers();
            else                          m_selectedTracersUiField = std::vector<QString>();
        }
        else
        {
            m_resultVariableUiField = "";
            m_selectedTracersUiField = std::vector<QString>();
        }
    }

    if (&m_resultVariableUiField == changedField)
    {
        m_porosityModel  = m_porosityModelUiField;
        m_resultType     = m_resultTypeUiField;
        m_resultVariable = m_resultVariableUiField;
        
        if (m_resultTypeUiField() == RiaDefines::FLOW_DIAGNOSTICS)
        {
            m_flowSolution = m_flowSolutionUiField();
            m_selectedTracers = m_selectedTracersUiField();
        }
        else if (m_resultTypeUiField() == RiaDefines::INJECTION_FLOODING)
        {
            m_selectedSouringTracers = m_selectedSouringTracersUiField();
        }
        loadDataAndUpdate();
    }

    if ( &m_selectedTracersUiField == changedField )
    {
        m_flowSolution = m_flowSolutionUiField();

        if (m_selectedTracersUiFieldFilter().isEmpty())
        {
            m_selectedTracers = m_selectedTracersUiField();
        }
        else
        {
            auto filteredTracerNames = tracerNamesMatchingFilter();

            // Keep selected strings not part of currently visible selection items
            std::vector<QString> newSelection;
            for (auto selectedTracer : m_selectedTracers())
            {
                if (std::find(begin(filteredTracerNames), end(filteredTracerNames), selectedTracer) == end(filteredTracerNames))
                {
                    newSelection.push_back(selectedTracer);
                }
            }

            for (auto selectedTracerUi : m_selectedTracersUiField())
            {
                newSelection.push_back(selectedTracerUi);
            }

            m_selectedTracers = newSelection;
        }

        loadDataAndUpdate();
    }

    if (&m_selectedSouringTracersUiField == changedField)
    {
        if (!m_resultVariable().isEmpty())
        {
            m_selectedSouringTracers = m_selectedSouringTracersUiField();
            loadDataAndUpdate();
        }
    }

    if (&m_phaseSelection == changedField)
    {
        if (m_phaseSelection() != RigFlowDiagResultAddress::PHASE_ALL)
        {
            m_resultType = m_resultTypeUiField;
            m_resultVariable = RIG_FLD_TOF_RESNAME;
            m_resultVariableUiField = RIG_FLD_TOF_RESNAME;
        }
        loadDataAndUpdate();
    }

    if (&m_selectedTracersUiFieldFilter == changedField)
    {
        auto visibleTracerNames = tracerNamesMatchingFilter();

        std::vector<QString> subSelection;

        // Remove hidden items from selection
        for (auto selectedTracer : m_selectedTracers())
        {
            if (std::find(begin(visibleTracerNames), end(visibleTracerNames), selectedTracer) != end(visibleTracerNames))
            {
                subSelection.push_back(selectedTracer);
            }
        }

        m_selectedTracersUiField = subSelection;

        updateConnectedEditors();
    }

    updateAnyFieldHasChanged();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::updateAnyFieldHasChanged()
{
    RimEclipsePropertyFilter* propFilter = nullptr;
    this->firstAncestorOrThisOfType(propFilter);
    if (propFilter)
    {
        propFilter->updateConnectedEditors();
    }
    
    RimEclipseFaultColors* faultColors = nullptr;
    this->firstAncestorOrThisOfType(faultColors);
    if (faultColors)
    {
        faultColors->updateConnectedEditors();
    }

    RimCellEdgeColors* cellEdgeColors = nullptr;
    this->firstAncestorOrThisOfType(cellEdgeColors);
    if (cellEdgeColors)
    {
        cellEdgeColors->updateConnectedEditors();
    }

    RimEclipseCellColors* cellColors = nullptr;
    this->firstAncestorOrThisOfType(cellColors);
    if (cellColors)
    {
        cellColors->updateConnectedEditors();
    }

    RimPlotCurve* curve = nullptr;
    this->firstAncestorOrThisOfType(curve);
    if (curve)
    {
        curve->updateConnectedEditors();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::setTofAndSelectTracer(const QString& tracerName)
{
    setResultType(RiaDefines::FLOW_DIAGNOSTICS);
    setResultVariable("TOF");
    setFlowDiagTracerSelectionType(FLOW_TR_BY_SELECTION);

    std::vector<QString> tracers;
    tracers.push_back(tracerName);
    setSelectedTracers(tracers);

    if (m_flowSolution() == nullptr)
    {
        assignFlowSolutionFromCase();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::assignFlowSolutionFromCase()
{
    RimFlowDiagSolution* defaultFlowDiagSolution = nullptr;

    RimEclipseResultCase* eclCase = dynamic_cast<RimEclipseResultCase*>(m_eclipseCase.p());

    if (eclCase)
    {
        defaultFlowDiagSolution = eclCase->defaultFlowDiagSolution();
    }
    this->setFlowSolution(defaultFlowDiagSolution);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::loadDataAndUpdate()
{
    Rim3dView* view = nullptr;
    this->firstAncestorOrThisOfType(view);

    loadResult();

    RimEclipsePropertyFilter* propFilter = nullptr;
    this->firstAncestorOrThisOfType(propFilter);
    if (propFilter)
    {
        propFilter->setToDefaultValues();
        propFilter->updateFilterName();

        if (view)
        {
            view->scheduleGeometryRegen(PROPERTY_FILTERED);
            view->scheduleCreateDisplayModelAndRedraw();
        }
    }

    RimEclipseCellColors* cellColors = nullptr;
    this->firstAncestorOrThisOfType(cellColors);
    if (cellColors)
    {
        this->updateLegendCategorySettings();

        if (view)
        {
            RimViewLinker* viewLinker = view->assosiatedViewLinker();
            if (viewLinker)
            {
                viewLinker->updateCellResult();
            }
            RimGridView* eclView = dynamic_cast<RimGridView*>(view);
            if (eclView) eclView->crossSectionCollection()->scheduleCreateDisplayModelAndRedraw2dIntersectionViews();
        }
    }

    RimCellEdgeColors* cellEdgeColors = nullptr;
    this->firstAncestorOrThisOfType(cellEdgeColors);
    if (cellEdgeColors)
    {
        cellEdgeColors->singleVarEdgeResultColors()->updateLegendCategorySettings();
        cellEdgeColors->loadResult();

        if (view)
        {
            view->scheduleCreateDisplayModelAndRedraw();
        }
    }

    RimPlotCurve* curve = nullptr;
    this->firstAncestorOrThisOfType(curve);
    if (curve)
    {
        curve->loadDataAndUpdate(true);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimEclipseResultDefinition::calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool* useOptionsOnly)
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &m_resultTypeUiField )
    {
        bool hasSourSimRLFile = false;
        RimEclipseResultCase* eclResCase = dynamic_cast<RimEclipseResultCase*>(m_eclipseCase.p());
        if ( eclResCase && eclResCase->eclipseCaseData() )
        {
            hasSourSimRLFile = eclResCase->hasSourSimFile();
        }

#ifndef USE_HDF5
        // If using ResInsight without HDF5 support, ignore SourSim files and
        // do not show it as a result category.
        hasSourSimRLFile = false;
#endif

        bool enableSouring = false;

#ifdef ENABLE_SOURING
        if (m_eclipseCase.notNull())
        {
            RigCaseCellResultsData* cellResultsData = m_eclipseCase->results(this->porosityModel());

            if (cellResultsData->hasFlowDiagUsableFluxes())
            {
                enableSouring = true;
            }
        }
#endif /* ENABLE_SOURING */


        RimGridTimeHistoryCurve* timeHistoryCurve;
        this->firstAncestorOrThisOfType(timeHistoryCurve);

        // Do not include flow diagnostics results if it is a time history curve
        // Do not include SourSimRL if no SourSim file is loaded
        if ( timeHistoryCurve != nullptr || !hasSourSimRLFile || !enableSouring )
        {
            using ResCatEnum = caf::AppEnum< RiaDefines::ResultCatType >;
            for ( size_t i = 0; i < ResCatEnum::size(); ++i )
            {
                RiaDefines::ResultCatType resType = ResCatEnum::fromIndex(i);
                if ( resType == RiaDefines::FLOW_DIAGNOSTICS 
                    && (timeHistoryCurve) )
                {
                        continue;
                }

                if ( resType == RiaDefines::SOURSIMRL 
                    && (!hasSourSimRLFile ) )
                {
                    continue;
                }

                if (resType == RiaDefines::INJECTION_FLOODING && !enableSouring)
                {
                    continue;
                }

                QString uiString = ResCatEnum::uiTextFromIndex(i);
                options.push_back(caf::PdmOptionItemInfo(uiString, resType));
            }
        }
    }

    if ( m_resultTypeUiField() == RiaDefines::FLOW_DIAGNOSTICS )
    {
        if ( fieldNeedingOptions == &m_resultVariableUiField )
        {
            options.push_back(caf::PdmOptionItemInfo("Time Of Flight (Average)", RIG_FLD_TOF_RESNAME));
            if (m_phaseSelection() == RigFlowDiagResultAddress::PHASE_ALL)
            {
                options.push_back(caf::PdmOptionItemInfo("Tracer Cell Fraction (Sum)", RIG_FLD_CELL_FRACTION_RESNAME));
                options.push_back(caf::PdmOptionItemInfo("Max Fraction Tracer", RIG_FLD_MAX_FRACTION_TRACER_RESNAME));
                options.push_back(caf::PdmOptionItemInfo("Injector Producer Communication", RIG_FLD_COMMUNICATION_RESNAME));
            }
        }
        else if (fieldNeedingOptions == &m_flowSolutionUiField)
        {
            RimEclipseResultCase* eclCase = dynamic_cast<RimEclipseResultCase*>(m_eclipseCase.p());
            if (eclCase)
            {
                std::vector<RimFlowDiagSolution*> flowSols = eclCase->flowDiagSolutions();
                for (RimFlowDiagSolution* flowSol : flowSols)
                {
                    options.push_back(caf::PdmOptionItemInfo(flowSol->userDescription(), flowSol));
                }
            }           
        }
        else if (fieldNeedingOptions == &m_selectedTracersUiField)
        {
            RimFlowDiagSolution* flowSol = m_flowSolutionUiField();
            if (flowSol)
            {
                std::vector<QString> tracerNames = tracerNamesMatchingFilter();
                std::map<QString, QString> prefixedTracerNamesMap;
                for ( const QString& tracerName : tracerNames )
                {
                    RimFlowDiagSolution::TracerStatusType status = flowSol->tracerStatusOverall(tracerName);
                    QString prefix; 
                    switch ( status )
                    {
                        case RimFlowDiagSolution::INJECTOR: prefix = "I   : "; break;
                        case RimFlowDiagSolution::PRODUCER: prefix = "P  : "; break;
                        case RimFlowDiagSolution::VARYING:  prefix = "I/P: "; break;
                        case RimFlowDiagSolution::UNDEFINED:prefix = "U  : "; break;
                    }

                    if (status != RimFlowDiagSolution::CLOSED) prefixedTracerNamesMap[prefix + tracerName] = tracerName;
                }

                for (auto nameIt: prefixedTracerNamesMap)
                {
                    options.push_back(caf::PdmOptionItemInfo(nameIt.first, QVariant(nameIt.second)));
                }
            }
        }
    }
    else if (m_resultTypeUiField() == RiaDefines::INJECTION_FLOODING)
    {
        if (fieldNeedingOptions == &m_selectedSouringTracersUiField)
        {
            RigCaseCellResultsData* cellResultsStorage = currentGridCellResults();
            if (cellResultsStorage)
            {
                QStringList dynamicResultNames = cellResultsStorage->resultNames(RiaDefines::DYNAMIC_NATIVE);

                for (const QString& resultName : dynamicResultNames)
                {
                    if (!resultName.endsWith("F") ||
                        resultName == RiaDefines::completionTypeResultName())
                    {
                        continue;
                    }
                    options.push_back(caf::PdmOptionItemInfo(resultName, resultName));
                }
            }
        }
        else if (fieldNeedingOptions == &m_resultVariableUiField)
        {
            options.push_back(caf::PdmOptionItemInfo(RIG_NUM_FLOODED_PV, RIG_NUM_FLOODED_PV));
        }
    }
    else
    {
        if ( fieldNeedingOptions == &m_resultVariableUiField )
        {
            options = calcOptionsForVariableUiFieldStandard();
        }
    }


    (*useOptionsOnly) = true;
    
    return options;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimEclipseResultDefinition::calcOptionsForVariableUiFieldStandard()
{
    CVF_ASSERT(m_resultTypeUiField() != RiaDefines::FLOW_DIAGNOSTICS && m_resultTypeUiField() != RiaDefines::INJECTION_FLOODING);

    if (this->currentGridCellResults())
    {
        QList<caf::PdmOptionItemInfo> optionList;

        QStringList cellCenterResultNames;
        QStringList cellFaceResultNames;

        RigCaseCellResultsData* results = this->currentGridCellResults();
        foreach(QString s, getResultNamesForCurrentUiResultType())
        {
            if (s == RiaDefines::completionTypeResultName() && results->timeStepDates().empty()) continue;

            if (RiaDefines::isPerCellFaceResult(s))
            {
                cellFaceResultNames.push_back(s);
            }
            else
            {
                cellCenterResultNames.push_back(s);
            }
        }

        cellCenterResultNames.sort();
        cellFaceResultNames.sort();

        // Cell Center result names
        foreach(QString s, cellCenterResultNames)
        {
            optionList.push_back(caf::PdmOptionItemInfo(s, s));
        }

        // Ternary Result
        bool hasAtLeastOneTernaryComponent = false;
        if (cellCenterResultNames.contains("SOIL")) hasAtLeastOneTernaryComponent = true;
        else if (cellCenterResultNames.contains("SGAS")) hasAtLeastOneTernaryComponent = true;
        else if (cellCenterResultNames.contains("SWAT")) hasAtLeastOneTernaryComponent = true;

        if (m_resultTypeUiField == RiaDefines::DYNAMIC_NATIVE && hasAtLeastOneTernaryComponent)
        {
            optionList.push_front(caf::PdmOptionItemInfo(RiaDefines::ternarySaturationResultName(), RiaDefines::ternarySaturationResultName()));
        }

        // Cell Face result names
        bool showDerivedResultsFirstInList = false;
        {
            RimEclipseFaultColors* rimEclipseFaultColors = nullptr;
            this->firstAncestorOrThisOfType(rimEclipseFaultColors);

            if ( rimEclipseFaultColors ) showDerivedResultsFirstInList = true;
        }

        foreach(QString s, cellFaceResultNames)
        {
            if (showDerivedResultsFirstInList)
            {
                optionList.push_front(caf::PdmOptionItemInfo(s, s));
            }
            else
            {
                optionList.push_back(caf::PdmOptionItemInfo(s, s));
            }
        }

        optionList.push_front(caf::PdmOptionItemInfo(RiaDefines::undefinedResultName(), RiaDefines::undefinedResultName()));

        // Remove Per Cell Face options
        {
            RimPlotCurve* curve = nullptr;
            this->firstAncestorOrThisOfType(curve);

            RimEclipsePropertyFilter* propFilter = nullptr;
            this->firstAncestorOrThisOfType(propFilter);

            RimCellEdgeColors* cellEdge = nullptr;
            this->firstAncestorOrThisOfType(cellEdge);

            if ( propFilter || curve || cellEdge )
            {
                removePerCellFaceOptionItems(optionList);
            }
        }

        return optionList;
    }

    return QList<caf::PdmOptionItemInfo>();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QStringList RimEclipseResultDefinition::getResultNamesForCurrentUiResultType()
{
    if ( m_resultTypeUiField() != RiaDefines::FLOW_DIAGNOSTICS )
    {
        RigCaseCellResultsData* cellResultsStorage = currentGridCellResults();

        if ( !cellResultsStorage ) return QStringList();

        return cellResultsStorage->resultNames(m_resultTypeUiField());
    }
    else
    {
        QStringList flowVars;
        flowVars.push_back(RIG_FLD_TOF_RESNAME);
        flowVars.push_back(RIG_FLD_CELL_FRACTION_RESNAME);
        flowVars.push_back(RIG_FLD_MAX_FRACTION_TRACER_RESNAME);
        flowVars.push_back(RIG_FLD_COMMUNICATION_RESNAME);
        return flowVars;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
size_t RimEclipseResultDefinition::scalarResultIndex() const
{
    size_t gridScalarResultIndex = cvf::UNDEFINED_SIZE_T;

    if (isFlowDiagOrInjectionFlooding()) return cvf::UNDEFINED_SIZE_T;

    const RigCaseCellResultsData* gridCellResults = this->currentGridCellResults();
    if (gridCellResults )
    {
        gridScalarResultIndex = gridCellResults->findScalarResultIndex(m_resultType(), m_resultVariable());
    }

    return gridScalarResultIndex;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RigFlowDiagResultAddress RimEclipseResultDefinition::flowDiagResAddress() const
{
    CVF_ASSERT(isFlowDiagOrInjectionFlooding());

    if (m_resultType() == RiaDefines::FLOW_DIAGNOSTICS)
    {
        size_t timeStep = 0;

        Rim3dView* rimView = nullptr;
        this->firstAncestorOrThisOfType(rimView);
        if (rimView)
        {
            timeStep = rimView->currentTimeStep();
        }
        RimWellLogExtractionCurve* wellLogExtractionCurve = nullptr;
        this->firstAncestorOrThisOfType(wellLogExtractionCurve);
        if (wellLogExtractionCurve)
        {
            timeStep = static_cast<size_t>(wellLogExtractionCurve->currentTimeStep());
        }

        // Time history curves are not supported, since it requires the time 
        // step to access to be supplied.
        RimGridTimeHistoryCurve* timeHistoryCurve = nullptr;
        this->firstAncestorOrThisOfType(timeHistoryCurve);
        CVF_ASSERT(timeHistoryCurve == nullptr);

        std::set<std::string> selTracerNames;
        if (m_flowTracerSelectionMode == FLOW_TR_BY_SELECTION)
        {
            for (const QString& tName : m_selectedTracers())
            {
                selTracerNames.insert(tName.toStdString());
            }
        }
        else
        {
            RimFlowDiagSolution* flowSol = m_flowSolution();
            if (flowSol)
            {
                std::vector<QString> tracerNames = flowSol->tracerNames();

                if (m_flowTracerSelectionMode == FLOW_TR_INJECTORS || m_flowTracerSelectionMode == FLOW_TR_INJ_AND_PROD)
                {
                    for (const QString& tracerName : tracerNames)
                    {
                        RimFlowDiagSolution::TracerStatusType status = flowSol->tracerStatusInTimeStep(tracerName, timeStep);
                        if (status == RimFlowDiagSolution::INJECTOR)
                        {
                            selTracerNames.insert(tracerName.toStdString());
                        }
                    }
                }

                if (m_flowTracerSelectionMode == FLOW_TR_PRODUCERS || m_flowTracerSelectionMode == FLOW_TR_INJ_AND_PROD)
                {
                    for (const QString& tracerName : tracerNames)
                    {
                        RimFlowDiagSolution::TracerStatusType status = flowSol->tracerStatusInTimeStep(tracerName, timeStep);
                        if (status == RimFlowDiagSolution::PRODUCER)
                        {
                            selTracerNames.insert(tracerName.toStdString());
                        }
                    }
                }
            }
        }

        return RigFlowDiagResultAddress(m_resultVariable().toStdString(), m_phaseSelection(), selTracerNames);
    }
    else
    {
        std::set<std::string> selTracerNames;
        for (const QString& selectedTracerName : m_selectedSouringTracers())
        {
            selTracerNames.insert(selectedTracerName.toUtf8().constData());
        }
        return RigFlowDiagResultAddress(m_resultVariable().toStdString(), RigFlowDiagResultAddress::PHASE_ALL, selTracerNames);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::setFlowDiagTracerSelectionType(FlowTracerSelectionType selectionType)
{   
    m_flowTracerSelectionMode = selectionType;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimEclipseResultDefinition::resultVariableUiName() const
{
    if (resultType() == RiaDefines::FLOW_DIAGNOSTICS)
    {
        QString fullName;

        if (m_flowTracerSelectionMode() == FLOW_TR_BY_SELECTION)
        {
            fullName = QString::fromStdString(flowDiagResAddress().uiText());
        }
        else
        {
            fullName = QString::fromStdString(flowDiagResAddress().uiShortText());
            fullName += QString(" (%1)").arg(m_flowTracerSelectionMode().uiText());
        }

        return fullName;
    }

    return m_resultVariable();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimEclipseResultDefinition::resultVariableUiShortName() const
{
    if (resultType() == RiaDefines::FLOW_DIAGNOSTICS)
    {
        QString shortName;

        if (m_flowTracerSelectionMode() == FLOW_TR_BY_SELECTION)
        {
            QString candidate = QString::fromStdString(flowDiagResAddress().uiText());

            int stringSizeLimit = 32;
            if (candidate.size() > stringSizeLimit)
            {
                candidate = candidate.left(stringSizeLimit);
                candidate += "...";
            }

            shortName = candidate;
        }
        else
        {
            shortName = QString::fromStdString(flowDiagResAddress().uiShortText());
            shortName += QString(" (%1)").arg(m_flowTracerSelectionMode().uiText());
        }

        return shortName;
    }

    return m_resultVariable();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::loadResult()
{
    if (isFlowDiagOrInjectionFlooding()) return; // Will load automatically on access

    RigCaseCellResultsData* gridCellResults = this->currentGridCellResults();
    if (gridCellResults)
    {
        gridCellResults->findOrLoadScalarResult(m_resultType(), m_resultVariable);
    }

}

//--------------------------------------------------------------------------------------------------
/// Returns whether the result requested by the definition is a single frame result 
/// The result needs to be loaded before asking
//--------------------------------------------------------------------------------------------------
bool RimEclipseResultDefinition::hasStaticResult() const
{
    if (isFlowDiagOrInjectionFlooding()) return false;

    const RigCaseCellResultsData* gridCellResults = this->currentGridCellResults();
    size_t gridScalarResultIndex = this->scalarResultIndex();

    if (hasResult() && gridCellResults->timeStepCount(gridScalarResultIndex) == 1 )
    {
        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/// Returns whether the result requested by the definition is loaded or possible to load from the result file
//--------------------------------------------------------------------------------------------------
bool RimEclipseResultDefinition::hasResult() const
{
    if (isFlowDiagOrInjectionFlooding())
    {
        if (m_flowSolution() && !m_resultVariable().isEmpty()) return true;
    }
    else if (this->currentGridCellResults() )
    {
        const RigCaseCellResultsData* gridCellResults = this->currentGridCellResults();
        size_t gridScalarResultIndex = gridCellResults->findScalarResultIndex(m_resultType(), m_resultVariable());
        return gridScalarResultIndex != cvf::UNDEFINED_SIZE_T;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/// Returns whether the result requested by the definition is a multi frame result 
/// The result needs to be loaded before asking
//--------------------------------------------------------------------------------------------------
bool RimEclipseResultDefinition::hasDynamicResult() const
{
    if (hasResult())
    {
        if (m_resultType() == RiaDefines::DYNAMIC_NATIVE)
        {
            return true;
        }
        else if (m_resultType() == RiaDefines::SOURSIMRL)
        {
            return true;
        }
        else if (m_resultType() == RiaDefines::FLOW_DIAGNOSTICS)
        {
            return true;
        }
        else if (m_resultType() == RiaDefines::INJECTION_FLOODING)
        {
            return true;
        }

        if (this->currentGridCellResults())
        {
            const RigCaseCellResultsData* gridCellResults = this->currentGridCellResults();
            size_t gridScalarResultIndex = this->scalarResultIndex();
            if (gridCellResults->timeStepCount(gridScalarResultIndex) > 1 )
            {
                return true;
            }
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::initAfterRead()
{
    m_porosityModelUiField = m_porosityModel;
    m_resultTypeUiField = m_resultType;
    m_resultVariableUiField = m_resultVariable;

    m_flowSolutionUiField = m_flowSolution();
    m_selectedTracersUiField = m_selectedTracers;

    this->updateUiIconFromToggleField();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::setResultType(RiaDefines::ResultCatType val)
{
    m_resultType = val;
    m_resultTypeUiField = val;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::setPorosityModel(RiaDefines::PorosityModelType val)
{
    m_porosityModel = val;
    m_porosityModelUiField = val;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::setResultVariable(const QString& val)
{
    m_resultVariable = val;
    m_resultVariableUiField = val;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimFlowDiagSolution* RimEclipseResultDefinition::flowDiagSolution()
{
    return m_flowSolution();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::setFlowSolution(RimFlowDiagSolution* flowSol)
{
    this->m_flowSolution = flowSol;
    this->m_flowSolutionUiField = flowSol;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::setSelectedTracers(const std::vector<QString>& selectedTracers)
{
    this->m_selectedTracers = selectedTracers;
    this->m_selectedTracersUiField = selectedTracers;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::setSelectedSouringTracers(const std::vector<QString>& selectedTracers)
{
    this->m_selectedSouringTracers = selectedTracers;
    this->m_selectedSouringTracersUiField = selectedTracers;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::updateUiFieldsFromActiveResult()
{
    m_resultTypeUiField = m_resultType;
    m_resultVariableUiField = resultVariable();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimEclipseResultDefinition::isTernarySaturationSelected() const
{
    bool isTernary =    (m_resultType() == RiaDefines::DYNAMIC_NATIVE) && 
                        (m_resultVariable().compare(RiaDefines::ternarySaturationResultName(), Qt::CaseInsensitive) == 0);

    return isTernary;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimEclipseResultDefinition::isCompletionTypeSelected() const
{
    return (m_resultType() == RiaDefines::DYNAMIC_NATIVE && m_resultVariable() == RiaDefines::completionTypeResultName());
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimEclipseResultDefinition::hasCategoryResult() const
{
    if (this->m_resultType() == RiaDefines::FORMATION_NAMES
        && m_eclipseCase 
        && m_eclipseCase->eclipseCaseData() 
        && m_eclipseCase->eclipseCaseData()->activeFormationNames() ) return true;

    if (this->m_resultType() == RiaDefines::DYNAMIC_NATIVE
        && this->resultVariable() == RiaDefines::completionTypeResultName()) return true;

    if (this->m_resultType() == RiaDefines::FLOW_DIAGNOSTICS
        && m_resultVariable() == RIG_FLD_MAX_FRACTION_TRACER_RESNAME) return true;

    if (!this->hasStaticResult()) return false;

    return this->resultVariable().contains("NUM", Qt::CaseInsensitive);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimEclipseResultDefinition::isFlowDiagOrInjectionFlooding() const
{
    if (this->m_resultType() == RiaDefines::FLOW_DIAGNOSTICS || this->m_resultType() == RiaDefines::INJECTION_FLOODING)
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimEclipseResultDefinition::hasDualPorFractureResult()
{
    if (m_eclipseCase
        && m_eclipseCase->eclipseCaseData())
    {
        return m_eclipseCase->eclipseCaseData()->hasFractureResults();
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    uiOrdering.add(&m_resultTypeUiField);

    if (hasDualPorFractureResult()) 
    {
        uiOrdering.add(&m_porosityModelUiField);
    }

    if ( m_resultTypeUiField() == RiaDefines::FLOW_DIAGNOSTICS )
    { 
        uiOrdering.add(&m_flowSolutionUiField);

        uiOrdering.add(&m_flowTracerSelectionMode);
        
        if (m_flowTracerSelectionMode == FLOW_TR_BY_SELECTION)
        {
            uiOrdering.add(&m_selectedTracersUiFieldFilter);
            uiOrdering.add(&m_selectedTracersUiField);
        }

        uiOrdering.add(&m_phaseSelection);

        if ( m_flowSolution() == nullptr )
        {
            assignFlowSolutionFromCase();
        }
    }

    if (m_resultTypeUiField() == RiaDefines::INJECTION_FLOODING)
    {
        uiOrdering.add(&m_selectedSouringTracersUiField);
    }

    uiOrdering.add(&m_resultVariableUiField);

    uiOrdering.skipRemainingFields(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::defineEditorAttribute(const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute)
{
    if (m_resultTypeUiField() == RiaDefines::FLOW_DIAGNOSTICS
        && field == &m_resultVariableUiField)
    {
        caf::PdmUiListEditorAttribute* listEditAttr = dynamic_cast<caf::PdmUiListEditorAttribute*>(attribute);
        if (listEditAttr)
        {
            listEditAttr->m_heightHint = 50;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimEclipseResultDefinition::removePerCellFaceOptionItems(QList<caf::PdmOptionItemInfo>& optionItems)
{
    std::vector<int> indicesToRemove;
    for (int i = 0; i < optionItems.size(); i++)
    {
        QString text = optionItems[i].optionUiText();

        if (RiaDefines::isPerCellFaceResult(text))
        {
            indicesToRemove.push_back(i);
        }
    }

    std::sort(indicesToRemove.begin(), indicesToRemove.end());

    std::vector<int>::reverse_iterator rit;
    for (rit = indicesToRemove.rbegin(); rit != indicesToRemove.rend(); ++rit)
    {
        optionItems.takeAt(*rit);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimEclipseResultDefinition::tracerNamesMatchingFilter() const
{
    std::vector<QString> matchingNames;

    RimFlowDiagSolution* flowSol = m_flowSolutionUiField();
    if (flowSol)
    {
        std::vector<QString> tracerNames = flowSol->tracerNames();
        if (m_selectedTracersUiFieldFilter().isEmpty())
        {
            matchingNames = tracerNames;
        }
        else
        {
            for (const QString& tracerName : tracerNames)
            {
                if (caf::Utils::isStringMatch(m_selectedTracersUiFieldFilter, tracerName))
                {
                    matchingNames.push_back(tracerName);
                }
            }
        }
    }

    return matchingNames;
}

