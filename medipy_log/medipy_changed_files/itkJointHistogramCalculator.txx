/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#ifndef _b985f048_3512_4b46_9a48_cdfa8e82084e
#define _b985f048_3512_4b46_9a48_cdfa8e82084e

#include "itkJointHistogramCalculator.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <itkContinuousIndex.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkVector.h>

#ifdef _WINDOWS
double rint( double x)
// Copyright (C) 2001 Tor M. Aamodt, University of Toronto
// Permisssion to use for all purposes commercial and otherwise granted.
// THIS MATERIAL IS PROVIDED "AS IS" WITHOUT WARRANTY, OR ANY CONDITION OR
// OTHER TERM OF ANY KIND INCLUDING, WITHOUT LIMITATION, ANY WARRANTY
// OF MERCHANTABILITY, SATISFACTORY QUALITY, OR FITNESS FOR A PARTICULAR
// PURPOSE.
{
    double diff;
    if( x > 0 ) {
        __int64 xint = (__int64) (x+0.5);
        if( xint % 2 ) {
            // then we might have an even number...
            diff = x - (double)xint;
            if( diff == -0.5 )
                return (double)(xint-1);
        }
        return (double)(xint);
    } else {
        __int64 xint = (__int64) (x-0.5);
        if( xint % 2 ) {
            // then we might have an even number...
            diff = x - (double)xint;
            if( diff == 0.5 )
                return (double)(xint+1);
        }
        return (double)(xint);
    }
}
#endif

namespace itk
{

template<typename TImage, typename TMask>
void
JointHistogramCalculator<TImage, TMask>
::Compute()
{
    this->InitializeHistogram();
    
    // Index is required for Linear Interpolation method
    typedef itk::ImageRegionConstIteratorWithIndex<ImageType> ImageIteratorType;
    typedef itk::ImageRegionConstIterator<MaskType> MaskIteratorType;
    
    ImageIteratorType it1(this->m_Image1, this->m_Image1->GetRequestedRegion());
    ImageIteratorType it2(this->m_Image2, this->m_Image2->GetRequestedRegion());
    MaskIteratorType mask_it;
    if(!this->m_Mask.IsNull())
    {
        mask_it = MaskIteratorType(this->m_Mask, this->m_Mask->GetRequestedRegion());
    }
    
    typedef void (Self::*Updater)(ImageIteratorType const &, ImageIteratorType const &);
    Updater updater=NULL;
    if(this->m_Method == Self::Method::NEAREST_NEIGHBOR)
    {
        updater = &Self::UpdateNearestNeighbor<ImageIteratorType>;
    }
    else if(this->m_Method == Self::Method::LINEAR_INTERPOLATION)
    {
        updater = &Self::UpdateLinearInterpolation<ImageIteratorType>;
    }

    while(!it1.IsAtEnd())
    {
        if(this->m_Mask.IsNull() || mask_it.Get() == this->m_MaskValue)
        {
            (this->*updater)(it1, it2);
        }
        // Otherwise do nothing: we are not in the mask
        
        // Update the image iterators and the mask iterator if available
        ++it1;
        ++it2;
        if(!this->m_Mask.IsNull())
        {
            ++mask_it;
        }
    }
}

template<typename TImage, typename TMask>
JointHistogramCalculator<TImage, TMask>
::JointHistogramCalculator()
{
    this->m_Image1 = ImageType::New();
    this->m_Image2 = ImageType::New();
    this->m_BinsCount1 = 100;
    this->m_BinsCount2 = 100;
    this->m_Mask = NULL;
    this->m_MaskValue = 1;
    this->m_Method = Self::Method::NEAREST_NEIGHBOR;
    this->m_Histogram = HistogramType::New();
}

template<typename TImage, typename TMask>
void
JointHistogramCalculator<TImage, TMask>
::PrintSelf(std::ostream & os, Indent indent) const
{
    Superclass::PrintSelf(os,indent);
    
    os << indent << "Image 1: \n";
    this->m_Image1->Print(os, indent.GetNextIndent());
    os << indent << "Image 2: \n";
    this->m_Image2->Print(os, indent.GetNextIndent());
    
    os << indent << "Number of bins: " 
       << this->m_BinsCount1 << " " << this->m_BinsCount2 << "\n";
    
    os << indent << "Mask: ";
    if(this->m_Mask.IsNull())
    {
        os << "NULL\n";
    }
    else
    {
        os << "\n";
        this->m_Mask->Print(os, indent.GetNextIndent());
    }
    
    os << indent << "Method: ";
    if(this->m_Method == Self::Method::NEAREST_NEIGHBOR)
    {
        os << "Nearest neighbor";
    }
    else if(this->m_Method == Self::Method::LINEAR_INTERPOLATION)
    {
        os << "Linear interpolation";
    }
    os << "\n";
}

template<typename TImage, typename TMask>
void 
JointHistogramCalculator<TImage, TMask>
::InitializeHistogram()
{
    typename HistogramType::SizeType size;
    size[0] = this->m_BinsCount1; size[1] = this->m_BinsCount2;
    
    typedef itk::ImageRegionConstIterator<ImageType> ImageIteratorType;
    typedef itk::ImageRegionConstIterator<MaskType> MaskIteratorType;
    
    ImageIteratorType it1(this->m_Image1, this->m_Image1->GetRequestedRegion());
    ImageIteratorType it2(this->m_Image2, this->m_Image2->GetRequestedRegion());
    MaskIteratorType mask_it;
    if(!this->m_Mask.IsNull())
    {
        mask_it = MaskIteratorType(this->m_Mask, this->m_Mask->GetRequestedRegion());
    }
    
    typedef typename HistogramType::MeasurementType MeasurementType;
    typedef typename HistogramType::MeasurementVectorType MeasurementVectorType;
    
    MeasurementVectorType lower_bound;
    lower_bound[0] = it1.Get(); lower_bound[1] = it2.Get();
    
    MeasurementVectorType upper_bound;
    upper_bound[0] = it1.Get(); upper_bound[1] = it2.Get();
    
    while(!it1.IsAtEnd())
    {
        if(this->m_Mask.IsNull() || mask_it.Get() == this->m_MaskValue)
        {
            lower_bound[0] = std::min<MeasurementType>(lower_bound[0], it1.Get());
            lower_bound[1] = std::min<MeasurementType>(lower_bound[1], it2.Get());
            
            upper_bound[0] = std::max<MeasurementType>(upper_bound[0], it1.Get());
            upper_bound[1] = std::max<MeasurementType>(upper_bound[1], it2.Get());
        }
        // Otherwise do nothing: we are not in the mask
        
        // Update the image iterators and the mask iterator if available
        ++it1;
        ++it2;
        if(!this->m_Mask.IsNull())
        {
            ++mask_it;
        }
    }
    
    this->m_Histogram->Initialize(size, lower_bound, upper_bound);
    this->m_Histogram->SetClipBinsAtEnds(false);
}

template<typename TImage, typename TMask>
template<typename TIterator>
void 
JointHistogramCalculator<TImage, TMask>
::UpdateNearestNeighbor(TIterator const & it1, TIterator const & it2)
{
    typename HistogramType::MeasurementVectorType measurement;
    measurement[0] = it1.Get(); measurement[1] = it2.Get();
    this->m_Histogram->IncreaseFrequency(measurement, 1);
}

template<typename TImage, typename TMask>
template<typename TIterator>
void 
JointHistogramCalculator<TImage, TMask>
::UpdateLinearInterpolation(TIterator const & it1, TIterator const & it2)
{
    typename HistogramType::MeasurementVectorType m1;
    m1[0] = it1.Get(); m1[1] = it2.Get();
    
    for(unsigned int d=0; d<ImageType::GetImageDimension(); ++d)
    {
        typename ImageType::OffsetType offset;
        offset.Fill(0); offset[d] = 1;
        
        typename ImageType::IndexType const neighbor = it1.GetIndex()+offset;
        if(!it1.GetRegion().IsInside(neighbor) || 
           (!this->m_Mask.IsNull() && this->m_Mask->GetPixel(neighbor) != this->m_MaskValue))
        {
            // this pixel will not contribute to the histogram
            continue;
        }
        
        typename HistogramType::MeasurementVectorType m2;
        m2[0] = this->m_Image1->GetPixel(neighbor); 
        m2[1] = this->m_Image2->GetPixel(neighbor);
        
        // Draw a line from m1 to m2
        typename HistogramType::IndexType index;
        
        itk::ContinuousIndex<float, 2> begin;
        index = this->m_Histogram->GetIndex(m1);
        std::copy(index.begin(), index.end(), begin.Begin());
        
        itk::ContinuousIndex<float, 2> end;
        index = this->m_Histogram->GetIndex(m2);
        std::copy(index.begin(), index.end(), end.Begin());
        
        itk::Vector<float, 2> step = end-begin;
        int const direction = (std::abs(step[0])>std::abs(step[1]))?0:1;
        step /= std::abs(step[direction]);
        
        bool const increment = step[direction]>0;
        
        unsigned int line_size = 0;
        for(itk::ContinuousIndex<float, 2> p(begin); 
            increment ? p[direction]<=end[direction] : p[direction]>=end[direction]; 
            p += step)
        {
            ++line_size;
        }
        
        float const increase = 1.0/(line_size);
        
        for(itk::ContinuousIndex<float, 2> p(begin); 
            increment ? p[direction]<=end[direction] : p[direction]>=end[direction]; 
            p += step)
        {
            typename HistogramType::IndexType index;
            index[0] = rint(p[0]); index[1] = rint(p[1]);
            this->m_Histogram->IncreaseFrequency(index, increase);
        }
    }
}

}

#endif // _b985f048_3512_4b46_9a48_cdfa8e82084e
