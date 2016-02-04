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

#ifndef FIP_PHOTOMETRIC_EQUALISE_H
#define FIP_PHOTOMETRIC_EQUALISE_H 1

#include <flitr/image_processor.h>
#include <flitr/image_processor_utils.h>

namespace flitr {
    
    /*! Applies photometric equalisation to the image stream. */
    class FLITR_EXPORT FIPPhotometricEqualise : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param targetAverage The average image brightness the image is transformed to.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPPhotometricEqualise(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                               float targetAverage,
                               uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPPhotometricEqualise();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
    private:
        
        //!Template method that does the equalisation. Pixel format agnostic, but pixel data type templated.
        template<typename T>
        void process(T * const dataWrite, T const * const dataRead,
                     const size_t componentsPerLine, const size_t height)
        {
            
            //Could be line parallel!
            for (size_t y=0; y<height; y++)
            {
                const size_t lineOffset=y * componentsPerLine;
                
                double *lineSum = lineSumArray_ + y;
                *lineSum=0.0;
                
                for (size_t compNum=0; compNum<componentsPerLine; compNum++)
                {
                    (*lineSum)+=dataRead[lineOffset + compNum];
                }
            }
            
            
            double imageSum=0.0;
            
            //Single threaded!
            for (size_t y=0; y<height; y++)
            {
                imageSum+=lineSumArray_[y];
            }
            
            
            const size_t componentsPerImage=componentsPerLine*height;
            const float average = imageSum / ((double)componentsPerImage);
            const float eScale=targetAverage_ / average;
            
            
            //Could be pixel parallel.
            for (size_t y=0; y<height; ++y)
            {
                const size_t lineOffset=y * componentsPerLine;
                
                for (size_t compNum=0; compNum<componentsPerLine; ++compNum)
                {
                    dataWrite[lineOffset + compNum]=dataRead[lineOffset + compNum] * eScale;
                }
            }
        }
        
        const float targetAverage_;
        
        /*! The sum per line in an image. */
        double * lineSumArray_;
    };
    
    
    /*! Applies LOCAL photometric equalisation to the image stream. */
    class FLITR_EXPORT FIPLocalPhotometricEqualise : public ImageProcessor
    {
    public:
        
        /*! Constructor given the upstream producer.
         *@param upStreamProducer The upstream image producer.
         *@param images_per_slot The number of images per image slot from the upstream producer.
         *@param targetAverage The average image brightness the image is transformed to.
         *@param windowSize The local window size taken into account during equalisation of a pixel.
         *@param buffer_size The size of the shared image buffer of the downstream producer.*/
        FIPLocalPhotometricEqualise(ImageProducer& upStreamProducer, uint32_t images_per_slot,
                                    const float targetAverage, const size_t windowSize,
                                    uint32_t buffer_size=FLITR_DEFAULT_SHARED_BUFFER_NUM_SLOTS);
        
        /*! Virtual destructor */
        virtual ~FIPLocalPhotometricEqualise();
        
        /*! Method to initialise the object.
         *@return Boolean result flag. True indicates successful initialisation.*/
        virtual bool init();
        
        /*!Synchronous trigger method. Called automatically by the trigger thread in ImageProcessor base class if started.
         *@sa ImageProcessor::startTriggerThread*/
        virtual bool trigger();
        
    private:
        
        const float targetAverage_;
        const size_t windowSize_;
        
        IntegralImage integralImage_;
        double *integralImageData_;
    };
    
}

#endif //FIP_PHOTOMETRIC_EQUALISE_H
