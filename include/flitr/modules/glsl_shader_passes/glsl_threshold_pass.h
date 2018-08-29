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

#ifndef GLSL_THRESHOLD_PASS_H
#define GLSL_THRESHOLD_PASS_H 1

#include <iostream>
#include <string>

#include <flitr/glsl_image_processor.h>

#include <flitr/flitr_export.h>
#include <flitr/modules/parameters/parameters.h>
#include <flitr/texture.h>

namespace flitr {

/*!
*	Takes an input image and produces a binary output image where all values above a given
*	threshold becomes 1 and everything else becomes 0.
*/
class FLITR_EXPORT GLSLThresholdPass : public GLSLImageProcessor
{
  public:
    GLSLThresholdPass(flitr::TextureRectangle *in_tex, bool read_back_to_CPU = false);
    virtual ~GLSLThresholdPass();

    osg::ref_ptr<osg::Group> getRoot() { return RootGroup_; }
    osg::ref_ptr<flitr::TextureRectangle> getOutputTexture() { return OutTexture_; }
    osg::ref_ptr<osg::Image> getOSGImage() { return OutImage_; }

	//! Value between 0 and 1, anything above \c threshold becomes 1 else 0
    void setThreshold(float value);
    float getThreshold() const;

    /*!
    * Following functions overwrite the flitr::Parameters virtual functions
    */
	virtual int getNumberOfParms()
    {
        return 1;
    }

    virtual flitr::Parameters::EParmType getParmType(int id)
    {
        return flitr::Parameters::PARM_FLOAT;
    }

    virtual std::string getParmName(int id)
    {
        switch (id)
        {
        case 0 :return std::string("Threshold");
        }
        return std::string("???");
    }

    virtual float getFloat(int id)
    {
        switch (id)
        {
        case 0 : return getThreshold()*100.0f;
        }

        return 0.0f;
    }

    virtual bool getFloatRange(int id, float &low, float &high)
    {
        if (id==0)
        {
            low=1.0; high=100.0;
            return true;
        }

        return false;
    }

    virtual bool setFloat(int id, float v)
    {
        switch (id)
        {
            case 0 : setThreshold(v/100.0f); return true;
        }

        return false;
    }

    virtual std::string getTitle()
    {
        return Title_;
    }

  private:
	std::string Title_ = "GLSL Threshold Pass";

    void setShader();

    osg::ref_ptr<osg::Group> createTexturedQuad();
    void createOutputTexture(bool read_back_to_CPU);
    void setupCamera();

	osg::ref_ptr<osg::Uniform> UniformThreshold_;
};
}
#endif //GLSL_THRESHOLD_PASS_H
