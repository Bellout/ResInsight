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

#include "cvfBase.h"
#include "cvfObject.h"
#include <QString>

#include <vector>
#include "RiaEclipseUnitTools.h"
#include "cvfVector3.h"

class RigFractureGrid;
class MinMaxAccumulator;
class PosNegAccumulator;

class RigStimPlanResultFrames
{
public:
    RigStimPlanResultFrames() {}

    QString resultName;
    QString unit;

    // Vector for each time step, for each y and for each x-value
    std::vector< std::vector< std::vector<double> > > parameterValues;

};

class RigStimPlanFractureDefinition: public cvf::Object
{ 
    friend class RifStimPlanXmlReader;

public:
    static const double THRESHOLD_VALUE;

    RigStimPlanFractureDefinition();
    ~RigStimPlanFractureDefinition();

    RiaEclipseUnitTools::UnitSystem           unitSet() const { return m_unitSet; }
    size_t                                    xCount() const { return m_Xs.size(); }
    size_t                                    yCount() const { return m_Ys.size(); }
    double                                    minY()   const { return m_Ys[0]; }
    double                                    maxY()   const { return m_Ys.back(); }

    double                                    minDepth()   const { return -minY(); }
    double                                    maxDepth()   const { return -maxY(); }

    double                                    topPerfTvd() const { return m_topPerfTvd; }
    double                                    bottomPerfTvd() const { return m_bottomPerfTvd; }
    void                                      setTvdToTopPerf(double topPerfTvd, RiaDefines::DepthUnitType unit);
    void                                      setTvdToBottomPerf(double bottomPerfTvd, RiaDefines::DepthUnitType unit);

    void                                      enableXMirrorMode(bool enable) { m_xMirrorMode = enable; }
    bool                                      xMirrorModeEnabled() const { return m_xMirrorMode; }

private:
    void                                    generateXsFromFileXs(bool xMirrorMode);
    std::vector<std::vector<double>>        generateDataLayoutFromFileDataLayout(std::vector<std::vector<double>> rawXYData);


    std::vector<double>                       adjustedYCoordsAroundWellPathPosition(double wellPathIntersectionAtFractureDepth) const;
public:

    cvf::ref<RigFractureGrid>                 createFractureGrid(const QString& resultName,
                                                                 int activeTimeStepIndex,
                                                                 RiaEclipseUnitTools::UnitSystemType fractureTemplateUnit,
                                                                 double wellPathIntersectionAtFractureDepth);
                                              

    void                                      createFractureTriangleGeometry(double wellPathIntersectionAtFractureDepth,
                                                                             RiaEclipseUnitTools::UnitSystem neededUnit,
                                                                             const QString& fractureUserName,
                                                                             std::vector<cvf::Vec3f>* vertices,
                                                                             std::vector<cvf::uint>* triangleIndices);
                                              
    std::vector<cvf::Vec3f>                   createFractureBorderPolygon(const QString& resultName,
                                                                          const QString& resultUnit,
                                                                          int activeTimeStepIndex,
                                                                          double wellPathIntersectionAtFractureDepth,
                                                                          RiaEclipseUnitTools::UnitSystem neededUnit,
                                                                          const QString& fractureUserName);
    // Result Access                                          
    
    const std::vector<double>&                timeSteps() const { return m_timeSteps; }
    void                                      addTimeStep(double time) { if (!timeStepExists(time)) m_timeSteps.push_back(time); }

    std::vector<std::pair<QString, QString> > getStimPlanPropertyNamesUnits() const;
private:
    bool                                      numberOfParameterValuesOK(std::vector<std::vector<double>> propertyValuesAtTimestep);
public:
    size_t                                    totalNumberTimeSteps();
    void                                      setDataAtTimeValue(QString resultName, QString unit, std::vector<std::vector<double>> data, double timeStepValue);
    const std::vector<std::vector<double>>&   getDataAtTimeIndex(const QString& resultName, const QString& unit, size_t timeStepIndex) const;
    std::vector<double>                       fractureGridResults(const QString& resultName, 
                                                                  const QString& unitName, 
                                                                  size_t timeStepIndex) const;

    void                                      appendDataToResultStatistics(const QString& resultName, const QString& unit, 
                                                                           MinMaxAccumulator& minMaxAccumulator,
                                                                           PosNegAccumulator& posNegAccumulator) const;

    QStringList                               conductivityResultNames() const;

private:                                      
    bool                                      timeStepExists(double timeStepValue);
    size_t                                    getTimeStepIndex(double timeStepValue);
    size_t                                    resultIndex(const QString& resultName, const QString& unit) const;

private:
    // Data read from file
    RiaEclipseUnitTools::UnitSystem         m_unitSet;
    std::vector<double>                     m_fileXs;
    std::vector<double>                     m_Ys;
    std::vector<double>                     m_timeSteps;

    // Modified file data
    std::vector<RigStimPlanResultFrames>    m_stimPlanResults;
    std::vector<double>                     m_Xs;
    bool                                    m_xMirrorMode;

    double                                  m_topPerfTvd;
    double                                  m_bottomPerfTvd;
};
