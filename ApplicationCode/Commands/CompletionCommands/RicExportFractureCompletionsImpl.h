/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017-     Statoil ASA
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

class RimWellPath;
class RicExportCompletionDataSettingsUi;

#include <vector>
#include "RigCompletionData.h"

class QTextStream;
class RigWellPath;
class RimEclipseCase;
class RimEclipseWell;
class RimFracture;

class RicExportFractureCompletionsImpl
{
public:
    static std::vector<RigCompletionData> generateCompdatValuesForWellPath(const RimWellPath* wellPath,
                                                                           const RicExportCompletionDataSettingsUi& settings, 
                                                                           QTextStream* outputStreamForIntermediateResultsText);

    static std::vector<RigCompletionData> generateCompdatValuesForSimWell(const RimEclipseCase* eclipseCase,
                                                                          const RimEclipseWell* well,
                                                                          size_t timeStep,
                                                                          QTextStream* outputStreamForIntermediateResultsText);

private:
    static std::vector<RigCompletionData> generateCompdatValues(const RimEclipseCase* caseToApply,
                                                                const QString& wellPathName,
                                                                const RigWellPath* wellPathGeometry,
                                                                const std::vector<RimFracture*> fractures,
                                                                QTextStream* outputStreamForIntermediateResultsText);
};