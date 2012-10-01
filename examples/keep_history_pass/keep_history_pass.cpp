#include "keep_history_pass.h"
#include <osgUtil/CullVisitor>

void KeepHistoryPass::CameraCullCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    osg::Camera* fboCam = dynamic_cast<osg::Camera*>( node );
    osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);

    if (fboCam && cv && !hp_->FirstPass_)
    {
        osg::FrameBufferObject* fbo = cv->getCurrentRenderBin()->getStage()->getFrameBufferObject();
        if (fbo) {
            fbo->setAttachment(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER),
                               osg::FrameBufferAttachment(hp_->OutTextures_[hp_->OverwriteIndex_].get()));
        }
    }
    traverse(node, nv);
}

void KeepHistoryPass::CameraPostDrawCallback::operator()(osg::RenderInfo& ri) const
{
    hp_->frameDone();
}

KeepHistoryPass::KeepHistoryPass(osg::TextureRectangle* in_tex, int hist_size) :
    HistorySize_(hist_size),
    OverwriteIndex_(0),
    ValidHistory_(0),
    FirstPass_(true)
{
    TextureWidth_ = in_tex->getTextureWidth();
    TextureHeight_ = in_tex->getTextureHeight();
    InTexture_ = in_tex;

    RootGroup_ = new osg::Group;

    createOutputTextures();

    Camera_ = new osg::Camera;
    
    setupCamera();

    Camera_->addChild(createTexturedQuad().get());

    RootGroup_->addChild(Camera_.get());

    // make sure other state doesn't bleed into here, attach empty shader
    setShader("");
}

KeepHistoryPass::~KeepHistoryPass()
{
}

osg::ref_ptr<osg::Group> KeepHistoryPass::createTexturedQuad()
{
    osg::ref_ptr<osg::Group> top_group = new osg::Group;
    
    osg::ref_ptr<osg::Geode> quad_geode = new osg::Geode;

    osg::ref_ptr<osg::Vec3Array> quad_coords = new osg::Vec3Array; // vertex coords
    // counter-clockwise
    quad_coords->push_back(osg::Vec3d(0, 0, -1));
    quad_coords->push_back(osg::Vec3d(1, 0, -1));
    quad_coords->push_back(osg::Vec3d(1, 1, -1));
    quad_coords->push_back(osg::Vec3d(0, 1, -1));

    osg::ref_ptr<osg::Vec2Array> quad_tcoords = new osg::Vec2Array; // texture coords
    quad_tcoords->push_back(osg::Vec2(0, 0));
    quad_tcoords->push_back(osg::Vec2(TextureWidth_, 0));
    quad_tcoords->push_back(osg::Vec2(TextureWidth_, TextureHeight_));
    quad_tcoords->push_back(osg::Vec2(0, TextureHeight_));

    osg::ref_ptr<osg::Geometry> quad_geom = new osg::Geometry;
    osg::ref_ptr<osg::DrawArrays> quad_da = new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4);

    osg::ref_ptr<osg::Vec4Array> quad_colors = new osg::Vec4Array;
    quad_colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

    quad_geom->setVertexArray(quad_coords.get());
    quad_geom->setTexCoordArray(0, quad_tcoords.get());
    quad_geom->addPrimitiveSet(quad_da.get());
    quad_geom->setColorArray(quad_colors.get());
    quad_geom->setColorBinding(osg::Geometry::BIND_OVERALL);

    StateSet_ = quad_geom->getOrCreateStateSet();
    StateSet_->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
    StateSet_->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
    StateSet_->setTextureAttributeAndModes(0, InTexture_.get(), osg::StateAttribute::ON);
    
    StateSet_->addUniform(new osg::Uniform("textureID0", 0));

    quad_geode->addDrawable(quad_geom.get());
    
    top_group->addChild(quad_geode.get());

    return top_group;
}

void KeepHistoryPass::setupCamera()
{
    // clearing
    bool need_clear = false;
    if (need_clear) {
        Camera_->setClearColor(osg::Vec4(0.1f,0.1f,0.3f,1.0f));
        Camera_->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        Camera_->setClearMask(0);
        Camera_->setImplicitBufferAttachmentMask(0, 0);
    }

    // projection and view
    Camera_->setProjectionMatrixAsOrtho(0,1,0,1,-100,100);
    Camera_->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    Camera_->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    Camera_->setViewMatrix(osg::Matrix::identity());

    // viewport
    Camera_->setViewport(0, 0, TextureWidth_, TextureHeight_);

    Camera_->setRenderOrder(osg::Camera::PRE_RENDER);
    Camera_->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

	// attach the texture and use it as the color buffer.
	Camera_->attach(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER), OutTextures_[OverwriteIndex_].get());

    Camera_->setCullCallback(new CameraCullCallback(this));
    Camera_->setFinalDrawCallback(new CameraPostDrawCallback(this));
}

void KeepHistoryPass::createOutputTextures()
{
    for (int i=0; i<HistorySize_; ++i) {
        OutTextures_.push_back(new osg::TextureRectangle);
        OutTextures_[i]->setTextureSize(TextureWidth_, TextureHeight_);
        OutTextures_[i]->setInternalFormat(GL_RGB);
        //OutTextures_[i]->setInternalFormat(GL_LUMINANCE);
        //OutTextures_[i]->setSourceFormat(GL_LUMINANCE);
        OutTextures_[i]->setSourceType(GL_UNSIGNED_BYTE);
        OutTextures_[i]->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
        OutTextures_[i]->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);
    }
}

osg::ref_ptr<osg::TextureRectangle> KeepHistoryPass::getOutputTexture(int age) 
{
    // get to within valid range
    age = age % HistorySize_;

    // textures are bound before frame calls or rendering
    if (age > ValidHistory_) {
        age = ValidHistory_;
    }

    // the latest texture would be in OverwriteIndex_
    int t = (HistorySize_ + OverwriteIndex_ - age) % HistorySize_;

    return OutTextures_[t]; 
}

void KeepHistoryPass::setShader(std::string filename)
{
    FragmentProgram_ = 0;
    FragmentProgram_ = new osg::Program;

    osg::ref_ptr<osg::Shader> fshader = new osg::Shader( osg::Shader::FRAGMENT );
    if (filename != "") {
        fshader->loadShaderSourceFromFile(filename);
        FragmentProgram_->addShader(fshader.get());
    }

    StateSet_->setAttributeAndModes(FragmentProgram_.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE );
}

void KeepHistoryPass::frameDone()
{
    FirstPass_ = false;
    OverwriteIndex_++;
    OverwriteIndex_ = OverwriteIndex_ % HistorySize_;
    ValidHistory_++;
}