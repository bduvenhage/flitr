#include "flitr_video_hub.h"

#include <flitr/multi_osg_consumer.h>
#include <flitr/textured_quad.h>
#include <flitr/manipulator_utils.h>
#include <flitr/ortho_texture_manipulator.h>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <iostream>
#include <string>



int main(int argc, char *argv[])
{
    flitr::VideoHub videoHub;
    
    //videoHub.createV4LProducer("input", "/dev/video0");
    //videoHub.createRTSPProducer("input", "rtsp://192.168.0.90:554/axis-media/media.amp");//PC set to 192.168.0.100
    videoHub.createVideoFileProducer("input", "/Users/bduvenhage/Desktop/nikon_compressed.mp4");
    
    videoHub.createImageStabProcess("istab", "input", 0.9925);
    videoHub.createMotionDetectProcess("imotion", "istab", true, true);
    
    //videoHub.createVideoFileConsumer("video_output", "istab", "/Users/bduvenhage/Desktop/output.avi", 20);
    videoHub.createVideoFileConsumer("video_output", "imotion", "/Users/bduvenhage/Desktop/output.avi", 20);
    
    videoHub.createImageBufferConsumer("image_output", "imotion");
    const flitr::VideoHubImageFormat imageBufferFormat=videoHub.getImageFormat("imotion");
    uint8_t * const imageBuffer=new uint8_t[imageBufferFormat._width * imageBufferFormat._height *  imageBufferFormat._bytesPerPixel];
    videoHub.imageBufferConsumerSetBuffer("image_output", imageBuffer);
    
    //videoHub.createWebRTCConsumer("webrtc_output", "istab", "webrtc.fifo");
    videoHub.createWebRTCConsumer("webrtc_output", "imotion", "webrtc.fifo");
    
    
    
    //=============================//
    //=== OSG and Viewer things ===//
    //std::shared_ptr<flitr::MultiOSGConsumer> osgc(new flitr::MultiOSGConsumer(*(videoHub.getProducer("input")), 1, 1));
    //std::shared_ptr<flitr::MultiOSGConsumer> osgc(new flitr::MultiOSGConsumer(*(videoHub.getProducer("istab")), 1, 1));
    std::shared_ptr<flitr::MultiOSGConsumer> osgc(new flitr::MultiOSGConsumer(*(videoHub.getProducer("imotion")), 1, 1));
    if (!osgc->init()) {
        std::cerr << "Could not init OSG consumer\n";
        exit(-1);
    }
    
    osg::Group *root_node = new osg::Group;
    
    std::shared_ptr<flitr::TexturedQuad> quad(new flitr::TexturedQuad(osgc->getOutputTexture()));
    root_node->addChild(quad->getRoot().get());
    {
        osg::Matrix translate;
        translate.makeTranslate(osg::Vec3d(0.0, //left/right
                                           0.0, //up/down
                                           0.0));
        quad->setTransform(translate);
    }
    
    osgViewer::Viewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.setSceneData(root_node);
    viewer.setUpViewInWindow(100, 100, 640, 480);
    viewer.realize();
    
    flitr::OrthoTextureManipulator* om = new flitr::OrthoTextureManipulator(osgc->getOutputTexture()->getTextureWidth(), osgc->getOutputTexture()->getTextureHeight());
    viewer.setCameraManipulator(om);
    //=============================//
    //=============================//
    
    
    while(!viewer.done())
    {
        //This trigger is ignores for producers that does not require a trigger.
        videoHub.getProducer("input")->trigger();
        
        if (osgc->getNext())
        {
            viewer.frame();
        }
        
        videoHub.imageBufferConsumerHold("image_output", true);
        for (int x=0; x<imageBufferFormat._width; ++x)
        {
            std::cout << int(imageBuffer[(100*imageBufferFormat._width*3) + x*3 + 0]) << " ";
        }
        std::cout.flush();
        videoHub.imageBufferConsumerHold("image_output", false);
        
        OpenThreads::Thread::microSleep(1000);
    }
    
    
    videoHub.stopAllThreads();
    videoHub.cleanup();
    
    delete [] imageBuffer;
    
    return 0;
}
