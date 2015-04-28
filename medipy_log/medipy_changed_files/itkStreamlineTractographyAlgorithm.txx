/*==========================================================================
  
  © Université de Strasbourg - Centre National de la Recherche Scientifique
  
  Date: 22/08/2012
  Author(s): Julien Pontabry (pontabry@unistra.fr)
  
  This software is governed by the CeCILL-B license under French law and
  abiding by the rules of distribution of free software.  You can  use,
  modify and/ or redistribute the software under the terms of the CeCILL-B
  license as circulated by CEA, CNRS and INRIA at the following URL
  "http://www.cecill.info".
  
  As a counterpart to the access to the source code and  rights to copy,
  modify and redistribute granted by the license, users are provided only
  with a limited warranty  and the software's author,  the holder of the
  economic rights,  and the successive licensors  have only  limited
  liability.
  
  In this respect, the user's attention is drawn to the risks associated
  with loading,  using,  modifying and/or developing or reproducing the
  software by the user in light of its specific status of free software,
  that may mean  that it is complicated to manipulate,  and  that  also
  therefore means  that it is reserved for developers  and  experienced
  professionals having in-depth computer knowledge. Users are therefore
  encouraged to load and test the software's suitability as regards their
  requirements in conditions enabling the security of their systems and/or
  data to be ensured and,  more generally, to use and operate it in the
  same conditions as regards security.
  
  The fact that you are presently reading this means that you have had
  knowledge of the CeCILL-B license and that you accept its terms.
  
==========================================================================*/

#ifndef BTK_STREAMLINE_TRACTOGRAPHY_ALGORITHM_cxx
#define BTK_STREAMLINE_TRACTOGRAPHY_ALGORITHM_cxx

#include "itkStreamlineTractographyAlgorithm.h"

#define M_PI       3.14159265358979323846

namespace itk
{

template<typename ModelType, typename MaskType>
StreamlineTractographyAlgorithm<ModelType, MaskType>
::StreamlineTractographyAlgorithm() 
: Superclass(), m_StepSize(0.5), m_UseRungeKuttaOrder4(false), 
  m_MaximumAngle(M_PI/3.0f), m_MinimumFA(0.2), m_MinimumLength(10), 
  m_MaximumLength(800)
{
    m_Calculator.SetDimension(3);
}

//----------------------------------------------------------------------------------------

template<typename ModelType, typename MaskType>
void
StreamlineTractographyAlgorithm<ModelType, MaskType>
::PrintSelf(std::ostream &os, Indent indent) const
{
    Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------------------


template<typename ModelType, typename MaskType>
typename StreamlineTractographyAlgorithm<ModelType, MaskType>::FiberType
StreamlineTractographyAlgorithm<ModelType, MaskType>
::PropagateSeed(PointType const &seed, typename InterpolateModelType::Pointer &interpolate, typename VectorImageToImageAdaptorType::Pointer &adaptor)
{
    FiberType currentFiber;

    // Propagation
    if(m_UseRungeKuttaOrder4) {
        currentFiber = PropagateSeedRK4(seed,interpolate,adaptor);
    }
    else {
        currentFiber = PropagateSeedRK1(seed,interpolate,adaptor);
    }
    
    if(currentFiber.size()*this->m_StepSize < m_MinimumLength)
    {
        currentFiber = FiberType();
    }

    return currentFiber;
}

//----------------------------------------------------------------------------------------

template<typename ModelType, typename MaskType>
typename StreamlineTractographyAlgorithm<ModelType, MaskType>::FiberType
StreamlineTractographyAlgorithm<ModelType, MaskType>
::PropagateSeedRK1(PointType const &seed, typename InterpolateModelType::Pointer &interpolate, typename VectorImageToImageAdaptorType::Pointer &adaptor)
{
    bool stop = false;
    VectorType d1 = PropagationDirectionT2At(seed,stop,interpolate,adaptor);
    VectorType d2 = -d1;

    std::vector< PointType > points1;  
    std::vector< PointType > points2;

    // Track towards one direction
    PointType nextPoint = seed;
    while (!stop) {
        // Insert new point 
        points1.push_back(nextPoint);
        // This use the RK1 (Euler) to compute the next point (Runge-Kutta, order 1, Euler method)
        nextPoint = points1.back() + (d1*m_StepSize);
        // Propagation
        VectorType d = PropagationDirectionT2At(nextPoint,stop,interpolate,adaptor);
        // Select the best propagation direction
        d1 = SelectClosestDirectionT2(d,d1,stop);
        if(points1.size()*this->m_StepSize > this->m_MaximumLength)
        {
            stop = true;
        }
    }
    
    // Track towards the opposite direction
    stop = (points1.size()*this->m_StepSize <= this->m_MaximumLength);
    nextPoint = seed;
    while (!stop) {
        points2.push_back(nextPoint);
        nextPoint = points2.back() + (d2*m_StepSize);
        VectorType d = PropagationDirectionT2At(nextPoint,stop,interpolate,adaptor);
        d2 = SelectClosestDirectionT2(d,d2,stop);
        if(points2.size()*this->m_StepSize > this->m_MaximumLength)
        {
            stop = true;
        }
    }
    
    // Concatenate
    std::vector< PointType > points;
    if(!points2.empty())
    {
        std::copy(points2.rbegin(), points2.rend()-1, std::back_inserter(points));
    }
    std::copy(points1.begin(), points1.end(), std::back_inserter(points));
    
    return points;
}

//----------------------------------------------------------------------------------------

template<typename ModelType, typename MaskType>
typename StreamlineTractographyAlgorithm<ModelType, MaskType>::VectorType
StreamlineTractographyAlgorithm<ModelType, MaskType>
::PropagationDirectionT2At(PointType const &point, bool &stop, typename InterpolateModelType::Pointer &interpolate, typename VectorImageToImageAdaptorType::Pointer &adaptor)
{
    VectorType pd;

    // Check if the point is still the the model
    typename ModelType::IndexType modelIndex;
    this->m_InputModel->TransformPhysicalPointToIndex(point, modelIndex);
    if ( ((unsigned int)modelIndex[0]>=(this->m_size[0]-1)) || ((unsigned int)modelIndex[1]>=(this->m_size[1]-1)) || ((unsigned int)modelIndex[2]>=(this->m_size[2]-1)) ||
         ((unsigned int)modelIndex[0]<=0) || ((unsigned int)modelIndex[1]<=0) || ((unsigned int)modelIndex[2]<=0) ) { stop = true; }

    // Check if the point is in the mask
    if (! stop && this->m_Mask.IsNotNull()) {
        typename MaskType::IndexType maskIndex;
        this->m_Mask->TransformPhysicalPointToIndex(point, maskIndex);
        if(this->m_Mask->GetPixel(maskIndex)==0) { stop = true; }
    }

    if (!stop) {
        Vector<float,6> dt6;          
        for( unsigned int i=0; i<6; i++) {
            adaptor->SetExtractComponentIndex( i );
            adaptor->Update();
            dt6[i] = interpolate->Evaluate(point);
        } 

        InputMatrixType dt33(3,3);
        dt33[0][0] =  (double) dt6[0];
        dt33[1][1] =  (double) dt6[3];
        dt33[2][2] =  (double) dt6[5];
        dt33[0][1] =  (double) dt6[1];
        dt33[0][2] =  (double) dt6[2];
        dt33[1][2] =  (double) dt6[4];
        dt33[1][0] =  dt33[0][1];
        dt33[2][0] =  dt33[0][2];
        dt33[2][1] =  dt33[1][2];

        EigenValuesArrayType eigenvalues;
        EigenVectorMatrixType eigenvectors;
        m_Calculator.ComputeEigenValuesAndVectors(dt33,eigenvalues,eigenvectors);

        pd[0] = eigenvectors(2,0);
        pd[1] = eigenvectors(2,1);
        pd[2] = eigenvectors(2,2);

        // Check if the point is within the white matter
        double ev1 = eigenvalues[2];
        double ev2 = eigenvalues[1];
        double ev3 = eigenvalues[0];
        double fa = 0.5 * ((ev1-ev2)*(ev1-ev2) + (ev2-ev3)*(ev2-ev3) + (ev3-ev1)*(ev3-ev1)) / (ev1*ev1 + ev2*ev2 + ev3*ev3);
        if (fa>0) { fa = sqrt(fa); }
        else { fa = 0; }
        if ((float)fa<m_MinimumFA) {
            stop = true;
        }
    }

    return pd;
}

//----------------------------------------------------------------------------------------

template<typename ModelType, typename MaskType>
typename StreamlineTractographyAlgorithm<ModelType, MaskType>::VectorType
StreamlineTractographyAlgorithm<ModelType, MaskType>
::SelectClosestDirectionT2(VectorType const &currentDirection, VectorType const &previousDirection, bool &stop)
{
    // The new direction is choosen to be the closer to the previous one.
    float dotProduct = currentDirection*previousDirection;
    VectorType nextDirection;
    if (dotProduct<0) { nextDirection = -currentDirection; }
    else { nextDirection = currentDirection; }

    // Check if the propagation angle is allowed
    float angle = std::acos( std::abs(dotProduct) );
    if (angle > m_MaximumAngle) { stop = true; }

    return nextDirection;
}

//----------------------------------------------------------------------------------------

template<typename ModelType, typename MaskType>
typename StreamlineTractographyAlgorithm<ModelType, MaskType>::FiberType
StreamlineTractographyAlgorithm<ModelType, MaskType>
::PropagateSeedRK4(PointType const &seed, typename InterpolateModelType::Pointer &interpolate, typename VectorImageToImageAdaptorType::Pointer &adaptor)
{
    // Usefull constants
    float stepSize_2 = m_StepSize / 2.f;
    float stepSize_6 = m_StepSize / 6.f;

    bool stop = false;
    VectorType d1 = PropagationDirectionT2At(seed,stop,interpolate,adaptor);
    VectorType d2 = -d1;

    std::vector< PointType > points1;  
    std::vector< PointType > points2;

    // Track towards one direction
    PointType nextPoint = seed;
    while (!stop) {
        // Insert new point 
        points1.push_back(nextPoint);
        // This use the RK4 to compute the next point (Runge-Kutta, order 4)
        // k1
        VectorType k1 = d1;
        // k2
        nextPoint = points1.back() + (k1*stepSize_2);
        VectorType d = PropagationDirectionT2At(nextPoint,stop,interpolate,adaptor);
        if (d==VectorType()) { break; }
        VectorType k2 = SelectClosestDirectionT2(d,k1,stop);
        // k3
        nextPoint = points1.back() + (k2*stepSize_2);
        d = PropagationDirectionT2At(nextPoint,stop,interpolate,adaptor);
        if (d==VectorType()) { break; }
        VectorType k3 = SelectClosestDirectionT2(d,k2,stop);
        // k4
        nextPoint = points1.back() + (k3*m_StepSize);
        d = PropagationDirectionT2At(nextPoint,stop,interpolate,adaptor);
        if (d==VectorType()) { break; }
        VectorType k4 = SelectClosestDirectionT2(d,k3,stop);
        // result
        nextPoint = points1.back() + (k1 + k2*2.f + k3*2.f + k4) * stepSize_6;
        d = PropagationDirectionT2At(nextPoint,stop,interpolate,adaptor);
        d1 = SelectClosestDirectionT2(d,k1,stop);
        if(points1.size()*this->m_StepSize > this->m_MaximumLength)
        {
            stop = true;
        }
    }


    // Track towards the opposite direction
    stop = (points1.size()*this->m_StepSize <= this->m_MaximumLength);
    nextPoint = seed;
    while (!stop) {
        // Insert new point 
        points2.push_back(nextPoint);
        // This use the RK4 to compute the next point (Runge-Kutta, order 4)
        // k1
        VectorType k1 = d2;
        // k2
        nextPoint = points2.back() + (k1*stepSize_2);
        VectorType d = PropagationDirectionT2At(nextPoint,stop,interpolate,adaptor);
        if (d==VectorType()) { break; }
        VectorType k2 = SelectClosestDirectionT2(d,k1,stop);
        // k3
        nextPoint = points2.back() + (k2*stepSize_2);
        d = PropagationDirectionT2At(nextPoint,stop,interpolate,adaptor);
        if (d==VectorType()) { break; }
        VectorType k3 = SelectClosestDirectionT2(d,k2,stop);
        // k4
        nextPoint = points2.back() + (k3*m_StepSize);
        d = PropagationDirectionT2At(nextPoint,stop,interpolate,adaptor);
        if (d==VectorType()) { break; }
        VectorType k4 = SelectClosestDirectionT2(d,k3,stop);
        // result
        nextPoint = points2.back() + (k1 + k2*2.f + k3*2.f + k4) * stepSize_6;
        d = PropagationDirectionT2At(nextPoint,stop,interpolate,adaptor);
        d2 = SelectClosestDirectionT2(d,k1,stop);
        if(points2.size()*this->m_StepSize > this->m_MaximumLength)
        {
            stop = true;
        }
    }

    // Concatenate
    std::vector< PointType > points;
    if(!points2.empty())
    {
        std::copy(points2.rbegin(), points2.rend()-1, std::back_inserter(points));
    }
    std::copy(points1.begin(), points1.end(), std::back_inserter(points));

    return points;
}


} // namespace itk

#endif
