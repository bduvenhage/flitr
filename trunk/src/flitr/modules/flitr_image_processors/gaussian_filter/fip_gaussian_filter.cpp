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

#include <flitr/modules/flitr_image_processors/gaussian_filter/fip_gaussian_filter.h>


using namespace flitr;
using std::shared_ptr;

FIPGaussianFilter::FIPGaussianFilter(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                     uint32_t buffer_size) :
ImageProcessor(upStreamProducer, images_per_slot, buffer_size)
{
    
    //Setup image format being produced to downstream.
    for (uint32_t i=0; i<images_per_slot; i++) {
        ImageFormat downStreamFormat=upStreamProducer.getFormat();
        
        ImageFormat_.push_back(downStreamFormat);
    }
    
}

FIPGaussianFilter::~FIPGaussianFilter()
{
    delete [] xFiltData_;
}

bool FIPGaussianFilter::init()
{
    bool rValue=ImageProcessor::init();
    //Note: SharedImageBuffer of downstream producer is initialised with storage in ImageProcessor::init.
    
    size_t maxXFiltDataSize=0;
    
    for (uint32_t i=0; i<ImagesPerSlot_; i++)
    {
        const ImageFormat imFormat=getUpstreamFormat(i);
        
        const size_t width=imFormat.getWidth();
        const size_t height=imFormat.getHeight();
        
        const size_t componentsPerPixel=imFormat.getComponentsPerPixel();
        
        const size_t xFiltDataSize = width*height*componentsPerPixel;
        
        if (xFiltDataSize>maxXFiltDataSize)
        {
            maxXFiltDataSize=xFiltDataSize;
        }
    }
    
    //Allocate a buffer big enough for any of the image slots.
    xFiltData_=new float[maxXFiltDataSize];
    
    return rValue;
}

bool FIPGaussianFilter::trigger()
{
    if ((getNumReadSlotsAvailable())&&(getNumWriteSlotsAvailable()))
    {
        std::vector<Image**> imvRead=reserveReadSlot();
        
        std::vector<Image**> imvWrite=reserveWriteSlot();
        
        //Start stats measurement event.
        ProcessorStats_->tick();
        
        for (size_t imgNum=0; imgNum<ImagesPerSlot_; imgNum++)
        {
            Image const * const imReadUS = *(imvRead[imgNum]);
            Image * const imWriteDS = *(imvWrite[imgNum]);
            
            float const * const dataReadUS=(float const * const)imReadUS->data();
            float * const dataWriteDS=(float * const)imWriteDS->data();
            
            const ImageFormat imFormat=getDownstreamFormat(imgNum);//down stream and up stream formats are the same.
            
            const size_t width=imFormat.getWidth();
            const size_t height=imFormat.getHeight();
            const size_t widthMinus5=width-((size_t)5);
            const size_t heightMinus5=height-((size_t)5);
            
            size_t y=0;
            {
                {
                    for (y=0; y<height; y++)
                    {
                        const size_t lineOffset=y * width;
                        
                        for (size_t x=((size_t)5); x<widthMinus5; x++)
                        {
                            float xFiltValue=( dataReadUS[lineOffset + x] ) * (252.0f/1024.0f);
                            
                            xFiltValue+=( dataReadUS[lineOffset + x - 1] +
                                         dataReadUS[lineOffset + x + 1] ) * (210.0f/1024.0f);
                            
                            xFiltValue+=( dataReadUS[lineOffset + x - 2] +
                                         dataReadUS[lineOffset + x + 2] ) * (120.0f/1024.0f);
                            
                            xFiltValue+=( dataReadUS[lineOffset + x - 3] +
                                         dataReadUS[lineOffset + x + 3] ) * (45.0f/1024.0f);
                            
                            xFiltValue+=( dataReadUS[lineOffset + x - 4] +
                                         dataReadUS[lineOffset + x + 4] ) * (10.0f/1024.0f);
                            
                            xFiltValue+=( dataReadUS[lineOffset + x - 5] +
                                         dataReadUS[lineOffset + x + 5] ) * (1.0f/1024.0f);
                            
                            xFiltData_[lineOffset + x]=xFiltValue;
                        }
                    }
                }
            }
            
            {
                {
                    for (y=((size_t)5); y<heightMinus5; y++)
                    {
                        const size_t lineOffset=y * width;
                        
                        for (size_t x=((size_t)5); x<widthMinus5; x++)
                        {
                            float filtValue=( xFiltData_[lineOffset + x] ) * (252.0f/1024.0f);
                            
                            filtValue+=( xFiltData_[lineOffset + x - width]+
                                        xFiltData_[lineOffset + width + x] ) * (210.0f/1024.0f);
                            
                            filtValue+=( xFiltData_[lineOffset + x - (width<<1)]+
                                        xFiltData_[lineOffset + (width<<1) + x] ) * (120.0f/1024.0f);
                            
                            filtValue+=( xFiltData_[lineOffset + x - ((width<<1)+width)]+
                                        xFiltData_[lineOffset + ((width<<1)+width) + x] ) * (45.0f/1024.0f);
                            
                            filtValue+=( xFiltData_[lineOffset + x - (width<<2)]+
                                        xFiltData_[lineOffset + (width<<2) + x] ) * (10.0f/1024.0f);
                            
                            filtValue+=( xFiltData_[lineOffset + x - ((width<<2)+width)]+
                                        xFiltData_[lineOffset + ((width<<2)+width) + x] ) * (1.0f/1024.0f);
                            
                            dataWriteDS[lineOffset+x]=filtValue;
                        }
                    }
                }
            }
        }
        
        //Stop stats measurement event.
        ProcessorStats_->tock();
        
        releaseWriteSlot();
        releaseReadSlot();
        
        return true;
    }
    
    return false;
}
