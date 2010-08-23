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

#ifndef MULTI_OSG_CONSUMER_H
#define MULTI_OSG_CONSUMER_H 1

#include <boost/tr1/memory.hpp>

#include <OpenThreads/Thread>
#include <OpenThreads/Mutex>

#include <osg/Image>
#include <osg/TextureRectangle>

#include <flitr/image_consumer.h>

namespace flitr {

class MultiOSGConsumer;

/// Thread to discard images if a client cannot read fast enough. This
/// prevents overflow in the producer by discarding all images except
/// the latest one. 
class MultiOSGConsumerThread : public OpenThreads::Thread {
  public: 
    MultiOSGConsumerThread(MultiOSGConsumer *consumer) :
        Consumer_(consumer),
        ShouldExit_(false) {}
    void run();
    void setExit() { ShouldExit_ = true; }
  private:
    MultiOSGConsumer *Consumer_;
    bool ShouldExit_;
};

/**
 * A consumer that takes images from a producer (possibly with many
 * streams) and creates images and textures for consumption in OSG.
 * 
 */
class MultiOSGConsumer : public ImageConsumer {
    friend class MultiOSGConsumerThread;
  public:
    /** 
     * Creates an image consumer that passes data to OSG. Format
     * is determined from the producer.
     * 
     * \param producer The producer we must connect to.
     * \param images_per_slot How many images we expect per
     * slot. Can be less than what the producer provides.
     * \param images_in_history How many images are kept in a read
     * busy state so that they can be used in further
     * processing. The producer must have more slots in its buffer
     * than the history requested here.
     * 
     */
    MultiOSGConsumer(ImageProducer& producer, 
                     uint32_t images_per_slot, 
                     uint32_t images_in_history = 1);
		
    ~MultiOSGConsumer();
    /** 
     * The init method is used after construction to be able to return
     * success or failure of initialisation.
     * 
     * \return True if successful.
     */
    bool init();
    
    /** 
     * Creates a background thread that discards images if getNext()
     * cannot be called fast enough. E.g. when OSG processing rate is
     * lower than the producer rate.
     * 
     */
    void startDiscardThread();
    
    /** 
     * Gets access to an osg::Image in the buffer.
     * 
     * \note Images are reused to avoid uploading too much data, so
     * the history parameter is valid only after the most recent
     * getNext() call.
     *
     * \param im_number The specific source in the selected slot.
     * \param im_age How far in history to select a slot. 0 is the latest.
     * 
     * \return Pointer to an osg::Image.
     */
    osg::Image* getOSGImage(uint32_t im_number = 0, uint32_t im_age = 0);

    /** 
     * Gets access to a texture that will contain updated
     * images. The textures are automatically updated as new
     * images are read. In other words, the texture with age 0
     * will always point to the latest image.
     * 
     * \param tex_number The specific source or stream in the selected slot.
     * \param tex_age How far in history to select a slot. 0 is the latest.
     * 
     * \return Pointer to an osg::TextureRectangle.
     */
    osg::TextureRectangle* getOutputTexture(uint32_t tex_number = 0, uint32_t tex_age = 0);

    /** 
     * Update all textures if new images are available from the producer. A single update is performed. E.g. a getNext() should follow each update or trigger of the producer.
     * 
     * \todo make async version
     *
     * \return Whether a new image (or set from a slot) was obtained.
     */
    bool getNext();
		
  private:
    /// A copy of the producer's formats.
    std::vector<ImageFormat> ImageFormat_;
    /// How many images per slot we expect.
    uint32_t ImagesPerSlot_;
    /// How many images/textures to keep in a read busy state.
    uint32_t ImagesInHistory_;
    /// Where in the list of history images to update next.
    uint32_t HistoryWritePos_;
    
    /// Buffer of OSG images that just wrap our producer's data.
    std::vector< std::vector< osg::ref_ptr <osg::Image> > > OSGImages_;
    /// Textures that can be used in OSG.
    std::vector< std::vector< osg::ref_ptr <osg::TextureRectangle> > > OutputTextures_;
    /// Images to be used at initialisation for textures.
    std::vector< osg::ref_ptr<osg::Image> > DummyImages_;

    /// Synchronise access the shared buffer when we have a background
    /// thread.
    OpenThreads::Mutex BufferMutex_;
    /// Background thread to discard images if getNext() cannot be
    /// called fast enough.
    std::tr1::shared_ptr<MultiOSGConsumerThread> Thread_;
};

}

#endif //MULTI_OSG_CONSUMER_H