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

#include <QApplication>
#include <QProcess>
#include <QMutex>

#include "cafPdmObject.h"
#include "cafPdmField.h"
#include "cvfBase.h"
#include "cvfObject.h"
#include "cvfFont.h"

#include <iostream>
#include <memory>

class QAction;

class Drawable;

class RIProcess;

class RiaPreferences;
class RiaFieldOpt;

class RiaProjectModifier;
class RiaSocketServer;

class RigEclipseCaseData;

class RimCommandObject;
class RimEclipseCase;
class RimEclipseView;
class RimGridView;
class RimProject;
class RimSummaryPlot;
class Rim3dView;
class RimViewWindow;
class RimWellLogPlot;
class RimWellAllocationPlot;

class RiuMainWindowBase;
class RiuMainPlotWindow;
class RiuRecentFileActionProvider;
class RiaArgumentParser;

namespace caf {
class UiProcess;
}

//==========================================================
class RiaApplication : public QApplication {
 Q_OBJECT

 public:
  enum RINavigationPolicy {
    NAVIGATION_POLICY_CEETRON,
    NAVIGATION_POLICY_CAD,
    NAVIGATION_POLICY_GEOQUEST,
    NAVIGATION_POLICY_RMS
  };

  enum ProjectLoadAction {
    PLA_NONE = 0,
    PLA_CALCULATE_STATISTICS = 1
  };

  enum FOReOptTypeAll {
    FORE_OPTTYPE_ALL_CS,
    FORE_OPTTYPE_ALL_APPS,
    FORE_OPTTYPE_ALL_DFTR,
    FORE_OPTTYPE_ALL_GA,
    FORE_OPTTYPE_ALL_PSO,
    FORE_OPTTYPE_ALL_CMA_ES,
    FORE_OPTTYPE_ALL_EGO,
    FORE_OPTTYPE_ALL_SPSA,
    FORE_OPTTYPE_ALL_MPSO,
    FORE_OPTTYPE_ALL_APPS_PSO_ML1,
    FORE_OPTTYPE_ALL_JNT_ROSP_CNTRL
  };

  enum FOReOptTypeDet {
    FORE_OPTTYPE_DET_CS,
    FORE_OPTTYPE_DET_APPS,
    FORE_OPTTYPE_DET_DFTR
  };

  enum FOReOptTypeSto {
    FORE_OPTTYPE_STO_GA,
    FORE_OPTTYPE_STO_PSO,
    FORE_OPTTYPE_STO_CMA_ES,
    FORE_OPTTYPE_STO_EGO,
    FORE_OPTTYPE_STO_SPSA
  };

  enum FOReOptTypeHyb {
    FORE_OPTTYPE_HYB_MPSO,
    FORE_OPTTYPE_HYB_APPS_PSO_ML1,
    FORE_OPTTYPE_HYB_JNT_ROSP_CNTRL
  };

  enum FOReOptMode {
    FORE_OPTMODE_MAXIMIZE,
    FORE_OPTMODE_MINIMIZE
  };

  enum FORePrbStrc {
    FORE_PRBSTRC_CONCURRENT,
    FORE_PRBSTRC_SEQUENTIAL,
    FORE_PRBSTRC_EMBEDDED
  };

  enum FOReObjType {
    FORE_OBJTYPE_NPV,
    FORE_OBJTYPE_WEIGHTED
  };

 public:
  // -------------------------------------------------------
  RiaApplication(int& argc, char** argv);
  ~RiaApplication();

  static RiaApplication* instance();

  int parseArgumentsAndRunUnitTestsIfRequested();
  bool parseArguments();

  void setActiveReservoirView(Rim3dView*);
  Rim3dView* activeReservoirView();
  const Rim3dView* activeReservoirView() const;
  RimGridView* activeGridView();

  RimViewWindow* activePlotWindow() const;

  RimProject* project();

  // -------------------------------------------------------
  void createMockModel();
  void createResultsMockModel();
  void createLargeResultsMockModel();
  void createMockModelCustomized();
  void createInputMockModel();

  // -------------------------------------------------------
  QString lastUsedDialogDirectory(const QString& dialogName);
  QString lastUsedDialogDirectoryWithFallback(const QString& dialogName,
                                              const QString& fallbackDirectory);
  void setLastUsedDialogDirectory(const QString& dialogName,
                                  const QString& directory);

  bool openFile(const QString& fileName);

  bool openOdbCaseFromFile(const QString& fileName);

  // -------------------------------------------------------
  QString currentProjectPath() const;
  QString createAbsolutePathFromProjectRelativePath(QString projectRelativePath);
  bool loadProject(const QString& projectFileName);

  bool loadProject(const QString& projectFileName,
                   ProjectLoadAction loadAction,
                   RiaProjectModifier* projectModifier);

  bool saveProject();
  bool saveProjectAs(const QString& fileName);
  bool saveProjectPromptForFileName();
  static bool hasValidProjectFileExtension(const QString& fileName);

  // -------------------------------------------------------
  bool askUserToSaveModifiedProject();
  void closeProject();

  void addWellPathsToModel(QList<QString> wellPathFilePaths);
  void addWellPathFormationsToModel(QList<QString> wellPathFilePaths);
  void addWellLogsToModel(const QList<QString>& wellLogFilePaths);

  void runMultiCaseSnapshots(const QString& templateProjectFileName, std::vector<QString> gridFileNames, const QString& snapshotFolderName);

  void processNonGuiEvents();

  // -------------------------------------------------------
  static const char*  getVersionStringApp(bool includeCrtInfo);

  void setUseShaders(bool enable);
  bool useShaders() const;

  void setShowPerformanceInfo(bool enable);
  bool showPerformanceInfo() const;

  RINavigationPolicy navigationPolicy() const;
  QString scriptDirectories() const;
  QString scriptEditorPath() const;

  QString octavePath() const;
  QStringList octaveArguments() const;

  // -------------------------------------------------------
  bool launchProcess(const QString& program,
                     const QStringList& arguments);

  bool launchProcessForMultipleCases(const QString& program,
                                     const QStringList& arguments,
                                     const std::vector<int>& caseIds);
  void terminateProcess();
  void waitForProcess() const;





  RiaPreferences* preferences();
  void applyPreferences();

  RiaFieldOpt* prefs_fieldopt();
  void applyPrefsFieldOpt();








  cvf::Font* standardFont();
  cvf::Font* customFont();

  // -------------------------------------------------------
  QString commandLineParameterHelp() const;
  void showFormattedTextInMessageBox(const QString& text);

  void setCacheDataObject(const QString& key,
                          const QVariant& dataObject);
  QVariant cacheDataObject(const QString& key) const;

  void addCommandObject(RimCommandObject* commandObject);
  void executeCommandObjects();

  int launchUnitTests();
  int launchUnitTestsWithConsole();

  // -------------------------------------------------------
  RiuMainPlotWindow* getOrCreateAndShowMainPlotWindow();
  RiuMainPlotWindow* mainPlotWindow();
  RiuMainWindowBase* mainWindowByID(int mainWindowID);

  static RimViewWindow* activeViewWindow();

  bool isMain3dWindowVisible() const;
  bool isMainPlotWindowVisible() const;

  bool tryCloseMainWindow();
  bool tryClosePlotWindow();

  void addToRecentFiles(const QString& fileName);
  std::vector<QAction*> recentFileActions() const;

  void setStartDir(const QString& startDir);

  // -------------------------------------------------------
  static std::vector<QString> readFileListFromTextFile(QString listFileName);

  void waitUntilCommandObjectsHasBeenProcessed();


 private:
  // -------------------------------------------------------
  void onProjectOpenedOrClosed();
  void setWindowCaptionFromAppState();

  void createMainPlotWindow();
  void deleteMainPlotWindow();

  void loadAndUpdatePlotData();

  void storeTreeViewState();

  // -------------------------------------------------------
  friend RiaArgumentParser;
  void setHelpText(const QString& helpText);

 private slots:
  void slotWorkerProcessFinished(int exitCode,
                                 QProcess::ExitStatus exitStatus);

 private:
  // -------------------------------------------------------
  caf::PdmPointer<Rim3dView> m_activeReservoirView;
  caf::PdmPointer<RimProject> m_project;

  RiaSocketServer* m_socketServer;

  caf::UiProcess* m_workerProcess;

  // -------------------------------------------------------
  // Execute for all settings
  std::list<int> m_currentCaseIds;
  QString m_currentProgram;
  QStringList m_currentArguments;

  RiaPreferences* m_preferences;
  RiaFieldOpt* m_fieldopt;

  std::map<QString, QString> m_fileDialogDefaultDirectories;
  QString m_startupDefaultDirectory;

  // -------------------------------------------------------
  cvf::ref<cvf::Font> m_standardFont;
  cvf::ref<cvf::Font> m_customFont;

  QMap<QString, QVariant> m_sessionCache;     // Session cache used to store username/passwords per session

  std::list<RimCommandObject*> m_commandQueue;
  QMutex m_commandQueueLock;

  QString m_helpText;

  // -------------------------------------------------------
  bool m_runningWorkerProcess;

  RiuMainPlotWindow* m_mainPlotWindow;

  std::unique_ptr<RiuRecentFileActionProvider> m_recentFileActionProvider;
};
