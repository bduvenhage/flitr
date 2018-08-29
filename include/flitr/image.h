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

#ifndef FLITR_IMAGE_H
#define FLITR_IMAGE_H 1

#include <flitr/image_format.h>
#include <flitr/image_metadata.h>
#include <flitr/log_message.h>

extern "C" {
#include <libavformat/avformat.h>
}

#undef PixelFormat


#include <cstdlib>

namespace flitr {

class Image {
  public:
    /*! Constructor for image from an image format
     *  @param image_format The image format of the image to allocate.
     *  @param zero_mem Flag to control zero-ing of memory once allocated.
     */
    Image(const ImageFormat& image_format, const bool zero_mem = false) :
        Format_(image_format)
    {
        Data_ = (uint8_t*)av_malloc(Format_.getBytesPerImage());
        if (!Data_)
        {
            outOfMem();
        } else
        {
            if (zero_mem)
            {
                memset(Data_, 0, Format_.getBytesPerImage());
            }
        }
    };
    
    ~Image()
    {
        av_free(Data_);
    }
    
    //! Copy constructor
    Image(const Image& rh)
    {
        Data_ = (uint8_t*)av_malloc(rh.Format_.getBytesPerImage());
        if (!Data_)
        {
            outOfMem();
        }
        deepCopy(rh);
    }
    
    //! Assignment operator
    Image& operator=(const Image& rh)
    {
        if (this == &rh)
        {
            return *this;
        }
       
        uint8_t* new_data = (uint8_t*)av_malloc(rh.Format_.getBytesPerImage());
        
        if (new_data != 0)
        {
            av_free(Data_);
            Data_ = new_data;
        } else
        {
            outOfMem();
        }

        deepCopy(rh);
        return *this;
    }
    
    //!Get a const pointer to the image format.
    ImageFormat const * format() const { return &Format_; }

    //!Get a non-const pointer to the image format. (Legacy method - included for older code that didn't use a const pointer to hold the return.)
    ImageFormat * format() { return &Format_; }
    
    //!Get the image mata data.
    const std::shared_ptr<ImageMetadata> metadata() const { return Metadata_; }
    
    //!Set the image mata data.
    void setMetadata(std::shared_ptr<ImageMetadata> md) { Metadata_ = md; }

    //!Get a pointer to the image data for reading and writing.
    uint8_t * data() { return &(Data_[0]); }
    
    //!Get a pointer to const image data for reading.
    uint8_t const * data() const { return &(Data_[0]); }

  private:
    void deepCopy(const Image& rh)
    {
        Format_ = rh.Format_;
        if (rh.Metadata_)
        {
            // make a copy
            std::shared_ptr<ImageMetadata> new_meta(rh.Metadata_->clone());
            Metadata_.swap(new_meta);
        } else
        {
            // delete ours too
            std::shared_ptr<ImageMetadata> new_meta;
            Metadata_.swap(new_meta);
        }
        // assume allocation was done
        memcpy(Data_, rh.Data_, Format_.getBytesPerImage());
    }
    
    void outOfMem()
    {
        logMessage(LOG_CRITICAL) << "Out of memory in Image.\n";
        abort();
    }

    ImageFormat Format_;
    std::shared_ptr<ImageMetadata> Metadata_;
    uint8_t* Data_;
};

}

#endif //FLITR_IMAGE_H
