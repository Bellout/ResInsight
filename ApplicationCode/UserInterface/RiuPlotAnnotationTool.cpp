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

#include "RiuPlotAnnotationTool.h"

#include <QString>

#include "qwt_plot.h"
#include "cvfMath.h"

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuPlotAnnotationTool::attachFormationNames(const std::vector<QString>& names, const std::vector<std::pair<double, double>> yPositions)
{
    detachAllAnnotations();
    if (names.size() != yPositions.size()) return;

    QPen curvePen;
    curvePen.setStyle(Qt::DashLine);
    curvePen.setColor(Qt::blue);
    curvePen.setWidth(1);

    double delta = 0.5;

    for (size_t i = 0; i < names.size(); i++)
    {
        std::unique_ptr<QwtPlotMarker> line(new QwtPlotMarker());
   
        line->setLineStyle(QwtPlotMarker::HLine);
        line->setLinePen(curvePen);
        line->setYValue(yPositions[i].first);
        QString name = "Top ";
        name += names[i];

        line->setLabel(name);
        line->setLabelAlignment(Qt::AlignRight | Qt::AlignBottom);

        line->attach(m_plot);

        m_markers.push_back(std::move(line));

        if ((i == names.size() - 1) || cvf::Math::abs(yPositions[i].second - yPositions[i+1].first) > delta)
        {
            std::unique_ptr<QwtPlotMarker> line(new QwtPlotMarker());

            line->setLineStyle(QwtPlotMarker::HLine);
            line->setLinePen(curvePen);
            line->setYValue(yPositions[i].second);
            
            line->attach(m_plot);

            m_markers.push_back(std::move(line));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RiuPlotAnnotationTool::detachAllAnnotations()
{
    for (size_t i = 0; i < m_markers.size(); i++)
    {
        m_markers[i]->detach();
    }
    m_markers.clear();
}