##########################################################################
# MediPy - Copyright (C) Universite de Strasbourg
# Distributed under the terms of the CeCILL-B license, as published by
# the CEA-CNRS-INRIA. Refer to the LICENSE file or to
# http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
# for details.
##########################################################################

import fnmatch

from vtk import (vtkDataReader, vtkFloatArray, vtkStructuredPoints, 
                 vtkStructuredPointsReader, vtkStructuredPointsWriter)
import vtk.util.numpy_support

import numpy

import medipy.base
import medipy.io
from utils import dti6to33, dti33to6, rotation33todt6

class Vtk(medipy.io.IOBase) :
    """I/O class for Vtk format.
    
    Uses vtk library."""
    
    filenames = ["*.vtk"]
    
    def __init__(self, *args, **kwargs):
        medipy.io.IOBase.__init__(self, *args, **kwargs)
        self._filename = str(self.filename)
        self._image = None
    
    def can_load(self) :
        with open(self.filename, 'rb') as fp:
            Data = fp.read()
            size = len(Data)

        generic_reader = vtkDataReader()
        generic_reader.GlobalWarningDisplayOff()
        generic_reader.ReadFromInputStringOn()
        generic_reader.SetBinaryInputString(Data, size)

        return (generic_reader.OpenVTKFile() and 
                generic_reader.ReadHeader() and
                generic_reader.IsFileStructuredPoints() and
                generic_reader.GetNumberOfTensorsInFile()>0)
    
    def number_of_images(self) :
        return 1
    
    def load_metadata(self, index=0) :
        if self._image is None :
            self._read()
        metadata = {
            "data_type" : "vector",
            "image_type" : "tensor_2",
            "origin" : self._image.GetOrigin()[::-1],
            "spacing" : self._image.GetSpacing()[::-1],
            # Since VTK images are not oriented, direction is identity
            "direction" : numpy.identity(3),
        }
        return metadata
    
    def load_data(self, index=0) :
        if self._image is None :
            self._read()
        
        vtk_array = self._image.GetPointData().GetTensors()
        array = vtk.util.numpy_support.vtk_to_numpy(vtk_array)
        # Reshape to matrix instead of vector of length 9
        array = array.reshape(array.shape[:-1]+(3,3))
        array = dti33to6(array)
        
        vtk_shape = list(1+numpy.subtract(self._image.GetExtent()[1::2],
                                          self._image.GetExtent()[0::2]))
        array = array.reshape(vtk_shape[::-1]+[6,])
        
        return array
    
    def can_save(self, image) :
        found = False
        for pattern in self.filenames :
            if fnmatch.fnmatch(self.filename, pattern):
                found = True
                break
        
        return (found and image.image_type == "tensor_2")
    
    def save(self, image) :
        data = dti6to33(self._to_axis_aligned_lps_space(image))
        spacing = image.spacing
        origin = image.origin

        # Convert numpy -> VTK table
        vtk_array = vtkFloatArray()
        vtk_array.SetNumberOfComponents(9)
        vtk_array.SetVoidArray(data, len(data.ravel()), 1)

        # Create VTK dataset
        structured_points = vtkStructuredPoints()
        structured_points.SetDimensions(data.shape[2],data.shape[1],data.shape[0])
        structured_points.SetSpacing(spacing[2],spacing[1],spacing[0])
        structured_points.SetOrigin(origin[2],origin[1],origin[0])
        structured_points.GetPointData().SetTensors(vtk_array)

        # Write VTK file
        writer = vtkStructuredPointsWriter()
        writer.SetFileName(self._filename)
        writer.SetFileTypeToBinary()
        writer.SetInput(structured_points)
        writer.Update()

    ##############
    # Properties #
    ##############
    
    def _set_filename(self, filename):
        self._filename = filename
        self._image = None

    #####################
    # Private interface #
    #####################
    
    def _to_axis_aligned_lps_space(self, image):
        """ Transform a tensor image to the closest axis-aligned approximation 
            of LPS (i.e. DICOM) space.
        """
        
        direction = numpy.linalg.inv(image.direction)
        return rotation33todt6(image.data, direction)
    
    def _read(self):
        reader = vtkStructuredPointsReader()
        reader.SetFileName(self.filename)
        reader.Update()
        self._image = reader.GetOutput()

# Add the new IO class to the known classes
import medipy.io.schemes
medipy.io.schemes.file.io_classes.insert(0, Vtk)
