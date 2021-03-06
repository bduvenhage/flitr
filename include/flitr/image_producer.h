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

#ifndef IMAGE_PRODUCER_H
#define IMAGE_PRODUCER_H 1

#include <flitr/flitr_export.h>

namespace flitr {
//Prototype of class defined in this file in case of circular includes.		
  class ImageProducer;
}

#include <flitr/modules/parameters/parameters.h>
#include <flitr/image.h>
#include <flitr/shared_image_buffer.h>

namespace flitr {

/**
 * The base class for image producers.
 * 
 * The ImageProducer class is the base for all image producers. It
 * contains the buffer used for storing or referring to images and
 * places images it creates into this buffer.
 *
 */
class FLITR_EXPORT ImageProducer : virtual public Parameters {
    friend class SharedImageBuffer;
    friend class ImageConsumer;
  public:
    ImageProducer() {}
    virtual ~ImageProducer() {}

    virtual bool init() = 0;
    
    /** 
     * Obtain the format of an image produced into a specific index of
     * a slot in the buffer.
     * 
     * \param index Index in the slot. From 0 to one less than images
     * in a slot.
     * 
     * \return Format of the image.
     */
    virtual ImageFormat getFormat(const uint32_t index = 0) const { return ImageFormat_[index]; }

    /**
     * Obtain the number of write slots that can be reserved.
     *
     * \return The number of available write slots.
     */
    virtual uint32_t getNumWriteSlotsAvailable() const
    {
        return SharedImageBuffer_->getNumWriteSlotsAvailable();
    }

    /**
     * Obtain the number of write slots that this producer has reserved for
     * writing but has not yet released.
     *
     * \return The number of slots reserved for writing.
     */
    virtual uint32_t getNumWriteSlotsReserved()
    {
        return SharedImageBuffer_->getNumWriteSlotsReserved();
    }

    /** 
     * Typically implemented to tell an asynchronous producer to create an image.
     * 
     * \return Whether an image was produced.
     */
    virtual bool trigger() { return false; }
	
    /** 
     * Implemented for producers where a specific image at a location
     * can be requested.
     * 
     * \param position The position in the file/stream to obtain.
     * 
     * \return Whether an image was produced.
     */
    virtual bool seek(uint32_t position) { (void)position; return false; }

    /** 
     * Find the least number of readable image slots available between
     * all consumers.
     * 
     * \return Least number of read slots available.
     */
    virtual uint32_t getLeastNumReadSlotsAvailable()
    {
        return SharedImageBuffer_->getLeastNumReadSlotsAvailable();
    }

    //Examples:
    // 1) producer->setCreateMetadataFunction(std::tr1::bind(&CustomMetadataClass::create, customMetadataObject))
    // 2) multiPlaybackFusionTextureCaptureProducer_->setCreateMetadataFunction(std::tr1::bind(&MultiOSGConsumerMetadataCreator::getCurrentMetadata, MultiOSGConsumerMetadataCreator(multiPlaybackOSGConsumer_.get(), 0, 0)));
    // 3) multiPlaybackFusionTextureCaptureProducer_->setCreateMetadataFunction(std::tr1::bind(&MultiOSGConsumer::getImageMetadata, multiPlaybackOSGConsumer_, (uint32_t)0, (uint32_t)0));
    virtual void setCreateMetadataFunction(CreateMetadataFunction f)
    {
        CreateMetadataFunction_ = f;
    }

  protected:
    /** 
     * Called when all consumers are done with the oldest available
     * slot. This can be used when we are reading from another
     * producer/producers and must release a read slot.
     * 
     */
    virtual void releaseReadSlotCallback() {}
    // notifyRead

    /** 
     * Called when a new consumer is added.
     * 
     * \param consumer Consumer that was added.
     * 
     * \return Whether the add was successful.
     */
    virtual bool addConsumer(ImageConsumer& consumer)
    {
        return SharedImageBuffer_->addConsumer(consumer);
    }

    /**
     * Called when a consumer must be removed.
     *
     * \param consumer Consumer that was previously added that must be removed.
     *
     * \return Whether the remove was successful.
     */
    virtual bool removeConsumer(ImageConsumer& consumer)
    {
        return SharedImageBuffer_->removeConsumer(consumer);
    }
    
    /** 
     * Reserve (obtain) a slot for writing new image data. Multiple
     * slots can be reserved before any are released.
     * 
     * \return A vector with pointers to image pointers where data can
     * be written. The size would match the number of images per slot
     * for this buffer. An empty vector if no slot could be obtained
     * (buffer full or readers busy with all slots). 
     */
    //virtual std::vector<Image**> getWritable() {
    virtual std::vector<Image**> reserveWriteSlot() {
        return SharedImageBuffer_->reserveWriteSlot();
    }

    /** 
     * Indicate that the producer has finished with a reserved write
     * slot. Should only be called after a slot has been reserved and
     * once for each slot that was reserved.
     * 
     */
    //virtual void pushWritable() {
    virtual void releaseWriteSlot() {
        SharedImageBuffer_->releaseWriteSlot();
    }

    /// A vector of the formats of the images is produced per slot.
    std::vector<ImageFormat> ImageFormat_;
    
    /// The shared buffer used for this producer and all consumers.
    std::shared_ptr<SharedImageBuffer> SharedImageBuffer_;

    /// An optional function that gets called as soon as an image is
    /// produced. Used to create the image metadata.
    CreateMetadataFunction CreateMetadataFunction_; 
};

}

#endif // IMAGE_PRODUCER_H
