/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Statoil ASA
//  Copyright (C) Ceetron Solutions AS
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

#include "RivIntersectionPartMgr.h"

#include "RigCaseCellResultsData.h"
#include "RigFemPartCollection.h"
#include "RigFemPartResultsCollection.h"
#include "RigGeoMechCaseData.h"
#include "RigResultAccessor.h"
#include "RigResultAccessorFactory.h"

#include "Rim2dIntersectionView.h"
#include "RimIntersection.h"
#include "RimEclipseCase.h"
#include "RimEclipseCellColors.h"
#include "RimEclipseView.h"
#include "RimGeoMechCase.h"
#include "RimGeoMechCellColors.h"
#include "RimGeoMechView.h"
#include "RimLegendConfig.h"
#include "RimSimWellInView.h"
#include "RimSimWellInViewCollection.h"
#include "RimTernaryLegendConfig.h"
#include "RimWellPath.h"
#include "RimWellPathCollection.h"

#include "RivHexGridIntersectionTools.h"
#include "RivIntersectionGeometryGenerator.h"
#include "RivObjectSourceInfo.h"
#include "RivIntersectionSourceInfo.h"
#include "RivPartPriority.h"
#include "RivPipeGeometryGenerator.h"
#include "RivResultToTextureMapper.h"
#include "RivScalarMapperUtils.h"
#include "RivSimWellPipeSourceInfo.h"
#include "RivTernaryScalarMapper.h"
#include "RivTernaryTextureCoordsCreator.h"
#include "RivWellPathSourceInfo.h"

#include "RiuGeoMechXfTensorResultAccessor.h"

#include "cafTensor3.h"

#include "cvfDrawableGeo.h"
#include "cvfGeometryTools.h"
#include "cvfModelBasicList.h"
#include "cvfPart.h"
#include "cvfPrimitiveSetDirect.h"
#include "cvfRenderState_FF.h"
#include "cvfRenderStateDepth.h"
#include "cvfRenderStatePoint.h"
#include "cvfStructGridGeometryGenerator.h"
#include "cvfTransform.h"

#include <functional>


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RivIntersectionPartMgr::RivIntersectionPartMgr(RimIntersection* rimCrossSection, bool isFlattened)
    : m_rimCrossSection(rimCrossSection),
    m_isFlattened(isFlattened)
{
    CVF_ASSERT(m_rimCrossSection);

    m_crossSectionFacesTextureCoords = new cvf::Vec2fArray;
    
    double horizontalLengthAlongWellToPolylineStart;

    std::vector< std::vector <cvf::Vec3d> > polyLines = m_rimCrossSection->polyLines(&horizontalLengthAlongWellToPolylineStart);
    if (polyLines.size() > 0)
    {
        cvf::Vec3d direction = m_rimCrossSection->extrusionDirection();
        cvf::ref<RivIntersectionHexGridInterface> hexGrid = createHexGridInterface();
        m_crossSectionGenerator = new RivIntersectionGeometryGenerator(m_rimCrossSection, 
                                                                       polyLines, 
                                                                       direction, 
                                                                       hexGrid.p(), 
                                                                       m_isFlattened, 
                                                                       horizontalLengthAlongWellToPolylineStart);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::applySingleColorEffect()
{
    if (m_crossSectionGenerator.isNull()) return;

    caf::SurfaceEffectGenerator geometryEffgen(cvf::Color3f::OLIVE, caf::PO_1);

    cvf::ref<cvf::Effect> geometryOnlyEffect = geometryEffgen.generateCachedEffect();

    if (m_crossSectionFaces.notNull())
    {
        m_crossSectionFaces->setEffect(geometryOnlyEffect.p());
    }

    // Update mesh colors as well, in case of change
    //RiaPreferences* prefs = RiaApplication::instance()->preferences();

    cvf::ref<cvf::Effect> eff;
    caf::MeshEffectGenerator CrossSectionEffGen(cvf::Color3::WHITE);//prefs->defaultCrossSectionGridLineColors());
    eff = CrossSectionEffGen.generateCachedEffect();

    if (m_crossSectionGridLines.notNull())
    {
        m_crossSectionGridLines->setEffect(eff.p());
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::updateCellResultColor(size_t timeStepIndex, 
                                                   const cvf::ScalarMapper* scalarColorMapper, 
                                                   const RivTernaryScalarMapper* ternaryColorMapper)
{
    CVF_ASSERT(scalarColorMapper);

    if (m_crossSectionGenerator.isNull()) return;

    if (!m_crossSectionGenerator->isAnyGeometryPresent()) return;

    RimEclipseView* eclipseView = nullptr;
    m_rimCrossSection->firstAncestorOrThisOfType(eclipseView);

    if (eclipseView)
    {
        RimEclipseCellColors* cellResultColors = eclipseView->cellResult();
        CVF_ASSERT(cellResultColors);
        CVF_ASSERT(ternaryColorMapper);

        RigEclipseCaseData* eclipseCase = eclipseView->eclipseCase()->eclipseCaseData();

        // CrossSections
        if (m_crossSectionFaces.notNull())
        {
            if (cellResultColors->isTernarySaturationSelected())
            {
                RivTernaryTextureCoordsCreator texturer(cellResultColors, ternaryColorMapper, timeStepIndex);
                
                texturer.createTextureCoords(m_crossSectionFacesTextureCoords.p(), m_crossSectionGenerator->triangleToCellIndex());

                RivScalarMapperUtils::applyTernaryTextureResultsToPart(m_crossSectionFaces.p(),
                                                                       m_crossSectionFacesTextureCoords.p(),
                                                                       ternaryColorMapper,
                                                                       1.0,
                                                                       caf::FC_NONE,
                                                                       eclipseView->isLightingDisabled());
            }
            else
            {
                CVF_ASSERT(m_crossSectionGenerator.notNull());

                cvf::ref<RigResultAccessor> resultAccessor;

                if (RiaDefines::isPerCellFaceResult(cellResultColors->resultVariable()))
                {
                    resultAccessor = new RigHugeValResultAccessor;
                }
                else
                {
                    resultAccessor = RigResultAccessorFactory::createFromResultDefinition(cellResultColors->reservoirView()->eclipseCase()->eclipseCaseData(),
                                                                                          0,
                                                                                          timeStepIndex,
                                                                                          cellResultColors);
                }

                RivIntersectionPartMgr::calculateEclipseTextureCoordinates(m_crossSectionFacesTextureCoords.p(),
                                                                            m_crossSectionGenerator->triangleToCellIndex(),
                                                                            resultAccessor.p(),
                                                                            scalarColorMapper);


                RivScalarMapperUtils::applyTextureResultsToPart(m_crossSectionFaces.p(),
                                                                m_crossSectionFacesTextureCoords.p(),
                                                                scalarColorMapper,
                                                                1.0,
                                                                caf::FC_NONE,
                                                                eclipseView->isLightingDisabled());
                }
        }
    }

    RimGeoMechView* geoView;
    m_rimCrossSection->firstAncestorOrThisOfType(geoView);

    if (geoView)
    {
        RimGeoMechCellColors* cellResultColors = geoView->cellResult();
        RigGeoMechCaseData* caseData = cellResultColors->ownerCaseData();
        
        if (!caseData) return;

        RigFemResultAddress      resVarAddress = cellResultColors->resultAddress();

        if (resVarAddress.resultPosType == RIG_ELEMENT)
        {
            const std::vector<float>& resultValues          = caseData->femPartResults()->resultValues(resVarAddress, 0, (int)timeStepIndex);
            const std::vector<size_t>& triangleToCellIdx    = m_crossSectionGenerator->triangleToCellIndex();

            RivIntersectionPartMgr::calculateElementBasedGeoMechTextureCoords(m_crossSectionFacesTextureCoords.p(),
                                                                              resultValues,
                                                                              triangleToCellIdx,
                                                                              scalarColorMapper);

        }
        else if(resVarAddress.resultPosType == RIG_ELEMENT_NODAL_FACE)
        {
            // Special direction sensitive result calculation
            const cvf::Vec3fArray* triangelVxes = m_crossSectionGenerator->triangleVxes();

            if (resVarAddress.componentName == "Pazi" || resVarAddress.componentName == "Pinc")
            {
                RivIntersectionPartMgr::calculatePlaneAngleTextureCoords(m_crossSectionFacesTextureCoords.p(),
                                                                         triangelVxes,
                                                                         resVarAddress,
                                                                         scalarColorMapper);
            }
            else
            {
                const std::vector<RivIntersectionVertexWeights> &vertexWeights = m_crossSectionGenerator->triangleVxToCellCornerInterpolationWeights();

                RivIntersectionPartMgr::calculateGeoMechTensorXfTextureCoords(m_crossSectionFacesTextureCoords.p(),
                                                                              triangelVxes,
                                                                              vertexWeights,
                                                                              caseData,
                                                                              resVarAddress,
                                                                              (int)timeStepIndex,
                                                                              scalarColorMapper);
            }
        }
        else
        {
            // Do a "Hack" to show elm nodal and not nodal POR results
            if (resVarAddress.resultPosType == RIG_NODAL && resVarAddress.fieldName == "POR-Bar") resVarAddress.resultPosType = RIG_ELEMENT_NODAL;

            const std::vector<float>& resultValues = caseData->femPartResults()->resultValues(resVarAddress, 0, (int)timeStepIndex);
            RigFemPart* femPart = caseData->femParts()->part(0);
            bool isElementNodalResult = !(resVarAddress.resultPosType == RIG_NODAL);
            const std::vector<RivIntersectionVertexWeights> &vertexWeights = m_crossSectionGenerator->triangleVxToCellCornerInterpolationWeights();

            RivIntersectionPartMgr::calculateNodeOrElementNodeBasedGeoMechTextureCoords(m_crossSectionFacesTextureCoords.p(),
                                                                                        vertexWeights,
                                                                                        resultValues,
                                                                                        isElementNodalResult,
                                                                                        femPart,
                                                                                        scalarColorMapper);
        }

        RivScalarMapperUtils::applyTextureResultsToPart(m_crossSectionFaces.p(), 
                                                        m_crossSectionFacesTextureCoords.p(), 
                                                        scalarColorMapper, 
                                                        1.0, 
                                                        caf::FC_NONE, 
                                                        geoView->isLightingDisabled());
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::calculateNodeOrElementNodeBasedGeoMechTextureCoords(cvf::Vec2fArray* textureCoords, 
                                                                                 const std::vector<RivIntersectionVertexWeights> &vertexWeights,
                                                                                 const std::vector<float> &resultValues,
                                                                                 bool isElementNodalResult,
                                                                                 const RigFemPart* femPart,
                                                                                 const cvf::ScalarMapper* mapper)
{
    textureCoords->resize(vertexWeights.size());

    if (resultValues.size() == 0)
    {
        textureCoords->setAll(cvf::Vec2f(0.0, 1.0f));
    }
    else
    {
        cvf::Vec2f* rawPtr = textureCoords->ptr();

        int vxCount = static_cast<int>(vertexWeights.size());

#pragma omp parallel for schedule(dynamic)
        for (int triangleVxIdx = 0; triangleVxIdx < vxCount; ++triangleVxIdx)
        {
            float resValue = 0;
            int weightCount = vertexWeights[triangleVxIdx].size();
            for (int wIdx = 0; wIdx < weightCount; ++wIdx)
            {
                size_t resIdx;
                if (isElementNodalResult)
                {
                    resIdx = vertexWeights[triangleVxIdx].vxId(wIdx);
                }
                else
                {
                    resIdx = femPart->nodeIdxFromElementNodeResultIdx(vertexWeights[triangleVxIdx].vxId(wIdx));
                }
                    
                resValue += resultValues[resIdx] * vertexWeights[triangleVxIdx].weight(wIdx);
            }

            if (resValue == HUGE_VAL || resValue != resValue) // a != a is true for NAN's
            {
                rawPtr[triangleVxIdx][1]       = 1.0f;
            }
            else
            {
                rawPtr[triangleVxIdx] = mapper->mapToTextureCoord(resValue);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::calculateElementBasedGeoMechTextureCoords(cvf::Vec2fArray* textureCoords,
                                                                       const std::vector<float> &resultValues,
                                                                       const std::vector<size_t>& triangleToCellIdx,
                                                                       const cvf::ScalarMapper* mapper)
{
    textureCoords->resize(triangleToCellIdx.size()*3);

    if (resultValues.size() == 0)
    {
        textureCoords->setAll(cvf::Vec2f(0.0, 1.0f));
    }
    else
    {
        cvf::Vec2f* rawPtr = textureCoords->ptr();

        for (size_t triangleIdx = 0; triangleIdx < triangleToCellIdx.size(); triangleIdx++)
        {
            size_t resIdx = triangleToCellIdx[triangleIdx];
            float resValue = resultValues[resIdx];

            size_t triangleVxIdx = triangleIdx * 3;
            
            if (resValue == HUGE_VAL || resValue != resValue) // a != a is true for NAN's
            {
                rawPtr[triangleVxIdx][1] = 1.0f;
                rawPtr[triangleVxIdx + 1][1] = 1.0f;
                rawPtr[triangleVxIdx + 2][1] = 1.0f;
            }
            else
            {
                rawPtr[triangleVxIdx] = mapper->mapToTextureCoord(resValue);
                rawPtr[triangleVxIdx + 1] = mapper->mapToTextureCoord(resValue);
                rawPtr[triangleVxIdx + 2] = mapper->mapToTextureCoord(resValue);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::calculateGeoMechTensorXfTextureCoords(cvf::Vec2fArray* textureCoords, 
                                                                   const cvf::Vec3fArray* triangelVertices,
                                                                   const std::vector<RivIntersectionVertexWeights> &vertexWeights, 
                                                                   RigGeoMechCaseData* caseData, 
                                                                   const RigFemResultAddress& resVarAddress, 
                                                                   int   timeStepIdx,
                                                                   const cvf::ScalarMapper* mapper)
{  

    RiuGeoMechXfTensorResultAccessor accessor(caseData->femPartResults(), resVarAddress, timeStepIdx);

    textureCoords->resize(vertexWeights.size());
    cvf::Vec2f* rawPtr = textureCoords->ptr();
    int vxCount = static_cast<int>(vertexWeights.size());
    int triCount = vxCount/3;

    #pragma omp parallel for schedule(dynamic)
    for ( int triangleIdx = 0; triangleIdx < triCount; ++triangleIdx )
    {
        int triangleVxStartIdx =  triangleIdx*3;
        float values[3];

        accessor.calculateInterpolatedValue(&((*triangelVertices)[triangleVxStartIdx]), &(vertexWeights[triangleVxStartIdx]), values );

        rawPtr[triangleVxStartIdx + 0] = (values[0] != std::numeric_limits<float>::infinity()) ? mapper->mapToTextureCoord(values[0]) : cvf::Vec2f(0.0f, 1.0f);
        rawPtr[triangleVxStartIdx + 1] = (values[1] != std::numeric_limits<float>::infinity()) ? mapper->mapToTextureCoord(values[1]) : cvf::Vec2f(0.0f, 1.0f);
        rawPtr[triangleVxStartIdx + 2] = (values[2] != std::numeric_limits<float>::infinity()) ? mapper->mapToTextureCoord(values[2]) : cvf::Vec2f(0.0f, 1.0f);
    }

}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::calculatePlaneAngleTextureCoords(cvf::Vec2fArray* textureCoords,
                                                                   const cvf::Vec3fArray* triangelVertices,
                                                                   const RigFemResultAddress& resVarAddress,
                                                                   const cvf::ScalarMapper* mapper)
{

    textureCoords->resize(triangelVertices->size());
    cvf::Vec2f* rawPtr = textureCoords->ptr();
    int vxCount = static_cast<int>(triangelVertices->size());
    int triCount = vxCount/3;

    std::function<float (const OffshoreSphericalCoords& )> operation;
    if (resVarAddress.componentName == "Pazi")
    {
        operation = [](const OffshoreSphericalCoords& sphCoord) { return sphCoord.azi();};
    }
    else if ( resVarAddress.componentName == "Pinc" )
    {
        operation = [](const OffshoreSphericalCoords& sphCoord) { return sphCoord.inc();};
    }

    #pragma omp parallel for schedule(dynamic)
    for ( int triangleIdx = 0; triangleIdx < triCount; ++triangleIdx )
    {
        int triangleVxStartIdx =  triangleIdx*3;
        
        const cvf::Vec3f* triangle = &((*triangelVertices)[triangleVxStartIdx]);
        cvf::Mat3f rotMx = cvf::GeometryTools::computePlaneHorizontalRotationMx(triangle[1] - triangle[0], triangle[2] - triangle[0]);

        OffshoreSphericalCoords sphCoord(cvf::Vec3f(rotMx.rowCol(0, 2), rotMx.rowCol(1, 2), rotMx.rowCol(2, 2))); // Use Ez from the matrix as plane normal

        float angle = cvf::Math::toDegrees( operation(sphCoord));
        cvf::Vec2f texCoord = (angle != std::numeric_limits<float>::infinity()) ? mapper->mapToTextureCoord(angle) : cvf::Vec2f(0.0f, 1.0f);
        rawPtr[triangleVxStartIdx + 0] = texCoord;
        rawPtr[triangleVxStartIdx + 1] = texCoord;
        rawPtr[triangleVxStartIdx + 2] = texCoord;
    }

}


//--------------------------------------------------------------------------------------------------
/// Calculates the texture coordinates in a "nearly" one dimensional texture. 
/// Undefined values are coded with a y-texturecoordinate value of 1.0 instead of the normal 0.5
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::calculateEclipseTextureCoordinates(cvf::Vec2fArray* textureCoords, 
                                                          const std::vector<size_t>& triangleToCellIdxMap,
                                                          const RigResultAccessor* resultAccessor, 
                                                          const cvf::ScalarMapper* mapper) 
{
    if (!resultAccessor) return;

    size_t numVertices = triangleToCellIdxMap.size()*3;

    textureCoords->resize(numVertices);
    cvf::Vec2f* rawPtr = textureCoords->ptr();

    int triangleCount = static_cast<int>(triangleToCellIdxMap.size());

#pragma omp parallel for
    for (int tIdx = 0; tIdx < triangleCount; tIdx++)
    {
        double cellScalarValue = resultAccessor->cellScalarGlobIdx(triangleToCellIdxMap[tIdx]);
        cvf::Vec2f texCoord = mapper->mapToTextureCoord(cellScalarValue);
        if (cellScalarValue == HUGE_VAL || cellScalarValue != cellScalarValue) // a != a is true for NAN's
        {
            texCoord[1] = 1.0f;
        }

        size_t j;
        for (j = 0; j < 3; j++)
        {   
            rawPtr[tIdx*3 + j] = texCoord;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::generatePartGeometry()
{
    if (m_crossSectionGenerator.isNull()) return;

    bool useBufferObjects = true;
    // Surface geometry
    {
        cvf::ref<cvf::DrawableGeo> geo = m_crossSectionGenerator->generateSurface();
        if (geo.notNull())
        {
            geo->computeNormals();

            if (useBufferObjects)
            {
                geo->setRenderMode(cvf::DrawableGeo::BUFFER_OBJECT);
            }

            cvf::ref<cvf::Part> part = new cvf::Part;
            part->setName("Cross Section");
            part->setDrawable(geo.p());

            // Set mapping from triangle face index to cell index
            cvf::ref<RivIntersectionSourceInfo> si = new RivIntersectionSourceInfo(m_crossSectionGenerator.p());
            part->setSourceInfo(si.p());

            part->updateBoundingBox();
            part->setEnableMask(faultBit);
            part->setPriority(RivPartPriority::PartType::Intersection);

            m_crossSectionFaces = part;
        }
    }

    // Mesh geometry
    {
        cvf::ref<cvf::DrawableGeo> geoMesh = m_crossSectionGenerator->createMeshDrawable();
        if (geoMesh.notNull())
        {
            if (useBufferObjects)
            {
                geoMesh->setRenderMode(cvf::DrawableGeo::BUFFER_OBJECT);
            }

            cvf::ref<cvf::Part> part = new cvf::Part;
            part->setName("Cross Section mesh");
            part->setDrawable(geoMesh.p());

            part->updateBoundingBox();
            part->setEnableMask(meshFaultBit);
            part->setPriority(RivPartPriority::PartType::MeshLines);

            m_crossSectionGridLines = part;
        }
    }

    createPolyLineParts(useBufferObjects);

    createExtrusionDirParts(useBufferObjects);

    applySingleColorEffect();
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::createPolyLineParts(bool useBufferObjects)
{
    // Highlight line

    m_highlightLineAlongPolyline = nullptr;
    m_highlightPointsForPolyline = nullptr;

    if (m_rimCrossSection->type == RimIntersection::CS_POLYLINE || m_rimCrossSection->type == RimIntersection::CS_AZIMUTHLINE)
    {
        {
            cvf::ref<cvf::DrawableGeo> polylineGeo = m_crossSectionGenerator->createLineAlongPolylineDrawable();
            if (polylineGeo.notNull())
            {
                if (useBufferObjects)
                {
                    polylineGeo->setRenderMode(cvf::DrawableGeo::BUFFER_OBJECT);
                }

                cvf::ref<cvf::Part> part = new cvf::Part;
                part->setName("Cross Section Polyline");
                part->setDrawable(polylineGeo.p());

                part->updateBoundingBox();
                part->setPriority(RivPartPriority::PartType::Highlight);

                // Always show this part, also when mesh is turned off
                //part->setEnableMask(meshFaultBit);

                cvf::ref<cvf::Effect> eff;
                caf::MeshEffectGenerator lineEffGen(cvf::Color3::MAGENTA);
                eff = lineEffGen.generateUnCachedEffect();

                cvf::ref<cvf::RenderStateDepth> depth = new cvf::RenderStateDepth;
                depth->enableDepthTest(false);
                eff->setRenderState(depth.p());

                part->setEffect(eff.p());

                m_highlightLineAlongPolyline = part;
            }
        }

        cvf::ref<cvf::DrawableGeo> polylinePointsGeo = m_crossSectionGenerator->createPointsFromPolylineDrawable();
        if (polylinePointsGeo.notNull())
        {
            if (useBufferObjects)
            {
                polylinePointsGeo->setRenderMode(cvf::DrawableGeo::BUFFER_OBJECT);
            }

            cvf::ref<cvf::Part> part = new cvf::Part;
            part->setName("Cross Section Polyline");
            part->setDrawable(polylinePointsGeo.p());

            part->updateBoundingBox();
            part->setPriority(RivPartPriority::PartType::Highlight);

            // Always show this part, also when mesh is turned off
            //part->setEnableMask(meshFaultBit);

            cvf::ref<cvf::Effect> eff;
            caf::MeshEffectGenerator lineEffGen(cvf::Color3::MAGENTA);
            eff = lineEffGen.generateUnCachedEffect();

            cvf::ref<cvf::RenderStateDepth> depth = new cvf::RenderStateDepth;
            depth->enableDepthTest(false);
            eff->setRenderState(depth.p());

            cvf::ref<cvf::RenderStatePoint> pointRendState = new  cvf::RenderStatePoint(cvf::RenderStatePoint::FIXED_SIZE);
            pointRendState->setSize(5.0f);
            eff->setRenderState(pointRendState.p());

            part->setEffect(eff.p());

            m_highlightPointsForPolyline = part;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::createExtrusionDirParts(bool useBufferObjects)
{
    m_highlightLineAlongExtrusionDir = nullptr;
    m_highlightPointsForExtrusionDir = nullptr;

    if (m_rimCrossSection->direction() == RimIntersection::CS_TWO_POINTS)
    {
        {
            cvf::ref<cvf::DrawableGeo> polylineGeo = m_crossSectionGenerator->createLineAlongPolylineDrawable(m_rimCrossSection->polyLinesForExtrusionDirection());
            if (polylineGeo.notNull())
            {
                if (useBufferObjects)
                {
                    polylineGeo->setRenderMode(cvf::DrawableGeo::BUFFER_OBJECT);
                }

                cvf::ref<cvf::Part> part = new cvf::Part;
                part->setName("Cross Section Polyline");
                part->setDrawable(polylineGeo.p());

                part->updateBoundingBox();
                part->setPriority(RivPartPriority::PartType::Highlight);

                // Always show this part, also when mesh is turned off
                //part->setEnableMask(meshFaultBit);

                cvf::ref<cvf::Effect> eff;
                caf::MeshEffectGenerator lineEffGen(cvf::Color3::MAGENTA);
                eff = lineEffGen.generateUnCachedEffect();

                cvf::ref<cvf::RenderStateDepth> depth = new cvf::RenderStateDepth;
                depth->enableDepthTest(false);
                eff->setRenderState(depth.p());

                part->setEffect(eff.p());

                m_highlightLineAlongExtrusionDir = part;
            }
        }

        cvf::ref<cvf::DrawableGeo> polylinePointsGeo = m_crossSectionGenerator->createPointsFromPolylineDrawable(m_rimCrossSection->polyLinesForExtrusionDirection());
        if (polylinePointsGeo.notNull())
        {
            if (useBufferObjects)
            {
                polylinePointsGeo->setRenderMode(cvf::DrawableGeo::BUFFER_OBJECT);
            }

            cvf::ref<cvf::Part> part = new cvf::Part;
            part->setName("Cross Section Polyline");
            part->setDrawable(polylinePointsGeo.p());

            part->updateBoundingBox();
            part->setPriority(RivPartPriority::PartType::Highlight);

            // Always show this part, also when mesh is turned off
            //part->setEnableMask(meshFaultBit);

            cvf::ref<cvf::Effect> eff;
            caf::MeshEffectGenerator lineEffGen(cvf::Color3::MAGENTA);
            eff = lineEffGen.generateUnCachedEffect();

            cvf::ref<cvf::RenderStateDepth> depth = new cvf::RenderStateDepth;
            depth->enableDepthTest(false);
            eff->setRenderState(depth.p());

            cvf::ref<cvf::RenderStatePoint> pointRendState = new  cvf::RenderStatePoint(cvf::RenderStatePoint::FIXED_SIZE);
            pointRendState->setSize(5.0f);
            eff->setRenderState(pointRendState.p());

            part->setEffect(eff.p());

            m_highlightPointsForExtrusionDir = part;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::ref<cvf::Part> createStdSurfacePart(cvf::DrawableGeo* geometry, 
                                                const cvf::Color3f& color, 
                                                cvf::String name, 
                                                cvf::Object* sourceInfo)
{
    if (!geometry) return nullptr;

    cvf::ref<cvf::Part> part = new cvf::Part;
    part->setName(name);
    part->setDrawable(geometry);

    caf::SurfaceEffectGenerator surfaceGen(color, caf::PO_1);
    cvf::ref<cvf::Effect> eff = surfaceGen.generateCachedEffect();
    part->setEffect(eff.p());

    part->setSourceInfo(sourceInfo);
    part->updateBoundingBox();

    return part;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::ref<cvf::Part> createStdLinePart(cvf::DrawableGeo* geometry, 
                                         const cvf::Color3f& color, 
                                         cvf::String name)
{
    if ( !geometry ) return nullptr;


    cvf::ref<cvf::Part> part = new cvf::Part;
    part->setName(name);
    part->setDrawable(geometry);

    caf::MeshEffectGenerator gen(color);
    cvf::ref<cvf::Effect> eff = gen.generateCachedEffect();

    part->setEffect(eff.p());
    part->updateBoundingBox();

    return part;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::appendWellPipePartsToModel(cvf::ModelBasicList* model, cvf::Transform* scaleTransform)
{
    if (m_rimCrossSection.isNull() || m_crossSectionGenerator.isNull()) return;

    // Get information on how to draw the pipe

    std::function< cvf::ref< cvf::Object > ( size_t ) > createSourceInfoFunc;
    double       pipeRadius = 1; 
    int          pipeCrossSectionVxCount = 6;
    cvf::Color3f wellPipeColor = cvf::Color3f::GRAY;
    double       characteristicCellSize = 0;

    if ( m_rimCrossSection->type() == RimIntersection::CS_SIMULATION_WELL )
    {
        RimSimWellInView * simWellInView = m_rimCrossSection->simulationWell();

        if (!simWellInView) return;

        RimEclipseView* eclView = nullptr;
        simWellInView->firstAncestorOrThisOfTypeAsserted(eclView);

        pipeRadius =  simWellInView->pipeRadius();
        pipeCrossSectionVxCount = eclView->wellCollection()->pipeCrossSectionVertexCount();
        wellPipeColor = simWellInView->wellPipeColor();

        createSourceInfoFunc = [&](size_t brIdx) { return new RivSimWellPipeSourceInfo(simWellInView, brIdx); };
        characteristicCellSize = eclView->eclipseCase()->characteristicCellSize();
    }
    else if (m_rimCrossSection->type() == RimIntersection::CS_WELL_PATH)
    {
        RimWellPath* wellPath = m_rimCrossSection->wellPath();

        if (!wellPath) return;

        RigWellPath* wellPathGeometry = wellPath->wellPathGeometry();

        RimGridView* gridView = nullptr;
        m_rimCrossSection->firstAncestorOrThisOfTypeAsserted(gridView);
        double cellSize = gridView->ownerCase()->characteristicCellSize();
       
        RimWellPathCollection* wellPathColl = nullptr;
        wellPath->firstAncestorOrThisOfTypeAsserted(wellPathColl);

        pipeRadius = wellPath->wellPathRadius(cellSize);
        pipeCrossSectionVxCount = wellPathColl->wellPathCrossSectionVertexCount();
        wellPipeColor = wellPath->wellPathColor();

        createSourceInfoFunc = [&](size_t brIdx) { return new RivObjectSourceInfo(wellPath); };  //Temporary fix to avoid crash
    }

    // Create pipe geometry

    if (   m_rimCrossSection->type() == RimIntersection::CS_SIMULATION_WELL
        || m_rimCrossSection->type() == RimIntersection::CS_WELL_PATH )
    {
        std::vector<std::vector<cvf::Vec3d> > polyLines = m_crossSectionGenerator->flattenedOrOffsettedPolyLines();
        
        // Remove intersectino extents from the polyline
        for (auto & polyLine: polyLines)
        {
            if ( polyLine.size() > 2 )
            {
                polyLine.pop_back();
                polyLine.erase(polyLine.begin());
            }
        }

        m_wellBranches.clear();

        for ( size_t brIdx = 0; brIdx < polyLines.size(); ++brIdx )
        {
            cvf::ref<cvf::Object> sourceInfo = createSourceInfoFunc(brIdx);

            m_wellBranches.emplace_back();
            RivPipeBranchData& pbd = m_wellBranches.back();

            pbd.m_pipeGeomGenerator = new RivPipeGeometryGenerator;
            pbd.m_pipeGeomGenerator->setRadius(pipeRadius);
            pbd.m_pipeGeomGenerator->setCrossSectionVertexCount(pipeCrossSectionVxCount);

            cvf::ref<cvf::Vec3dArray> cvfCoords = new cvf::Vec3dArray;
            cvfCoords->assign(polyLines[brIdx]);

            // Scale the centerline coordinates using the Z-scale transform of the grid.

            for ( size_t cIdx = 0; cIdx < cvfCoords->size(); ++cIdx )
            {
                (*cvfCoords)[cIdx].transformPoint(scaleTransform->worldTransform());
            }

            (*cvfCoords)[0].z() += characteristicCellSize;

            pbd.m_pipeGeomGenerator->setPipeCenterCoords(cvfCoords.p());
            auto surfaceDrawable = pbd.m_pipeGeomGenerator->createPipeSurface();
            auto centerLineDrawable = pbd.m_pipeGeomGenerator->createCenterLine();

            pbd.m_surfacePart = createStdSurfacePart(surfaceDrawable.p(),
                                                     wellPipeColor,
                                                     "FlattenedSimWellPipe",
                                                     sourceInfo.p());

            pbd.m_centerLinePart = createStdLinePart(centerLineDrawable.p(),
                                                     wellPipeColor,
                                                     "FlattenedSimWellPipeCenterLine");
            model->addPart(pbd.m_surfacePart.p());
            model->addPart(pbd.m_centerLinePart.p());
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::appendNativeCrossSectionFacesToModel(cvf::ModelBasicList* model, cvf::Transform* scaleTransform)
{
    if (m_crossSectionFaces.isNull())
    {
        generatePartGeometry();
    }

    if (m_crossSectionFaces.notNull())
    {
        m_crossSectionFaces->setTransform(scaleTransform);
        model->addPart(m_crossSectionFaces.p());
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::appendMeshLinePartsToModel(cvf::ModelBasicList* model, cvf::Transform* scaleTransform)
{
    if (m_crossSectionGridLines.isNull())
    {
        generatePartGeometry();
    }

    if (m_crossSectionGridLines.notNull())
    {
        m_crossSectionGridLines->setTransform(scaleTransform);
        model->addPart(m_crossSectionGridLines.p());
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivIntersectionPartMgr::appendPolylinePartsToModel(cvf::ModelBasicList* model, cvf::Transform* scaleTransform)
{
    if (m_rimCrossSection->inputPolyLineFromViewerEnabled)
    {
        if (m_highlightLineAlongPolyline.notNull())
        {
            m_highlightLineAlongPolyline->setTransform(scaleTransform);
            model->addPart(m_highlightLineAlongPolyline.p());
        }

        if (m_highlightPointsForPolyline.notNull())
        {
            m_highlightPointsForPolyline->setTransform(scaleTransform);
            model->addPart(m_highlightPointsForPolyline.p());
        }
    }

    if (m_rimCrossSection->inputExtrusionPointsFromViewerEnabled)
    {
        if (m_highlightLineAlongExtrusionDir.notNull())
        {
            m_highlightLineAlongExtrusionDir->setTransform(scaleTransform);
            model->addPart(m_highlightLineAlongExtrusionDir.p());
        }

        if (m_highlightPointsForExtrusionDir.notNull())
        {
            m_highlightPointsForExtrusionDir->setTransform(scaleTransform);
            model->addPart(m_highlightPointsForExtrusionDir.p());
        }
    }

    if (m_rimCrossSection->inputTwoAzimuthPointsFromViewerEnabled)
    {
        if (m_highlightLineAlongPolyline.notNull())
        {
            m_highlightLineAlongPolyline->setTransform(scaleTransform);
            model->addPart(m_highlightLineAlongPolyline.p());
        }

        if (m_highlightPointsForPolyline.notNull())
        {
            m_highlightPointsForPolyline->setTransform(scaleTransform);
            model->addPart(m_highlightPointsForPolyline.p());
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
const RimIntersection* RivIntersectionPartMgr::intersection() const
{
    return m_rimCrossSection.p();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::Mat4d RivIntersectionPartMgr::unflattenTransformMatrix(const cvf::Vec3d& intersectionPointUtm)
{
    return m_crossSectionGenerator->unflattenTransformMatrix(intersectionPointUtm);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::ref<RivIntersectionHexGridInterface> RivIntersectionPartMgr::createHexGridInterface()
{
    RimEclipseView* eclipseView;
    m_rimCrossSection->firstAncestorOrThisOfType(eclipseView);
    if (eclipseView)
    {
        RigMainGrid* grid = eclipseView->mainGrid();
        return new RivEclipseIntersectionGrid(grid, eclipseView->currentActiveCellInfo(), m_rimCrossSection->showInactiveCells());
    }

    RimGeoMechView* geoView;
    m_rimCrossSection->firstAncestorOrThisOfType(geoView);
    if (geoView)
    {
        RigFemPart* femPart = geoView->geoMechCase()->geoMechData()->femParts()->part(0);
        return new RivFemIntersectionGrid(femPart);
    }

    return nullptr;
}

