/* Framework for Live Image Transformation (FLITr) 
 * Copyright (c) 2010 CSIR
 * 
 * This file is part of FLITr.
 *
 * FLITr is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * FLITr is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with FLITr. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef FLITR_QUAD_OVERLAY_H
#define FLITR_QUAD_OVERLAY_H 1

#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Group>
#include <osg/Switch>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osg/Projection>
#include <osg/Geode>
#include <osg/Vec4d>
#include <osg/Material>
#include <osg/LineWidth>

#include <flitr/geometry_overlay.h>

namespace flitr {

/**
 * Filled or outline quad with a given center, width and height.
 */
class QuadOverlay : public GeometryOverlay
{
  public:
    QuadOverlay(double center_x, double center_y, double width, double height, bool filled=false);
    ~QuadOverlay() {};

    void setCenter(double x, double y);
    void setWidth(double width);
    void setHeight(double height);

  private:
    void makeQuad(bool filled);
    void updateQuad();
    osg::ref_ptr<osg::Vec3Array> _Vertices;

    double _CenterX;
    double _CenterY;
    double _Width;
    double _Height;
};

}

#endif // FLITR_QUAD_OVERLAY_H