/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg, 2012
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#ifndef _08b44aa8_37ee_4f7c_a6f6_9662a3301ef4
#define _08b44aa8_37ee_4f7c_a6f6_9662a3301ef4

#include "itkSymmetryPlaneCalculator.h"

#include <algorithm>
#include <cmath>

#include <itkBinaryThresholdImageFilter.h>
#include <itkHistogram.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkShrinkImageFilter.h>
#include <itkSobelEdgeDetectionImageFilter.h>

namespace itk
{

template<typename TInputImage, typename TCoordRep>
typename SymmetryPlaneCalculator<TInputImage, TCoordRep>::PlaneType const &
SymmetryPlaneCalculator<TInputImage, TCoordRep>
::GetPlane() const
{
    return this->m_Plane;
}

template<typename TInputImage, typename TCoordRep>
void
SymmetryPlaneCalculator<TInputImage, TCoordRep>
::Compute()
{
    this->m_Plane = this->get_best_plane(4, 2);
    this->m_Plane = this->get_best_plane(2, 1, this->m_Plane, 4);
    this->m_Plane = this->get_best_plane(1, 0.5, this->m_Plane, 4);
}

template<typename TInputImage, typename TCoordRep>
SymmetryPlaneCalculator<TInputImage, TCoordRep>
::SymmetryPlaneCalculator()
{
    this->m_Image = TInputImage::New();
}

template<typename TInputImage, typename TCoordRep>
void
SymmetryPlaneCalculator<TInputImage, TCoordRep>
::PrintSelf(std::ostream& os, Indent indent) const
{
    this->Superclass::PrintSelf(os, indent);
    os << indent << "Image: \n"; this->m_Image->Print(os, indent.GetNextIndent());
    os << indent << "Plane: \n"; this->m_Plane.Print(os, indent.GetNextIndent());
}

template<typename TInputImage, typename TCoordRep>
typename SymmetryPlaneCalculator<TInputImage, TCoordRep>::PlaneType
SymmetryPlaneCalculator<TInputImage, TCoordRep>
::get_best_plane(int sizeFactor, float voxelStep, 
                 PlaneType const & initial_plane, float range) const
{
    ImageConstPointer image;
    if(sizeFactor != 1)
    {
        // Don't run shrinker
        typedef ShrinkImageFilter<TInputImage, TInputImage> Shrinker;
        typename Shrinker::Pointer shrinker = Shrinker::New();
        shrinker->SetInput(this->GetImage());
        shrinker->SetShrinkFactors(sizeFactor);
        shrinker->Update();
        image = shrinker->GetOutput();
    }
    else
    {
        image = this->GetImage();
    }
    
    // Compute the edge image
    typedef SobelEdgeDetectionImageFilter<TInputImage, TInputImage> SobelFilter;
    typename SobelFilter::Pointer sobel_filter = SobelFilter::New();
    sobel_filter->SetInput(image);
    sobel_filter->Update();
    
    // Binarize the edge image using the 50% brightest voxels
    // Although the paper mentions the 5% brightest voxel, this does not yield
    // enough features in the edges.
    PixelType const threshold = get_histogram_quantile(sobel_filter->GetOutput(), 0.5);
    typedef Image<unsigned char, ImageType::ImageDimension> BinaryImageType;
    typedef BinaryThresholdImageFilter<ImageType, BinaryImageType> ThresholdFilter;
    typename ThresholdFilter::Pointer threshold_filter = ThresholdFilter::New();
    threshold_filter->SetInput(sobel_filter->GetOutput());
    threshold_filter->SetLowerThreshold(threshold);
    threshold_filter->SetInsideValue(1);
    threshold_filter->SetOutsideValue(0);
    threshold_filter->Update();
    typename BinaryImageType::Pointer binary_image = threshold_filter->GetOutput();
    
    typename ImageType::RegionType const region = binary_image->GetRequestedRegion();
    
    // Parameters of the main iterations:
    //   * indices on which to vary the plane
    //   * limits of the iterations
    int fixed_index_min, fixed_index_max;
    std::vector<float> v1_min(3, 0);
    std::vector<float> v1_max(3, 0);
    std::vector<float> v2_min(3, 0);
    std::vector<float> v2_max(3, 0);
    std::vector<float> v3_min(3, 0);
    std::vector<float> v3_max(3, 0);
    
    if(range != 0)
    {
        // We're using an initial plane and range
        
        // Convert the initial plane to voxel coordinates
        typename BinaryImageType::PointType p1, p2, p3;
        std::copy(initial_plane.GetP1().Begin(), initial_plane.GetP1().End(), p1.Begin());
        std::copy(initial_plane.GetP2().Begin(), initial_plane.GetP2().End(), p2.Begin());
        std::copy(initial_plane.GetP3().Begin(), initial_plane.GetP3().End(), p3.Begin());
                  
        typename BinaryImageType::IndexType i1, i2, i3;
        binary_image->TransformPhysicalPointToIndex(p1, i1);
        binary_image->TransformPhysicalPointToIndex(p2, i2);
        binary_image->TransformPhysicalPointToIndex(p3, i3);
        
        // Find the two indices in i1 which are equal to 0 (or very close)
        // Use the other one as the fixed index. Initialize the values to avoid
        // a compile-time warning.
        fixed_index_min = 0;
        fixed_index_max = 1;
        for(unsigned d1=0; d1<TInputImage::ImageDimension; ++d1)
        {
            bool largest=true;
            for(unsigned d2=0; d2<TInputImage::ImageDimension; ++d2)
            {
                if(std::abs(i1[d1])<std::abs(i1[d2]))
                {
                    largest = false;
                }
            }
            if(largest)
            {
                fixed_index_min = d1;
                fixed_index_max = d1+1;
            }
        }
        
        // Only compute metric on the given range
        for(int i=fixed_index_min; i<fixed_index_max; ++i)
        {
            v1_min[i] = i1[i]-range; 
            v1_max[i] = i1[i]+range;
            v2_min[i] = i2[i]-range; 
            v2_max[i] = i2[i]+range;
            v3_min[i] = i3[i]-range; 
            v3_max[i] = i3[i]+range;
        }
    }
    else
    {
        // No initial plane
        
        // Use all orientations
        fixed_index_min = 0;
        fixed_index_max = 3;
        
        // Compute metric on all planes
        for(int i=fixed_index_min; i<fixed_index_max; ++i)
        {
            v1_min[i] = region.GetIndex()[i]; 
            v1_max[i] = region.GetIndex()[i]+region.GetSize()[i];
            v2_min[i] = region.GetIndex()[i]; 
            v2_max[i] = region.GetIndex()[i]+region.GetSize()[i];
            v3_min[i] = region.GetIndex()[i]; 
            v3_max[i] = region.GetIndex()[i]+region.GetSize()[i];
        }
    }
    
    // Use voxel-based planes to avoid costly Index <-> Point computations
    PlaneType best_plane;
    float best_metric = 0;
    
    for(int fixed_index=fixed_index_min; fixed_index<fixed_index_max; ++fixed_index)
    {
        // OpenMP does not allow float variables for parallel loops, let's
        // pre-compute them
        std::vector<float> v3_values;
        for(float v3=v3_min[fixed_index]; v3<v3_max[fixed_index]; v3+=voxelStep)
        {
            v3_values.push_back(v3);
        }

        // Move first point
        for(float v1=v1_min[fixed_index]; v1<v1_max[fixed_index]; v1+=voxelStep)
        {
            typename BinaryImageType::PointType p1;
            p1[fixed_index] = v1;
            
            int index = (fixed_index+1)%3;
            p1[index] = region.GetIndex()[index];
            
            index = (fixed_index+2)%3;
            p1[index] = region.GetIndex()[index];
            
            // Move second point
            for(float v2=v2_min[fixed_index]; v2<v2_max[fixed_index]; v2+=voxelStep)
            {
                typename BinaryImageType::PointType p2;
                p2[fixed_index] = v2;
                
                int index = (fixed_index+1)%3;
                p2[index] = region.GetIndex()[index]+region.GetSize()[index]-1;
                
                index = (fixed_index+2)%3;
                p2[index] = region.GetIndex()[index];
                
                // Move third point, using the previously-computed array of 
                // values
                #pragma omp parallel for shared(best_plane, best_metric)
                for(long int i=0; i<v3_values.size(); ++i)
                {
                    typename BinaryImageType::PointType p3;
                    p3[fixed_index] = v3_values[i];
                    
                    int index = (fixed_index+1)%3;
                    p3[index] = region.GetIndex()[index]+region.GetSize()[index]-1;
                    
                    index = (fixed_index+2)%3;
                    p3[index] = region.GetIndex()[index]+region.GetSize()[index]-1;
                    
                    PlaneType const plane(p1, p2, p3);
                
                    float const metric = 
                        this->get_metric<BinaryImageType>(binary_image, plane, true);
                    
                    #pragma omp critical
                    if(metric > best_metric)
                    {
                        best_metric = metric;
                        best_plane = plane;
                    }                    
                }
            }
        }
    }
    
    // Transform plane from voxel space to physical space
    typename BinaryImageType::IndexType i1, i2, i3;
    std::copy(best_plane.GetP1().Begin(), best_plane.GetP1().End(), i1.m_Index);
    std::copy(best_plane.GetP2().Begin(), best_plane.GetP2().End(), i2.m_Index);
    std::copy(best_plane.GetP3().Begin(), best_plane.GetP3().End(), i3.m_Index);
              
    typename BinaryImageType::PointType p1, p2, p3;
    binary_image->TransformIndexToPhysicalPoint(i1, p1);
    binary_image->TransformIndexToPhysicalPoint(i2, p2);
    binary_image->TransformIndexToPhysicalPoint(i3, p3);
    return PlaneType(p1, p2, p3);
}

template<typename TInputImage, typename TCoordRep>
typename SymmetryPlaneCalculator<TInputImage, TCoordRep>::PixelType
SymmetryPlaneCalculator<TInputImage, TCoordRep>
::get_histogram_quantile(TInputImage * image, float quantile)
{
    // Compute the minimum and maximum of the image to initialize the histogram.
    typedef MinimumMaximumImageCalculator<ImageType> MinimumMaximumImageCalculator;
    typename MinimumMaximumImageCalculator::Pointer calculator = 
        MinimumMaximumImageCalculator::New();
    calculator->SetImage(image);
    calculator->Compute();
    
    // Initialize histogram.
    typedef Statistics::Histogram<> Histogram;
    Histogram::Pointer histogram = Histogram::New();
    {
        Histogram::SizeType size; size.Fill(256);
        Histogram::MeasurementVectorType min; min.Fill(calculator->GetMinimum());
        Histogram::MeasurementVectorType max; max.Fill(calculator->GetMaximum());
        histogram->Initialize(size, min, max);
    }
    // Compute histogram.
    typedef ImageRegionConstIterator<ImageType> ConstIterator;
    for(ConstIterator it(image, image->GetRequestedRegion()); !it.IsAtEnd(); ++it)
    {
        histogram->IncreaseFrequency(it.Get(), 1);
    }
    
    // Return the specified quantile
    return histogram->Quantile(0, quantile);
}

template<typename TInputImage, typename TCoordRep>
template<typename TMetricImage>
float
SymmetryPlaneCalculator<TInputImage, TCoordRep>
::get_metric(TMetricImage* image, PlaneType const & plane, bool optimize)
{
    // Values for the non-optimized metric.
    float product_sum = 0;
    float original_squares_sum = 0;
    float flipped_squares_sum = 0;
    
    // Values for the optimized metric.
    float optimized_metric_numerator=0;
    float optimized_metric_denominator=0;
    
    typedef ImageRegionConstIteratorWithIndex<TMetricImage> Iterator;
    for(Iterator it(image, image->GetRequestedRegion()); !it.IsAtEnd(); ++it)
    {
        if(optimize && it.Get() == 0)
        {
            continue;
        }
        
        // Cast the original index as point
        typename TMetricImage::IndexType const original_index = it.GetIndex();
        typename TMetricImage::PointType original_point;
        std::copy(
            original_index.m_Index, original_index.m_Index+original_index.GetIndexDimension(), 
            original_point.Begin());
        
        // Compute its reflection
        typename TMetricImage::PointType const flipped_point = plane.Reflect(original_point);
        
        // Cast the reflected point as index
        typename TMetricImage::IndexType flipped_index;
        std::copy(flipped_point.Begin(), flipped_point.End(),
                  flipped_index.m_Index);
        
        // Get the original and reflected values
        typename TMetricImage::PixelType const original_pixel = it.Get();
        typename TMetricImage::PixelType const flipped_pixel =
            (image->GetRequestedRegion().IsInside(flipped_index)?image->GetPixel(flipped_index):0);
        
        // Compute the metric
        if(optimize)
        {
            optimized_metric_numerator += flipped_pixel;
            ++optimized_metric_denominator;
        }
        else
        {
            product_sum += original_pixel*flipped_pixel;
            original_squares_sum += original_pixel*original_pixel;
            flipped_squares_sum += flipped_pixel*flipped_pixel;
        }
    }
    
    if(optimize)
    {
        return optimized_metric_numerator/optimized_metric_denominator;
    }
    else
    {
        return product_sum/std::sqrt(original_squares_sum*flipped_squares_sum);
    }
}

}

#endif // _08b44aa8_37ee_4f7c_a6f6_9662a3301ef4
