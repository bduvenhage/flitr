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

#include <flitr/manipulator_utils.h>

void flitr::adjustCameraManipulatorHomeForYUp(osgGA::CameraManipulator* m)
{
    m->home(0);
    osg::Vec3d home_eye, home_center, home_up;
    m->getHomePosition(home_eye, home_center, home_up);
    
    // rotate the center to eye vector
    osg::Quat q(osg::DegreesToRadians(90.0),osg::Vec3d(1,0,0));
    osg::Vec3d new_eye = home_center + (q*(home_center-home_eye));
    
    osg::Vec3d new_center(home_center);
    osg::Vec3d new_up(0,1,0);
    
    m->setHomePosition(new_eye, new_center, new_up);
    m->home(0);
}
