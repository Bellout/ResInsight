////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-     Statoil ASA
//  Copyright (C) 2013-     Ceetron Solutions AS
//  Copyright (C) 2011-2012 Ceetron AS
//
//  ResInsight is free software: you can redistribute it
//  and/or modify it under the terms of the GNU General
//  Public License as published by the Free Software
//  Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will
//  be useful, but WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at
//  <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
////////////////////////////////////////////////////////////

#include "RiaFieldOpt.h"

#include "RifReaderSettings.h"

#include "cafPdmFieldCvfColor.h"
#include "cafPdmUiCheckBoxEditor.h"
#include "cafPdmUiFieldHandle.h"
#include "cafPdmUiFilePathEditor.h"

// =========================================================
namespace caf
{
template<>
void RiaFieldOpt::SummaryRestartFilesImportModeType::setUp() {
  addItem(RiaFieldOpt::ASK_USER, "ASK_USER", "Ask user");
  addItem(RiaFieldOpt::IMPORT, "IMPORT", "Always import");
  addItem(RiaFieldOpt::NOT_IMPORT, "NOT_IMPORT", "Never import");
  addItem(RiaFieldOpt::SEPARATE_CASES, "SEPARATE_CASES", "Import as separate summary cases");
  setDefault(RiaFieldOpt::ASK_USER);
}
}

CAF_PDM_SOURCE_INIT(RiaFieldOpt, "RiaFieldOpt");

// =========================================================
RiaFieldOpt::RiaFieldOpt(void) {

  CAF_PDM_InitField(
    &navigationPolicy, "navigationPolicy",
    caf::AppEnum<RiaApplication::RINavigationPolicy>(
      RiaApplication::NAVIGATION_POLICY_RMS),
    "Navigation Mode", "", "", "");

  CAF_PDM_InitFieldNoDefault(
    &scriptDirectories, "scriptDirectory",
    "Shared Script Folder(s)", "", "", "");

  // -------------------------------------------------------
  scriptDirectories.uiCapability()->setUiEditorTypeName(
    caf::PdmUiFilePathEditor::uiEditorTypeName());

  CAF_PDM_InitField(
    &scriptEditorExecutable, "scriptEditorExecutable",
    QString("kate"), "Script Editor", "", "", "");

  scriptEditorExecutable.uiCapability()->setUiEditorTypeName(
    caf::PdmUiFilePathEditor::uiEditorTypeName());

  // -------------------------------------------------------
  CAF_PDM_InitField(
    &octaveExecutable, "octaveExecutable",
    QString("octave"), "Octave Executable Location", "", "", "");

  octaveExecutable.uiCapability()->setUiEditorTypeName(
    caf::PdmUiFilePathEditor::uiEditorTypeName());

  octaveExecutable.uiCapability()->setUiLabelPosition(
    caf::PdmUiItemInfo::TOP);

  CAF_PDM_InitField(
    &octaveShowHeaderInfoWhenExecutingScripts,
    "octaveShowHeaderInfoWhenExecutingScripts", false,
    "Show Text Header When Executing Scripts", "", "", "");

  octaveShowHeaderInfoWhenExecutingScripts
    .uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);







  // -------------------------------------------------------
  // FORe-Open JSON fields :: "OPTIMIZER"

  CAF_PDM_InitField(
    &globalName, "globalName",
    QString("olympr37-msw-opt-aug-msw-scl"), "Name",
    "", "", "");

  CAF_PDM_InitField(&verbose, "VerboseConf",
                    5,"Verbose Configuration",
                    "", "", "");

  CAF_PDM_InitField(
    &optMode, "optMode",
    caf::AppEnum<RiaApplication::FOReOptMode>(
      RiaApplication::FORE_OPTMODE_MAXIMIZE),
    "Optimization Mode", "", "", "");

//  CAF_PDM_InitField(
//    &optTypeAll, "optTypeAll",
//    caf::AppEnum<RiaApplication::FOReOptTypeAll>(RiaApplication::FORE_OPTTYPE_ALL_APPS),
//    "Optimizer Type", "", "", "");

  std::vector<QString> optTypeAllTextData(0);
  optTypeAllTextData.push_back("CS -- Compass Search (CS)");
  optTypeAllTextData.push_back("APPS -- Asynchronous Paralell Pattern Search");
  optTypeAllTextData.push_back("DFTR -- Derivative-Free Trust-Region Algorithm");
  optTypeAllTextData.push_back("GA -- Genetic Algorithm");
  optTypeAllTextData.push_back("PSO -- Particle Swarm Optimization");
  optTypeAllTextData.push_back("CMA-ES -- Covariance Matrix Adaption Evolutionary Strategy");
  optTypeAllTextData.push_back("EGO -- Bayesian Optimization");
  optTypeAllTextData.push_back("SPSA -- Simultaneous Perturbation Stochastic Approximation");
////  optTypeAllTextData.push_back("mPSO");
////  optTypeAllTextData.push_back("APPS/PSO + data-driven meta-optimization");
////  optTypeAllTextData.push_back("Joint optimization using embedded reduced-order sub-routines");

  CAF_PDM_InitField(&optTypeAllText, "optTypeAll",
                    optTypeAllTextData,"Algorithms",
                    "", "", "");

  std::vector<QString> optTypeHybridsData(0);
  optTypeHybridsData.push_back("PSO + APPS");
  optTypeHybridsData.push_back("APPS + ANN-ConfA");
  optTypeHybridsData.push_back("APPS + ANN-ConfB");
  optTypeHybridsData.push_back("DFTR + ANN");

  CAF_PDM_InitField(&optTypeHybridsText, "optTypeHybrids",
                    optTypeHybridsData,"Hybrid Procedures",
                    "", "", "");

  CAF_PDM_InitField(&optScaleVars,"ScaleVars",
                    true,"Scale Variables",
                    "", "", "");

  CAF_PDM_InitField(&maxEvals, "OptMaxEvals",
                    1000,"Max Evaluations",
                    "", "", "");

  // -------------------------------------------------------
  // FORe-Open JSON fields :: "MODEL/PROBLEM"

//  CAF_PDM_InitField(
//    &prbStrc, "prbStrc",
//    caf::AppEnum<RiaApplication::FORePrbStrc>(
//      RiaApplication::FORE_PRBSTRC_EMBEDDED),"Solution Approach","", "", "");

  std::vector<QString> probTypesData(0);
  probTypesData.push_back("Well Placement, Well Controls");
  probTypesData.push_back("Inflow Control Valves, Well Controls");
  probTypesData.push_back("Well Placement, Well Controls, Inflow Control Valves");

  CAF_PDM_InitField(&probTypes, "probTypes",
                    probTypesData,"Problem Types",
                    "", "", "");

  std::vector<QString> prbStrcAllTextData(0);
  prbStrcAllTextData.push_back("Concurrent");
  prbStrcAllTextData.push_back("Embedded");
  prbStrcAllTextData.push_back("Sequential");

  CAF_PDM_InitField(&probStructure, "probStructure",
                    prbStrcAllTextData,"Solution Approach",
                    "", "", "");

  std::vector<QString> prbAutoVarSegrData(0);
  prbAutoVarSegrData.push_back("None");
  prbAutoVarSegrData.push_back("Manual");
  prbAutoVarSegrData.push_back("Automatic");

  CAF_PDM_InitField(&prbAutoVarSegr,
                    "prbAutoVarSegr",
                    prbAutoVarSegrData,"Variable Segregation",
                    "", "", "");

  CAF_PDM_InitField(
    &objType, "objType",
    caf::AppEnum<RiaApplication::FOReObjType>(
      RiaApplication::FORE_OBJTYPE_NPV),
    "Objective Function", "", "", "");

  std::vector<QString> augTermsData(0);
  augTermsData.push_back("Segment WCT [NPV scaled]");
  augTermsData.push_back("Segment WCT [Well production scaled]");
  augTermsData.push_back("Well Water Breakthrough Time");

  CAF_PDM_InitField(&augTerms, "augTermsData",
                    augTermsData,"Augmented Terms",
                    "", "", "");

  // -------------------------------------------------------
  // FORe-Open JSON fields :: "CONSTRAINTS"



  // -------------------------------------------------------
  // FORe-Open JSON fields :: "RUNNER"




  // -------------------------------------------------------
  CAF_PDM_InitField(
    &ssihubAddress, "ssihubAddress",
    QString("http://"), "SSIHUB Address",
    "", "", "");

  ssihubAddress.uiCapability()->setUiLabelPosition(
    caf::PdmUiItemInfo::TOP);

  CAF_PDM_InitField(
    &defaultGridLines, "defaultGridLines", true,
    "Gridlines", "", "", "");

  // -------------------------------------------------------
  CAF_PDM_InitField(
    &defaultGridLineColors,
    "defaultGridLineColors",
    cvf::Color3f(0.92f, 0.92f, 0.92f),
    "Mesh Color", "", "", "");

  CAF_PDM_InitField(
    &defaultFaultGridLineColors,
    "defaultFaultGridLineColors",
    cvf::Color3f(0.08f, 0.08f, 0.08f),
    "Mesh Color Along Faults", "", "", "");

  CAF_PDM_InitField(
    &defaultWellLabelColor,
    "defaultWellLableColor",
    cvf::Color3f(0.92f, 0.92f, 0.92f),
    "Well Label Color", "",
    "The default well label color in new views", "");

  CAF_PDM_InitField(
    &defaultViewerBackgroundColor,
    "defaultViewerBackgroundColor",
    cvf::Color3f(0.69f, 0.77f, 0.87f),
    "Viewer Background", "",
    "The viewer background color for new views", "");

  // -------------------------------------------------------
  CAF_PDM_InitField(
    &defaultScaleFactorZ, "defaultScaleFactorZ",
    5, "Default Z Scale Factor", "", "", "");

  CAF_PDM_InitField(
    &fontSizeInScene, "fontSizeInScene",
    QString("8"), "Font Size", "", "", "");

  CAF_PDM_InitField(
    &showLasCurveWithoutTvdWarning,
    "showLasCurveWithoutTvdWarning", true,
    "Show LAS Curve Without TVD Warning", "", "", "");

  // -------------------------------------------------------
  showLasCurveWithoutTvdWarning
    .uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);

  CAF_PDM_InitField(
    &useShaders, "useShaders", true,
    "Use Shaders", "", "", "");

  useShaders.uiCapability()->setUiLabelPosition(
    caf::PdmUiItemInfo::HIDDEN);

  CAF_PDM_InitField(
    &showHud, "showHud", false,
    "Show 3D Information", "", "", "");

  showHud.uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);

  // -------------------------------------------------------
  CAF_PDM_InitField(
    &appendClassNameToUiText, "appendClassNameToUiText", false,
    "Show Class Names", "", "", "");

  appendClassNameToUiText
    .uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);

  CAF_PDM_InitField(
    &appendFieldKeywordToToolTipText,
    "appendFieldKeywordToToolTipText", false,
    "Show Field Keyword in ToolTip", "", "", "");

  appendFieldKeywordToToolTipText
    .uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);

  // -------------------------------------------------------
  CAF_PDM_InitField(
    &showTestToolbar, "showTestToolbar", false,
    "Enable Test Toolbar", "", "", "");

  showTestToolbar
    .uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);

  CAF_PDM_InitField(
    &includeFractureDebugInfoFile,
    "includeFractureDebugInfoFile", false,
    "Include Fracture Debug Info for Completion Export", "", "", "");

  includeFractureDebugInfoFile.uiCapability()->setUiLabelPosition(
    caf::PdmUiItemInfo::HIDDEN);

  CAF_PDM_InitField(&showLegendBackground,
                    "showLegendBackground",
                    true,"Enable Legend Background",
                    "", "", "");

  // -------------------------------------------------------
  CAF_PDM_InitFieldNoDefault(&lastUsedProjectFileName,
                             "lastUsedProjectFileName", "Last Used Project File",
                             "", "", "");

  lastUsedProjectFileName.uiCapability()->setUiHidden(true);

  CAF_PDM_InitField(
    &autocomputeDepthRelatedProperties,
    "autocomputeDepth", true,
    "Compute DEPTH Related Properties", "",
    "DEPTH, DX, DY, DZ, TOP, BOTTOM", "");

  autocomputeDepthRelatedProperties
    .uiCapability()->setUiLabelPosition(
    caf::PdmUiItemInfo::HIDDEN);

  CAF_PDM_InitField(
    &loadAndShowSoil, "loadAndShowSoil", true,
    "Load and Show SOIL", "", "", "");

  // -------------------------------------------------------
  loadAndShowSoil
    .uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);

  CAF_PDM_InitFieldNoDefault(&summaryRestartFilesImportMode,
                             "summaryRestartFilesImportMode",
                             "Import summary restart files", "", "", "");

  //loadAndShowSoil.uiCapability()->setUiLabelPosition(caf::PdmUiItemInfo::HIDDEN);

  CAF_PDM_InitFieldNoDefault(&m_readerSettings,
                             "readerSettings", "Reader Settings", "", "", "");

  m_readerSettings = new RifReaderSettings;

  m_tabNames << "Optimizer";
  m_tabNames << "Model/Problem";
  m_tabNames << "Constraints";
  m_tabNames << "Runner";

//  m_tabNames << "GENERAL";
//  m_tabNames << "ECLIPSE";
//  m_tabNames << "OCTAVE";
//  m_tabNames << "SYSTEM";


}

// =========================================================
RiaFieldOpt::~RiaFieldOpt(void) {
  delete m_readerSettings;
}

// =========================================================
void RiaFieldOpt::defineEditorAttribute(
  const caf::PdmFieldHandle* field,
  QString uiConfigName,
  caf::PdmUiEditorAttribute * attribute) {

  m_readerSettings->defineEditorAttribute(field, uiConfigName, attribute);

  // -------------------------------------------------------
  if (field == &scriptDirectories) {

    caf::PdmUiFilePathEditorAttribute* myAttr =
      dynamic_cast<caf::PdmUiFilePathEditorAttribute*>(attribute);

    if (myAttr) {
      myAttr->m_selectDirectory = true;
      myAttr->m_appendUiSelectedFolderToText = true;
    }

  } else if (field == &octaveShowHeaderInfoWhenExecutingScripts ||
    field == &autocomputeDepthRelatedProperties ||
    field == &loadAndShowSoil ||
    field == &useShaders ||
    field == &showHud ||
    field == &appendClassNameToUiText ||
    field == &appendFieldKeywordToToolTipText ||
    field == &showTestToolbar ||
    field == &includeFractureDebugInfoFile ||
    field == &showLasCurveWithoutTvdWarning) {

    caf::PdmUiCheckBoxEditorAttribute* myAttr =
      dynamic_cast<caf::PdmUiCheckBoxEditorAttribute*>(attribute);
    if (myAttr) {
      myAttr->m_useNativeCheckBoxLabel = true;
    }
  }
}

// =========================================================
void RiaFieldOpt::defineUiOrdering(QString uiConfigName,
                                   caf::PdmUiOrdering& uiOrdering) {

  std::cout << "uiConfigName: " << uiConfigName.toStdString() << std::endl;

  // -------------------------------------------------------
  // "OPTIMIZER"
  if (uiConfigName == m_tabNames[0]) {
    caf::PdmUiGroup* globalGrp = uiOrdering.addNewGroup("Global");
    globalGrp->add(&globalName);
    globalGrp->add(&verbose);

    caf::PdmUiGroup* optGrp = uiOrdering.addNewGroup("Optimizer");
    optGrp->add(&optMode);
    // optGrp->add(&optTypeAll);

    optGrp->add(&optTypeAllText);
    optGrp->add(&optTypeHybridsText);
    optGrp->add(&optScaleVars);
    optGrp->add(&maxEvals);


    // -----------------------------------------------------
    // MODEL/PROBLEM
    caf::PdmUiGroup* probGrp = uiOrdering.addNewGroup("Problem Structure");
    probGrp->add(&probTypes);
    probGrp->add(&probStructure);
    probGrp->add(&prbAutoVarSegr);

    caf::PdmUiGroup* objGrp = uiOrdering.addNewGroup("Objective");
    objGrp->add(&objType);
    objGrp->add(&augTerms);

  } else if (uiConfigName == m_tabNames[1]) {

    // -----------------------------------------------------
    // CONSTRAINTS
  } else if (uiConfigName == m_tabNames[2]) {
//    caf::PdmUiGroup* constGrp = uiOrdering.addNewGroup("Configuration");
//    constGrp->add(&probStructure);


    // -----------------------------------------------------
    // CONSTRAINTS
  } else if (uiConfigName == m_tabNames[3]) {
//    caf::PdmUiGroup* rnnrGrp = uiOrdering.addNewGroup("Runner Type");
//    rnnrGrp->add(&probStructure);





    // GENERAL
  } else if (uiConfigName == m_tabNames[4]) {

    caf::PdmUiGroup* defaultSettingsGroup = uiOrdering.addNewGroup("Default Settings");
    defaultSettingsGroup->add(&defaultViewerBackgroundColor);
    defaultSettingsGroup->add(&defaultGridLines);
    defaultSettingsGroup->add(&defaultGridLineColors);
    defaultSettingsGroup->add(&defaultFaultGridLineColors);
    defaultSettingsGroup->add(&defaultWellLabelColor);
    defaultSettingsGroup->add(&fontSizeInScene);
    defaultSettingsGroup->add(&defaultScaleFactorZ);
    defaultSettingsGroup->add(&showLegendBackground);

    caf::PdmUiGroup* viewsGroup = uiOrdering.addNewGroup("3D Views");
    viewsGroup->add(&navigationPolicy);
    viewsGroup->add(&useShaders);
    viewsGroup->add(&showHud);


    caf::PdmUiGroup* otherGroup = uiOrdering.addNewGroup("Other");
    otherGroup->add(&ssihubAddress);
    otherGroup->add(&showLasCurveWithoutTvdWarning);

    // ECLIPSE
  } else if (uiConfigName == m_tabNames[5]) {
    caf::PdmUiGroup* newCaseBehaviourGroup = uiOrdering.addNewGroup("Behavior When Loading Data");
    newCaseBehaviourGroup->add(&autocomputeDepthRelatedProperties);
    newCaseBehaviourGroup->add(&loadAndShowSoil);

//    m_readerSettings2->defineUiOrdering(uiConfigName, *newCaseBehaviourGroup);

    caf::PdmUiGroup* restartBehaviourGroup = uiOrdering.addNewGroup("Summary Restart Files");
    restartBehaviourGroup->add(&summaryRestartFilesImportMode);

    // OCTAVE
  } else if (uiConfigName == m_tabNames[6]) {

    caf::PdmUiGroup* octaveGroup = uiOrdering.addNewGroup("Octave");
    octaveGroup->add(&octaveExecutable);
    octaveGroup->add(&octaveShowHeaderInfoWhenExecutingScripts);

    caf::PdmUiGroup* scriptGroup = uiOrdering.addNewGroup("Script files");
    scriptGroup->add(&scriptDirectories);
    scriptGroup->add(&scriptEditorExecutable);

    // SYSTEM
  } else if (uiConfigName == m_tabNames[7]) {

    uiOrdering.add(&appendClassNameToUiText);
    uiOrdering.add(&appendFieldKeywordToToolTipText);
    uiOrdering.add(&showTestToolbar);
    uiOrdering.add(&includeFractureDebugInfoFile);
  }

  uiOrdering.skipRemainingFields(true);
}

// =========================================================
QList<caf::PdmOptionItemInfo> RiaFieldOpt::calculateValueOptions(
  const caf::PdmFieldHandle* fieldNeedingOptions,
  bool * useOptionsOnly) {

  QList<caf::PdmOptionItemInfo> options;
  *useOptionsOnly = true;

  if (&fontSizeInScene == fieldNeedingOptions) {

    QStringList fontSizes;
    fontSizes <<  "8";
    fontSizes << "12";
    fontSizes << "16";
    fontSizes << "24";
    fontSizes << "32";

    for (int oIdx = 0; oIdx < fontSizes.size(); ++oIdx) {
      options.push_back(
        caf::PdmOptionItemInfo(
          fontSizes[oIdx], fontSizes[oIdx]));
    }
  }

  return options;
}

// =========================================================
QStringList RiaFieldOpt::tabNames() {
  return m_tabNames;
}

// =========================================================
const RifReaderSettings* RiaFieldOpt::readerSettings() const {
  return m_readerSettings;
}

