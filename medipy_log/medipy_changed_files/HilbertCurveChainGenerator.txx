/*************************************************************************
 * MediPy - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#include "HilbertCurveChainGenerator.h"

#include <algorithm>
#include <vector>

#include <itkImageRegionConstIteratorWithIndex.h>
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_vector.h>

template<typename TImage, typename TMask>
HilbertCurveChainGenerator<TImage, TMask>
::HilbertCurveChainGenerator()
{ 
    // Nothing to do.
}

template<typename TImage, typename TMask>
HilbertCurveChainGenerator<TImage, TMask>
::~HilbertCurveChainGenerator()
{
    // Nothing to do.
}

template<typename TImage, typename TMask>
void 
HilbertCurveChainGenerator<TImage, TMask>
::operator()(std::vector<ImageConstPointer> const & images, MaskConstPointer mask)
{
    //creation du parcours d'hilbert-peano
    unsigned int const cube_size = this->_find_cube_length(images[0]);
    
    Cube::Pointer const cube = this->_hilbert_peano_scan(cube_size);
    
    // Convert the cube to a list of offsets in cube
    typedef itk::ImageRegionConstIteratorWithIndex<Cube> CubeIteratorType;
    ScanType scan(cube->GetRequestedRegion().GetNumberOfPixels(), 0);
    ScanType::value_type offset=0;
    for(CubeIteratorType it(cube, cube->GetRequestedRegion()); !it.IsAtEnd(); ++it)
    {
        scan[it.Get()] = offset;
        ++offset;
    }
    
    // Convert the list of offsets in cube to a list of offsets in images[0],
    // using the mask and the image region.
    this->m_Scan.clear();
    for(ScanType::const_iterator it=scan.begin(); it!=scan.end(); ++it)
    {
        ScanType::value_type const & cube_offset = *it;
        Cube::IndexType const index = cube->ComputeIndex(cube_offset);
        
        if(images[0]->GetRequestedRegion().IsInside(index) && 
           (mask.IsNull() || mask->GetPixel(index) != 0))
        {
            typename ImageType::OffsetValueType const image_offset = 
                images[0]->ComputeOffset(index);
            this->m_Scan.push_back(image_offset);
        }
    }
    
    // Create the chains based on the scan
    this->m_ImageChains = ImageChainsType(images.size(), this->m_Scan.size(), 0);
    for(ScanType::size_type i=0; i<this->m_Scan.size(); ++i)
    {
        ScanType::value_type offset = this->m_Scan[i];
        typename ImageType::IndexType const index = images[0]->ComputeIndex(offset);
        for(unsigned int modality=0; modality!=images.size(); modality++)
        {
            this->m_ImageChains(modality, i) = images[modality]->GetPixel(index);
        }
    }
}

template<typename TImage, typename TMask>
typename HilbertCurveChainGenerator<TImage, TMask>::ImageChainsType const & 
HilbertCurveChainGenerator<TImage, TMask>
::GetImageChains() const
{
    return this->m_ImageChains;
}

template<typename TImage, typename TMask>
typename HilbertCurveChainGenerator<TImage, TMask>::ScanType const & 
HilbertCurveChainGenerator<TImage, TMask>
::GetScan() const
{
    return this->m_Scan;
}

template<typename TImage, typename TMask>
unsigned int 
HilbertCurveChainGenerator<TImage, TMask>
::_find_cube_length(ImageType const * image)
{
    typename ImageType::SizeType const & size = 
        image->GetRequestedRegion().GetSize();
	
    //on cherche le max
    unsigned int const max = *std::max_element(
        size.m_Size, size.m_Size+size.GetSizeDimension());

    //trouver la plus petite puissance de 2 superieure à max
    unsigned int power=1;
    while(power<max)
    {
        power *= 2;
    }

    return power;
}

template<typename TImage, typename TMask>
HilbertCurveChainGenerator<TImage, TMask>::Cube::Pointer
HilbertCurveChainGenerator<TImage, TMask>
::_hilbert_peano_scan(unsigned int cube_size)
{
    Cube::Pointer cube=Cube::New();
    {
        Cube::IndexType start; start.Fill(0);
        Cube::SizeType size; size.Fill(cube_size);
        Cube::RegionType region(start, size);
        cube->SetRegions(region);
        cube->Allocate();
    }

    //parcours de Peano dans un cube de taille 2
    if(cube_size==2)
    {
        Cube::IndexType index;
        index[0] = 0; index[1] = 0; index[2] = 0; cube->SetPixel(index, 0);
        index[0] = 0; index[1] = 0; index[2] = 1; cube->SetPixel(index, 1);
        index[0] = 0; index[1] = 1; index[2] = 1; cube->SetPixel(index, 2);
        index[0] = 0; index[1] = 1; index[2] = 0; cube->SetPixel(index, 3);
        index[0] = 1; index[1] = 1; index[2] = 0; cube->SetPixel(index, 4);
        index[0] = 1; index[1] = 1; index[2] = 1; cube->SetPixel(index, 5);
        index[0] = 1; index[1] = 0; index[2] = 1; cube->SetPixel(index, 6);
        index[0] = 1; index[1] = 0; index[2] = 0; cube->SetPixel(index, 7);
    }
    else
    {
        //position (3D) d'insertion du cube de degre inferieur
        int iOrig=0,jOrig=0,kOrig=0;
        
        //position (dans la chaine) de Orig
        int posOrig=0;
        
        //nombre de voxels ds le cube
        unsigned int const nbVox=std::pow((double)cube_size, 3);
        
        //taille du sous-cube 
        unsigned int const sub_cube_size=cube_size/2;
        
        //nombre de voxels ds le sous-cube
        int const nbVoxInf=std::pow((double)sub_cube_size, 3);
        
        Cube::Pointer sub_cube = Self::_hilbert_peano_scan(sub_cube_size);
      
        typedef itk::ImageRegionConstIteratorWithIndex<Cube> IteratorType;
        
        //calcul du demicube gauche (4 cubes de taille cube_size/2)
        for(IteratorType it(sub_cube, sub_cube->GetRequestedRegion());
            !it.IsAtEnd(); ++it)
        {
            Cube::IndexType const & source=it.GetIndex();
            
            Cube::IndexType destination;
            destination[0] = iOrig+source[2]; 
            destination[1] = jOrig+source[1];
            destination[2] = kOrig+source[0];
            
            cube->SetPixel(destination, posOrig+it.Get());
        }

        //definition du point d'insertion du prochain mini cube
        kOrig+=sub_cube_size;
		
        //maj du nombre de voxels inseres
        posOrig+=nbVoxInf;
        for(IteratorType it(sub_cube, sub_cube->GetRequestedRegion());
            !it.IsAtEnd(); ++it)
        {
            Cube::IndexType const & source=it.GetIndex();
            
            Cube::IndexType destination;
            destination[0] = iOrig+source[1]; 
            destination[1] = jOrig+source[0];
            destination[2] = kOrig+source[2];
            
            cube->SetPixel(destination, posOrig+it.Get());
        }

        //definition du point d'insertion du prochain mini cube
        jOrig+=sub_cube_size;
        //maj du nombre de voxels inseres
        posOrig+=nbVoxInf;
        for(IteratorType it(sub_cube, sub_cube->GetRequestedRegion());
            !it.IsAtEnd(); ++it)
        {
            Cube::IndexType const & source=it.GetIndex();
            
            Cube::IndexType destination;
            destination[0] = iOrig+source[0]; 
            destination[1] = jOrig+source[1];
            destination[2] = kOrig+source[2];
            
            cube->SetPixel(destination, posOrig+it.Get());
        }

        //definition du point d'insertion du prochain mini cube
        iOrig+=(sub_cube_size-1);
        kOrig--;
        //maj du nombre de voxels inseres
        posOrig+=nbVoxInf;
        for(IteratorType it(sub_cube, sub_cube->GetRequestedRegion());
            !it.IsAtEnd(); ++it)
        {
            Cube::IndexType const & source=it.GetIndex();
            
            Cube::IndexType destination;
            destination[0] = iOrig-source[1]; 
            destination[1] = jOrig+source[2];
            destination[2] = kOrig-source[0];
            
            cube->SetPixel(destination, posOrig+it.Get());
        }

        //le reste du cube est obtenu par symetrie
        for(unsigned int i=0;i!=sub_cube_size;i++)
        {
            for(unsigned int j=0;j!=cube_size;j++)
            {
                for(unsigned int k=0;k!=cube_size;k++)
                {
                    Cube::IndexType source;
                    source[0] = sub_cube_size-(i+1);
                    source[1] = j;
                    source[2] = k;
                    
                    Cube::IndexType destination;
                    destination[0] = sub_cube_size+i; 
                    destination[1] = j;
                    destination[2] = k;
                    
                    cube->SetPixel(destination, nbVox-(cube->GetPixel(source)+1));
                }
            }
        }
    }
    
    return cube;
}
