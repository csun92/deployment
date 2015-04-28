/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#ifndef _4d8e7051_97fc_45c3_9843_2cec9493448b
#define _4d8e7051_97fc_45c3_9843_2cec9493448b

#include <ostream>
#include <vector>

#include <itkSmartPointer.h>
#include <vnl/vnl_matrix.h>

#include "itkDWIStatisticsImageFilter.h"
#undef GetBValue 

namespace itk
{

/**
 * @brief Estimate the mean and standard deviation of a DTI image using a 
 *        bootstrap method.
 *
 * This filter operates on DWI data: TInputImage must be a scalar image, and
 * the filter has one input per gradient direction.
 *  
 */
template<typename TInputImage, typename TMeanImage=itk::VectorImage<
        typename TInputImage::PixelType, TInputImage::ImageDimension>,
    typename TStandardDeviationImage=TInputImage,
    typename TMaskImage=TInputImage>
class BootstrapDWIStatisticsImageFilter : public DWIStatisticsImageFilter<
    TInputImage, TMeanImage, TStandardDeviationImage, TMaskImage>
{
public :
    /// @brief Standard class typedefs.
    typedef BootstrapDWIStatisticsImageFilter Self;
    typedef DWIStatisticsImageFilter<
        TInputImage, TMeanImage, TStandardDeviationImage, TMaskImage> Superclass;
    typedef SmartPointer<Self> Pointer;
    typedef SmartPointer<Self const> ConstPointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    typedef typename Superclass::MeanImageType MeanImageType;
    typedef typename Superclass::MeanImagePointer MeanImagePointer;
    
    typedef typename Superclass::StandardDeviationImageType StandardDeviationImageType;
    typedef typename Superclass::StandardDeviationImagePointer StandardDeviationImagePointer;
    
    typedef typename Superclass::MaskImageType MaskImageType;
    typedef typename Superclass::MaskImagePointer MaskImagePointer;
    typedef typename Superclass::MaskImageConstPointer MaskImageConstPointer;

    /// @brief Run-time type information (and related methods).
    itkTypeMacro(BootstrapDWIStatisticsImageFilter, DWIStatisticsImageFilter);

    typedef float BValueType;
    typedef typename TInputImage::PointType DirectionType; 
    typedef vnl_matrix<float> TensorType; 

    itkGetConstMacro(BValue, BValueType);
    itkSetMacro(BValue, BValueType);
    
    DirectionType GetGradientDirection(unsigned int i);
    void SetGradientDirection(unsigned int i, DirectionType bvec);
    
    itkGetConstMacro(SamplesCount, unsigned int);
    itkSetMacro(SamplesCount, unsigned int);
    
    itkGetConstMacro(UseSpatialBootstrap, bool);
    itkSetMacro(UseSpatialBootstrap, bool);
    itkBooleanMacro(UseSpatialBootstrap);

    /// @brief Return the size of the neighborhood in the plane direction.
    itkGetConstMacro(SizePlane, unsigned int);
    
    /// @brief Set the size of the neighborhood in the plane direction, default to 3.
    itkSetMacro(SizePlane, unsigned int);
    
    /// @brief Return the size of the neighborhood in the normal direction.
    itkGetConstMacro(SizeDepth, unsigned int);
    
    /// @brief Set the size of the neighborhood in the normal direction, default to 3.
    itkSetMacro(SizeDepth, unsigned int);

protected :
    typedef typename TStandardDeviationImage::RegionType OutputImageRegionType;
    typedef typename MeanImageType::IndexType MeanImageIndexType;
    typedef vnl_matrix<float> BMatrixType; 
    
    typedef vnl_matrix<float> SignalType;
    typedef std::vector<SignalType>(Self::*BootstrapInitializer)(
        MeanImageIndexType idx);
    typedef std::vector<TensorType> (Self::*BootstrapGenerator)(
        std::vector<SignalType> signals);
    
    BootstrapDWIStatisticsImageFilter();
    ~BootstrapDWIStatisticsImageFilter() {}
    void PrintSelf(std::ostream& os, Indent indent) const;
    void BeforeThreadedGenerateData();
    void ThreadedGenerateData(
        OutputImageRegionType const & outputRegionForThread, ThreadIdType);
    
    std::vector<unsigned int> Random(unsigned int size, unsigned int max_value) const;
    
    std::vector<SignalType> SpatialBootstrapInit(MeanImageIndexType idx);
    std::vector< TensorType > SpatialBootstrapGenerator(
        std::vector<SignalType> signals);

    std::vector<SignalType> LocalBootstrapInit(MeanImageIndexType idx);
    std::vector< TensorType > LocalBootstrapGenerator(
        std::vector<SignalType> signals);

    void ComputeParameters(
        std::vector<TensorType> const & tensors, MeanImageIndexType idx);

private :
    BValueType m_BValue;
    
    unsigned int m_SizePlane;
    unsigned int m_SizeDepth;
    
    unsigned int m_SamplesCount;
    bool m_UseSpatialBootstrap;
    
    std::vector<DirectionType> directions;
    
    BMatrixType bmatrix;
    BMatrixType invbmatrix;
    
    BootstrapDWIStatisticsImageFilter(const Self &); //purposely not implemented
    void operator=(const Self &);  //purposely not implemented
    
};

}

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkBootstrapDWIStatisticsImageFilter.txx"
#endif


#endif // _4d8e7051_97fc_45c3_9843_2cec9493448b
