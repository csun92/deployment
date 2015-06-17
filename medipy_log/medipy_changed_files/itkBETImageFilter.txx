/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg, 2011-2012
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#ifndef components_segmentation_betimagefilter_txx
#define components_segmentation_betimagefilter_txx

#include "itkBETImageFilter.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>
#include <vector>

#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkScalarImageToHistogramGenerator.h>

#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_vector_fixed_ref.h>

#include <vtkCellArray.h>
#include <vtkDataArray.h>
#include <vtkIdList.h>
#include <vtkLinearSubdivisionFilter.h>
#include <vtkPlatonicSolidSource.h>
#include <vtkPointData.h>
#include <vtkPolyDataNormals.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#ifdef _WINDOWS
#define M_PI       3.14159265358979323846
#endif

namespace itk
{

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::SetT2(InputImagePixelType _arg)
{
    itkDebugMacro("Setting T2 to " << _arg);
    if(this->m_T2 != _arg)
    {
        this->m_T2 = _arg;
        this->m_t = this->m_T2+0.1*(this->m_T98-this->m_T2);
        this->Modified();
    }
    this->are_thresholds_specified_ = true;
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::SetT98(InputImagePixelType _arg)
{
    itkDebugMacro("Setting T98 to " << _arg);
    if(this->m_T98 != _arg)
    {
        this->m_T98 = _arg;
        this->m_t = this->m_T2+0.1*(this->m_T98-this->m_T2);
        itkDebugMacro("Setting t to " << this->m_t);
        this->Modified();
    }
    this->are_thresholds_specified_ = true;
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::EstimateThresholds()
{
    typedef Statistics::ScalarImageToHistogramGenerator<InputImageType> HistogramGenerator;
    unsigned long bins = 256;

    typename HistogramGenerator::Pointer histogramGenerator = HistogramGenerator::New();
    histogramGenerator->SetInput(this->GetInput());
    histogramGenerator->SetNumberOfBins(bins);
    histogramGenerator->SetMarginalScale(10.0);
    histogramGenerator->Compute();
    typedef typename HistogramGenerator::HistogramType Histogram;
    typename Histogram::ConstPointer histogram = histogramGenerator->GetOutput();

    this->SetT2(histogram->Quantile(0, 0.2));
    this->SetT98(histogram->Quantile(0, 0.98));
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::SetCenterOfGravity(typename TInputImage::IndexType & _arg)
{
    itkDebugMacro("setting CenterOfGravity to " << _arg);
    if(this->m_CenterOfGravity != _arg)
    {
        this->m_CenterOfGravity = _arg;
        this->Modified();
    }
    this->is_cog_specified_ = true;
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::EstimateCenterOfGravity()
{
    float totalMass = 0;

    vnl_vector_fixed<float, TInputImage::ImageDimension> cog(0.);

    ImageRegionConstIteratorWithIndex<InputImageType> it(
        this->GetInput(), this->GetInput()->GetRequestedRegion());

    while(!it.IsAtEnd())
    {
        typename TInputImage::PixelType const & value = it.Get();
        typename TInputImage::IndexType const & index = it.GetIndex();

        if(value >= this->m_t)
        {
            // 3.2, p. 7 : Intensity values are upper limited at t_98
            float const mass = std::max(value, this->m_T98);
            totalMass += mass;

            for(unsigned int i=0; i<TInputImage::ImageDimension; ++i)
            {
                cog[i] += float(mass)*float(index[i]);
            }
        }
        ++it;
    }

    cog /= totalMass;

    for(unsigned int d=0; d<TInputImage::ImageDimension; ++d)
    {
        this->m_CenterOfGravity[d] = cog[d];
    }

    this->is_cog_specified_ = true;
}

template<typename TInputImage, typename TOutputImage>
BETImageFilter<TInputImage, TOutputImage>
::BETImageFilter()
: m_BT(0.5),
  m_T2(0), m_T98(0), m_t(0), are_thresholds_specified_(false),
  is_cog_specified_(false), m_SmoothnessFactor(0)
{
    // Nothing more to do
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::PrintSelf(std::ostream& os, Indent indent) const
{
    Superclass::PrintSelf(os,indent);
    os << indent << "BT (main BET parameter) : " << this->m_BT << "\n";
    os << indent << "T2 : " << this->m_T2 << ", T98 : " << this->m_T98;
    if(!this->are_thresholds_specified_)
    {
        os << " (thresholds will be estimated during Update)";
    }
    os << "\n";
    os << indent << "CenterOfGravity : " << this->m_CenterOfGravity;
    if(!this->is_cog_specified_)
    {
        os << " (center of gravity will be estimated during Update)";
    }
    os << "\n";
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::GenerateData()
{
    // Used in 3.4
    unsigned int const nbIterations = 1000;

    // Used in 3.4.4, values are in mm
    float const r_min = 3.33;
    float const r_max = 10.;
    float const E = (1./r_min+1./r_max)/2.;
    float const F = 6./(1./r_min-1./r_max);

    // Used in 3.4.5, values are in voxels
    float const maxSpacing = *std::max_element(
        this->GetInput()->GetSpacing().Begin(), this->GetInput()->GetSpacing().End());
    float const d1 = 20./maxSpacing;
    float const d2 = d1/2.;

    this->AllocateOutputs();
    this->GetOutput()->FillBuffer(NumericTraits<typename TOutputImage::PixelType>::Zero);

    /**************************************************************
    * 3.2 Estimation of Basic Image and Brain Parameters (p. 5-7) *
    **************************************************************/
    // Thresholds
    if(!this->are_thresholds_specified_)
    {
        this->EstimateThresholds();
    }

    // Center of gravity
    if(!this->is_cog_specified_)
    {
        this->EstimateCenterOfGravity();
    }
    else
    {
        itkDebugMacro(<< "Skipping computation of center of gravity : user-defined");
    }
    itkDebugMacro(<< "Center of gravity : " << this->m_CenterOfGravity);

    // Radius
    this->radius();
    itkDebugMacro(<< "Radius : " << this->r_);

    // Median intensity
    this->medianIntensity();
    itkDebugMacro(<< "tm : " << this->tm_);

    /************************************************
    * 3.3 Surface Model and Initialization (p. 7-8) *
    ************************************************/
    unsigned int const subdivisions = 4;
    this->buildSphere(subdivisions);

    this->neighborhood_.resize(this->sphere_->GetNumberOfPoints());
    for(vtkIdType pointId=0; pointId<this->sphere_->GetNumberOfPoints(); ++pointId)
    {
        std::vector<vtkIdType> const neighbors = this->neighbors(pointId);
        this->neighborhood_[pointId] = neighbors;
    }

    /***********************************
    * 3.4 Main iterated loop (p. 8-14) *
    ***********************************/

    for(unsigned int iteration=0; iteration<nbIterations; ++iteration)
    {
        float smoothness;
        if(iteration<=0.75*nbIterations)
        {
            smoothness = std::pow(10.f, float(this->m_SmoothnessFactor));
        }
        else
        {
            // Linear interpolation between 1 and 10^pass
            float const alpha = (iteration-0.75*nbIterations)/(0.25*nbIterations);
            smoothness = (1-alpha)*std::pow(10.f, float(this->m_SmoothnessFactor))+alpha;
        }

        // Update l
        this->meanVertexDistance();
        itkDebugMacro(<< "Mean vertex distance updated to : " << this->l_);

        /*************************************
        * 3.4.1 Local Surface Normal (p.8-9) *
        *************************************/
        vtkSmartPointer<vtkPolyDataNormals> normalFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
        normalFilter->SetInput(this->sphere_);
        // We know that all the normals on the original model are outward-pointing
        normalFilter->AutoOrientNormalsOff();
        normalFilter->ConsistencyOff();
        // Don't create new points at sharp edges
        normalFilter->SplittingOff();
        normalFilter->Update();
        vtkPolyData* normals = normalFilter->GetOutput();

        vtkDataArray* pointsDataArray = normals->GetPoints()->GetData();
        vtkDataArray* normalsDataArray = normals->GetPointData()->GetNormals();
        if(pointsDataArray->GetDataType() != VTK_FLOAT || normalsDataArray->GetDataType() != VTK_FLOAT)
        {
            throw std::runtime_error("Points should be VTK_FLOAT");
        }

        std::vector<vnl_vector_fixed<float, 3> > displacements(normals->GetNumberOfPoints());

        MultiThreader::Pointer threader = MultiThreader::New();
        threader->SetNumberOfThreads(this->GetNumberOfThreads());

        Self::ThreadStruct thread_structure;
        thread_structure.filter = this;
        thread_structure.pointsDataArray = pointsDataArray;
        thread_structure.normalsDataArray = normalsDataArray;
        thread_structure.E = E;
        thread_structure.F = F;
        thread_structure.d1 = d1;
        thread_structure.d2 = d2;
        thread_structure.smoothness = smoothness;
        thread_structure.displacements = &displacements;
        threader->SetSingleMethod(Self::adjust_model_callback, &thread_structure);
        threader->SingleMethodExecute();

        for(vtkIdType pointId=0; pointId<this->sphere_->GetNumberOfPoints(); ++pointId)
        {
            vnl_vector_fixed_ref<float, 3> p((float*)pointsDataArray->GetVoidPointer(3*pointId));
            p += displacements[pointId];
        }
    } // for all iterations

    this->voxelize();
}

template<typename TInputImage, typename TOutputImage>
ITK_THREAD_RETURN_TYPE
BETImageFilter<TInputImage, TOutputImage>
::adjust_model_callback(void* data)
{
    MultiThreader::ThreadInfoStruct* thread_info =
        reinterpret_cast<MultiThreader::ThreadInfoStruct*>(data);
    int const thread_id = thread_info->ThreadID;
    int const thread_count = thread_info->NumberOfThreads;
    Self::ThreadStruct* user_data = reinterpret_cast<Self::ThreadStruct*>(thread_info->UserData);

    // Split the total number of points
    Self* filter = user_data->filter;
    int const points_count = filter->sphere_->GetNumberOfPoints();
    int const chunk = points_count/thread_count;
    vtkIdType const begin = thread_id*chunk;
    vtkIdType const end = (thread_id==thread_count-1)?(points_count):(begin+chunk);

    // Get the rest of the arguments
    vtkDataArray* pointsDataArray = user_data->pointsDataArray;
    vtkDataArray* normalsDataArray = user_data->normalsDataArray;
    float E = user_data->E;
    float F = user_data->F;
    float d1 = user_data->d1;
    float d2 = user_data->d2;
    float smoothness = user_data->smoothness;
    std::vector<vnl_vector_fixed<float, 3> >*  displacements = user_data->displacements;

    // Call the processing function
    Self::adjust_model(filter, E, F, d1, d2, smoothness,
                       pointsDataArray, normalsDataArray,
                       begin, end, *displacements);

    return ITK_THREAD_RETURN_VALUE;
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::adjust_model(BETImageFilter<TInputImage, TOutputImage>* filter,
    float E, float F, float d1, float d2, float smoothness,
    vtkDataArray* pointsDataArray, vtkDataArray* normalsDataArray,
    vtkIdType points_begin, vtkIdType points_end,
    std::vector<vnl_vector_fixed<float, 3> > & displacements)
{
    typedef typename InputImageType::IndexType InputImageIndexType;

    for(vtkIdType pointId=points_begin; pointId<points_end; ++pointId)
    {
        // Current point
        vnl_vector_fixed_ref_const<float, 3> const p((float*)pointsDataArray->GetVoidPointer(3*pointId));
        // Current normal
        vnl_vector_fixed_ref_const<float, 3> const n((float*)normalsDataArray->GetVoidPointer(3*pointId));

        /*****************************************************************
        * 3.4.2 Mean Position of Neighbours and Difference Vector (p. 9) *
        *****************************************************************/
        // Find all neighbors
        std::vector<vtkIdType> const & neighbors = filter->neighborhood_[pointId];

        // Difference vector
        vnl_vector_fixed<float, 3> s(0,0,0);
        for(std::vector<vtkIdType>::const_iterator neighborsIt=neighbors.begin();
            neighborsIt != neighbors.end(); ++neighborsIt)
        {
            vnl_vector_fixed_ref_const<float, 3> const neighbor(
                (float*)pointsDataArray->GetVoidPointer(3*(*neighborsIt)));
            s += neighbor;
        }
        s /= neighbors.size();
        s -= p;

        // Decompose difference vector
        float const dotProduct = s[0]*n[0]+s[1]*n[1]+s[2]*n[2];
        vnl_vector_fixed<float, 3> const sn(dotProduct*n);
        vnl_vector_fixed<float, 3> const st(s-sn);

        /******************************************************************
        * 3.4.3 Update Component 1: Within-Surface Vertex Spacing (p. 10) *
        ******************************************************************/
        vnl_vector_fixed<float, 3> const u1(st/2.f);

        /******************************************************************
        * 3.4.4 Update Component 2: Surface Smoothness Control (p. 10-12) *
        ******************************************************************/

        float const r = (filter->l_*filter->l_)/(2*sn.magnitude());
        float f2 = (1.+std::tanh(F*(1./r-E)))/2.;

        /******************************************************************
        * 3.4.7 Increased smoothing                                       *
        ******************************************************************/
        // The sign of sn.n determine if the area if convex or concave,
        // cf. figure 4. If smoothness == 1, this is the first pass, and f2
        // should not be modified
        float const concave = sn[0]*n[0]+sn[1]*n[1]+sn[2]*n[2];
        if(smoothness>1 && concave > 0)
        {
            f2 *= smoothness;
            f2 = std::min(f2, 1.f);
        }

        vnl_vector_fixed<float, 3> const u2(f2*sn);

        /********************************************************************
        * 3.4.5 Update Component 3: Brain Surface Selection Term (p. 12-14) *
        ********************************************************************/
        // Line pointing inward from the current vertex
        InputImageType const * input = filter->GetInput();
        typename InputImageType::SpacingType const & spacing = input->GetSpacing();
        InputImageIndexType const firstIndex = {{
                (p[0]-n[0])/spacing[0],
                (p[1]-n[1])/spacing[1],
                (p[2]-n[2])/spacing[2] }};
        InputImageIndexType const lastIndex = {{
                (p[0]-d1*n[0])/spacing[0],
                (p[1]-d1*n[1])/spacing[1],
                (p[2]-d1*n[2])/spacing[2] }};

        float f3;
        if(input->GetBufferedRegion().IsInside(firstIndex) &&
           input->GetBufferedRegion().IsInside(lastIndex))
        {
            InputImagePixelType const firstIntensity = input->GetPixel(firstIndex);
            InputImagePixelType const lastIntensity = input->GetPixel(lastIndex);

            InputImagePixelType I_min = std::min(filter->tm_, std::min(firstIntensity, lastIntensity));
            InputImagePixelType I_max = std::max(filter->m_t, std::max(firstIntensity, lastIntensity));

            for(float d=2; d<d1; ++d)
            {
                InputImageIndexType const index = {{
                    (p[0]-d*n[0])/filter->GetInput()->GetSpacing()[0],
                    (p[1]-d*n[1])/filter->GetInput()->GetSpacing()[1],
                    (p[2]-d*n[2])/filter->GetInput()->GetSpacing()[2]
                }};

                InputImagePixelType const value = filter->GetInput()->GetPixel(index);

                I_min = std::min(I_min, value);

                if(d<d2)
                {
                    I_max = std::max(I_max, value);
                }
            }

            I_min = std::max(filter->m_T2, I_min);
            I_max = std::min(filter->tm_, I_max);

            float const tl = (I_max-filter->m_T2)*filter->m_BT+filter->m_T2;
            if(float(I_max)-filter->m_T2>0)
            {
                f3 = 2.*(I_min-tl)/(float(I_max)-filter->m_T2);
            }
            else
            {
                // Use maximum if intensity is too low
                // cf. BET2, 2.3.2
                f3 = 2.*(I_min-tl);
            }
        }
        else
        {
            f3 = 0;
        }

        // There is a typo in the paper : it reads \^s_n, but should
        // read n
        float const coefficient = 0.05*f3*filter->l_;
        vnl_vector_fixed<float, 3> const u3(coefficient*n);

        vnl_vector_fixed<float, 3> const u(u1+u2+u3);
        displacements[pointId] = u;

    } // for all points

}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::radius()
{
    // Radius : count number of voxels >= t, assume this is a sphere, and derive
    // the radius from the volume of the sphere (4/3*pi*r^3)

    ImageRegionConstIterator<InputImageType> it(
        this->GetInput(), this->GetInput()->GetRequestedRegion());

    unsigned long nbPoints = 0;
    while(!it.IsAtEnd())
    {
        typename InputImageType::PixelType const & value = it.Get();
        if(value >= this->m_t)
        {
            ++nbPoints;
        }
        ++it;
    }

    float voxel_volume = 1.;
    for(unsigned int i=0; i<InputImageType::ImageDimension; ++i)
    {
        voxel_volume *= this->GetInput()->GetSpacing()[i];
    }
    this->r_ = std::pow(nbPoints*voxel_volume*0.75/M_PI, 1./3.);

    // Test if initial estimate is too large (i.e. sphere is not fully contained
    // in image region. If so, use a smaller estimate.
    typename InputImageType::SizeType const size =
        this->GetInput()->GetRequestedRegion().GetSize();
    typename InputImageType::SpacingType const spacing =
        this->GetInput()->GetSpacing();
    for(unsigned int d=0; d<InputImageType::ImageDimension; ++d)
    {
        if(this->m_CenterOfGravity[d]+this->r_*spacing[d] > size[d] ||
           this->m_CenterOfGravity[d]-this->r_*spacing[d] < 0)
        {
            this->r_ /= 2.;
            break;
        }
    }
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::medianIntensity()
{
    ImageRegionConstIteratorWithIndex<TInputImage> it(
        this->GetInput(), this->GetInput()->GetRequestedRegion());

    float const rSquared = std::pow(this->r_, 2);
    float const rCubed = std::pow(this->r_, 3);

    std::vector<typename TInputImage::PixelType> intensities;
    intensities.reserve(4./3.*M_PI*(rCubed+1));

    typename InputImageType::PointType cog;
    for(unsigned int d=0; d<TInputImage::ImageDimension; ++d)
    {
        cog[d] = this->m_CenterOfGravity[d]*this->GetInput()->GetSpacing()[d];
    }

    while(!it.IsAtEnd())
    {
        typename TInputImage::PixelType const value = it.Get();

        typename TInputImage::IndexType const index = it.GetIndex();

        typename TInputImage::PointType p;
        for(unsigned int d=0; d<this->GetInput()->GetImageDimension(); ++d)
        {
            p[d] = index[d]*this->GetInput()->GetSpacing()[d];
        }

        if(value >= this->m_T2 && value <= this->m_T98 &&
           p.SquaredEuclideanDistanceTo(cog) <= rSquared)
        {
            intensities.push_back(value);
        }

        ++it;
    }

    std::sort(intensities.begin(), intensities.end());
    if(intensities.size()%2==1)
    {
        this->tm_ = intensities[intensities.size()/2];
    }
    else
    {
        this->tm_ = (intensities[intensities.size()/2-1]+intensities[intensities.size()/2])/2.;
    }
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::buildSphere(unsigned int subdivisions)
{
    vtkSmartPointer<vtkPlatonicSolidSource> icosahedron = vtkSmartPointer<vtkPlatonicSolidSource>::New();
    icosahedron->SetSolidTypeToIcosahedron();

    vtkSmartPointer<vtkLinearSubdivisionFilter> subdivision = vtkSmartPointer<vtkLinearSubdivisionFilter>::New();
    subdivision->SetInputConnection(icosahedron->GetOutputPort());
    subdivision->SetNumberOfSubdivisions(subdivisions);

    subdivision->Update();

    vtkDataArray* dataArray = subdivision->GetOutput()->GetPoints()->GetData();
    if(dataArray->GetDataType() != VTK_FLOAT)
    {
        throw std::runtime_error("Points should be VTK_FLOAT");
    }
    for(vtkIdType i=0; i<subdivision->GetOutput()->GetNumberOfPoints(); ++i)
    {
        vnl_vector_fixed_ref<float, 3> p((float*)dataArray->GetVoidPointer(3*i));
        p /= p.magnitude();
    }

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(this->m_CenterOfGravity[0]*this->GetInput()->GetSpacing()[0],
                         this->m_CenterOfGravity[1]*this->GetInput()->GetSpacing()[1],
                         this->m_CenterOfGravity[2]*this->GetInput()->GetSpacing()[2]);
    transform->Scale(this->r_, this->r_, this->r_);

    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInput(subdivision->GetOutput());
    transformFilter->SetTransform(transform);
    transformFilter->Update();

    // vtkPlatonicSolidSource give inwards pointing normals
    vtkPolyData* sphere = transformFilter->GetOutput();
    for(unsigned int i=0; i<sphere->GetNumberOfCells(); ++i)
    {
        sphere->ReverseCell(i);
    }

    this->sphere_ = transformFilter->GetOutput();
}

template<typename TInputImage, typename TOutputImage>
std::vector<vtkIdType>
BETImageFilter<TInputImage, TOutputImage>
::neighbors(vtkIdType pointId)
{
    vtkSmartPointer<vtkIdList> cells = vtkSmartPointer<vtkIdList>::New();
    this->sphere_->GetPointCells(pointId, cells);

    std::vector<vtkIdType> neighbors;
    for(vtkIdType const * cellIt=cells->GetPointer(0);
        cellIt!=cells->GetPointer(0)+cells->GetNumberOfIds(); ++cellIt)
    {
        vtkIdType const cellId = *cellIt;
        vtkSmartPointer<vtkIdList> points = vtkSmartPointer<vtkIdList>::New();
        this->sphere_->GetCellPoints(cellId, points);
        for(vtkIdType const* neighborIt = points->GetPointer(0);
            neighborIt != points->GetPointer(0)+points->GetNumberOfIds(); ++neighborIt)
        {
            vtkIdType const neighbor = *neighborIt;
            if(neighbor != pointId &&
               std::find(neighbors.begin(), neighbors.end(), neighbor)==neighbors.end())
            {
                neighbors.push_back(neighbor);
            }
        }
    }

    return neighbors;
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::meanVertexDistance()
{
    MultiThreader::Pointer threader = MultiThreader::New();
    threader->SetNumberOfThreads(this->GetNumberOfThreads());

    Self::MeanVertexDistanceThreadStruct thread_structure;
    thread_structure.model = this->sphere_;
    thread_structure.neighborhood = &this->neighborhood_;
    thread_structure.distances = new float[threader->GetNumberOfThreads()];
    threader->SetSingleMethod(Self::mean_vertex_distance_callback, &thread_structure);
    threader->SingleMethodExecute();

    float const distance = std::accumulate(
        thread_structure.distances,
        thread_structure.distances+threader->GetNumberOfThreads(),
        0);

    this->l_ = distance / this->sphere_->GetNumberOfPoints();

    delete[] thread_structure.distances;
}

template<typename TInputImage, typename TOutputImage>
ITK_THREAD_RETURN_TYPE
BETImageFilter<TInputImage, TOutputImage>
::mean_vertex_distance_callback(void* data)
{
    MultiThreader::ThreadInfoStruct* thread_info =
        reinterpret_cast<MultiThreader::ThreadInfoStruct*>(data);
    int const thread_id = thread_info->ThreadID;
    int const thread_count = thread_info->NumberOfThreads;
    Self::MeanVertexDistanceThreadStruct* user_data =
        reinterpret_cast<Self::MeanVertexDistanceThreadStruct*>(thread_info->UserData);

    // Split the total number of points
    vtkPolyData* model = user_data->model;
    int const points_count = model->GetNumberOfPoints();
    int const chunk = points_count/thread_count;
    vtkIdType const begin = thread_id*chunk;
    vtkIdType const end = (thread_id==thread_count-1)?(points_count):(begin+chunk);

    // Get the rest of the arguments
    std::vector<std::vector<vtkIdType> >* neighborhood = user_data->neighborhood;

    // Call the processing function
    user_data->distances[thread_id] = Self::mean_vertex_distance_thread(model, *neighborhood, begin, end);

    return ITK_THREAD_RETURN_VALUE;
}

template<typename TInputImage, typename TOutputImage>
float
BETImageFilter<TInputImage, TOutputImage>
::mean_vertex_distance_thread(vtkPolyData* model,
    std::vector<std::vector<vtkIdType> > const & neighborhood,
    vtkIdType points_begin, vtkIdType points_end)
{
    vtkDataArray* dataArray(model->GetPoints()->GetData());

    float total_distance = 0;
    for(vtkIdType pointId=points_begin; pointId<points_end; ++pointId)
    {
        vnl_vector_fixed_ref_const<float, 3> const p((float*)dataArray->GetVoidPointer(3*pointId));

        float distance = 0;
        std::vector<vtkIdType> const & neighbors = neighborhood[pointId];
        for(std::vector<vtkIdType>::const_iterator neighborsIt=neighbors.begin();
            neighborsIt != neighbors.end(); ++neighborsIt)
        {
            vnl_vector_fixed_ref_const<float, 3> const neighbor((float*)dataArray->GetVoidPointer(3*(*neighborsIt)));
            distance += std::sqrt(vnl_vector_ssd(p, neighbor));
        }
        distance /= neighbors.size();
        total_distance += distance;
    }

    return total_distance;
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::voxelize()
{
    this->drawMesh();

    // Find center of gravity of mesh
    vtkDataArray* dataArray(this->sphere_->GetPoints()->GetData());
    vnl_vector_fixed<float, 3> cog(0,0,0);
    for(vtkIdType pointId=0; pointId<dataArray->GetNumberOfTuples(); ++pointId)
    {
        vnl_vector_fixed_ref<float, 3> const p((float*)dataArray->GetVoidPointer(3*pointId));
        cog += p;
    }
    cog /= dataArray->GetNumberOfTuples();

    typename TOutputImage::IndexType seed = {{
        cog[0]/this->GetOutput()->GetSpacing()[0],
        cog[1]/this->GetOutput()->GetSpacing()[1],
        cog[2]/this->GetOutput()->GetSpacing()[2] }};

    this->fill(seed);
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::drawSegment(vnl_vector_fixed_ref<float, TOutputImage::ImageDimension> const & p1,
              vnl_vector_fixed_ref<float, TOutputImage::ImageDimension> const & p2,
              float resolution)
{
    vnl_vector_fixed<float, TOutputImage::ImageDimension> direction(p1-p2);
    float const magnitude = direction.magnitude();

    direction /= magnitude;
    vnl_vector_fixed<float, TOutputImage::ImageDimension> const increment(
        direction*resolution);

    vnl_vector_fixed<float, TOutputImage::ImageDimension> p(p2);
    for(float i=0; i<=magnitude; i+=resolution)
    {
        typename TOutputImage::IndexType const index = {{
            p[0]/this->GetOutput()->GetSpacing()[0],
            p[1]/this->GetOutput()->GetSpacing()[1],
            p[2]/this->GetOutput()->GetSpacing()[2] }};

        if(this->GetOutput()->GetRequestedRegion().IsInside(index))
        {
            this->GetOutput()->SetPixel(index, 1);
        }

        p+=increment;
    }
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::drawTriangle(vnl_vector_fixed_ref<float, TOutputImage::ImageDimension> const & p1,
               vnl_vector_fixed_ref<float, TOutputImage::ImageDimension> const & p2,
               vnl_vector_fixed_ref<float, TOutputImage::ImageDimension> const & p3,
               float resolution)
{
    vnl_vector_fixed<float, TOutputImage::ImageDimension> direction(p1-p2);
    float const magnitude = direction.magnitude();

    direction /= magnitude;
    vnl_vector_fixed<float, 3> const increment = direction*resolution;

    vnl_vector_fixed<float, 3> p(p2);
    for(float i=0; i<=magnitude; i+=resolution)
    {
        drawSegment(p, p3, resolution);

        p+=increment;
    }
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::drawMesh()
{
    vtkCellArray* polygons = this->sphere_->GetPolys();
    vtkDataArray* dataArray(this->sphere_->GetPoints()->GetData());

    polygons->InitTraversal();
    vtkIdType npts;
    vtkIdType* points;

    float const minSpacing = *std::min_element(
        this->GetOutput()->GetSpacing().GetDataPointer(),
        this->GetOutput()->GetSpacing().GetDataPointer()+this->GetOutput()->GetSpacing().Size());

    while(polygons->GetNextCell(npts, points))
    {
        if(npts != 3)
        {
            std::cout << "Found poly with " << npts << " vertices" << std::endl;
            continue;
        }

        vnl_vector_fixed_ref<float, TOutputImage::ImageDimension> const p1(
            (float*)dataArray->GetVoidPointer(3*points[0]));
        vnl_vector_fixed_ref<float, TOutputImage::ImageDimension> const p2(
            (float*)dataArray->GetVoidPointer(3*points[1]));
        vnl_vector_fixed_ref<float, TOutputImage::ImageDimension> const p3(
            (float*)dataArray->GetVoidPointer(3*points[2]));

        drawTriangle(p1, p2, p3, 0.5*minSpacing);
    }
}

template<typename TInputImage, typename TOutputImage>
void
BETImageFilter<TInputImage, TOutputImage>
::fill(typename TOutputImage::IndexType const & seed)
{
    std::queue<typename TOutputImage::IndexType> queue;

    queue.push(seed);
    this->GetOutput()->SetPixel(seed, 1);

    while(!queue.empty())
    {
        typename TOutputImage::IndexType const & current = queue.front();

        for(unsigned int d=0; d<this->GetOutput()->GetImageDimension(); ++d)
        {
            for(int offset=-1; offset<=1; offset+=2)
            {
                // Build neighbor
                typename TOutputImage::IndexType neighbor;
                for(unsigned int n=0; n<this->GetOutput()->GetImageDimension(); ++n)
                {
                    if(n==d)
                    {
                        neighbor[n] = current[n]+offset;
                    }
                    else
                    {
                        neighbor[n] = current[n];
                    }
                }

                // Flood-fill
                if(this->GetOutput()->GetRequestedRegion().IsInside(neighbor) &&
                   this->GetOutput()->GetPixel(neighbor) == 0)
                {
                    this->GetOutput()->SetPixel(neighbor, 1);
                    queue.push(neighbor);
                }
            }
        }

        queue.pop();
    }
}

}

#endif // components_segmentation_betimagefilter_txx
