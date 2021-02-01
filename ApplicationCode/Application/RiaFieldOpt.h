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

#pragma once

#include "RiaApplication.h"

#include "cafAppEnum.h"
#include "cafPdmChildField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

// Include to make Pdm work for cvf::Color
#include "cafPdmFieldCvfColor.h"

class RifReaderSettings;

// =========================================================
class RiaFieldOpt : public caf::PdmObject
{
 CAF_PDM_HEADER_INIT;

 public:
  enum SummaryRestartFilesImportMode {
    ASK_USER, IMPORT, NOT_IMPORT, SEPARATE_CASES
  };

  typedef caf::AppEnum<SummaryRestartFilesImportMode>
    SummaryRestartFilesImportModeType;

  RiaFieldOpt(void);
  virtual ~RiaFieldOpt(void);

  QStringList tabNames();

  const RifReaderSettings* readerSettings() const;

 public: // Pdm Fields
  caf::PdmField<caf::AppEnum< RiaApplication::RINavigationPolicy > > navigationPolicy;

  caf::PdmField<QString> scriptDirectories;
  caf::PdmField<QString> scriptEditorExecutable;

  caf::PdmField<QString> octaveExecutable;
  caf::PdmField<bool> octaveShowHeaderInfoWhenExecutingScripts;

  caf::PdmField<QString> ssihubAddress;

  caf::PdmField<int> defaultScaleFactorZ;
  caf::PdmField<bool> defaultGridLines;
  caf::PdmField<cvf::Color3f> defaultGridLineColors;
  caf::PdmField<cvf::Color3f> defaultFaultGridLineColors;
  caf::PdmField<cvf::Color3f> defaultViewerBackgroundColor;
  caf::PdmField<cvf::Color3f> defaultWellLabelColor;

  caf::PdmField<bool> showLasCurveWithoutTvdWarning;
  caf::PdmField<QString> fontSizeInScene;
  caf::PdmField<bool> showLegendBackground;

  caf::PdmField<bool> useShaders;
  caf::PdmField<bool> showHud;
  caf::PdmField<bool> appendClassNameToUiText;
  caf::PdmField<bool> appendFieldKeywordToToolTipText;
  caf::PdmField<bool> showTestToolbar;
  caf::PdmField<bool> includeFractureDebugInfoFile;

  caf::PdmField<QString> lastUsedProjectFileName;

  caf::PdmField<bool> autocomputeDepthRelatedProperties;
  caf::PdmField<bool> loadAndShowSoil;

  caf::PdmField<SummaryRestartFilesImportModeType>
    summaryRestartFilesImportMode;


  // -------------------------------------------------------
  // FORe-Open JSON fields :: "OPTIMIZER"
  caf::PdmField<QString> globalName;
  caf::PdmField<caf::AppEnum< RiaApplication::FOReOptMode > > optMode;

//  caf::PdmField<caf::AppEnum< RiaApplication::FOReOptTypeAll > > optTypeAll;
//  caf::PdmField<caf::AppEnum< RiaApplication::FOReOptTypeDet > > optTypeDet;
//  caf::PdmField<caf::AppEnum< RiaApplication::FOReOptTypeSto > > optTypeSto;
//  caf::PdmField<caf::AppEnum< RiaApplication::FOReOptTypeHyb > > optTypeHyb;

  caf::PdmField<std::vector<QString> > optTypeAllText;
  caf::PdmField<std::vector<QString> > optTypeHybridsText;
  caf::PdmField<bool> optScaleVars;

  caf::PdmField<int> verbose;
  caf::PdmField<int> maxEvals;


  // -------------------------------------------------------
  // FORe-Open JSON fields :: "MODEL/PROBLEM"
//  caf::PdmField<caf::AppEnum< RiaApplication::FORePrbStrc > > prbStrc;

  caf::PdmField<std::vector<QString> > probTypes;
  caf::PdmField<std::vector<QString> > probStructure;
  caf::PdmField<std::vector<QString>> prbAutoVarSegr;

  caf::PdmField<caf::AppEnum< RiaApplication::FOReObjType > > objType;

  caf::PdmField<std::vector<QString> > augTerms;


  // -------------------------------------------------------
  // FORe-Open JSON fields :: "CONSTRAINTS"

  // -------------------------------------------------------
  // FORe-Open JSON fields :: "RUNNER"


 protected:
  virtual void defineEditorAttribute(
    const caf::PdmFieldHandle* field,
    QString uiConfigName,
    caf::PdmUiEditorAttribute* attribute);

  virtual void defineUiOrdering(
    QString uiConfigName,
    caf::PdmUiOrdering& uiOrdering);

  virtual QList<caf::PdmOptionItemInfo> calculateValueOptions(
    const caf::PdmFieldHandle* fieldNeedingOptions,
    bool * useOptionsOnly);

 private:
  caf::PdmChildField<RifReaderSettings*> m_readerSettings;

  QStringList m_tabNames;
};
