##########################################################################
# MediPy - Copyright (C) Universite de Strasbourg
# Distributed under the terms of the CeCILL-B license, as published by
# the CEA-CNRS-INRIA. Refer to the LICENSE file or to
# http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
# for details.
##########################################################################


import copy
import logging

import numpy

import exception
from observable import Observable
from observable_list import ObservableList

class Image(Observable):
    """ n-dimensional image class, based on numpy.ndarray. An image contains
            * an array of voxel data (image.data), which can also be directly
              accessed : image.data[something] can be written as image[something]
            * a transformation to convert the voxel space to and from the physical
              space (origin, direction, spacing). A voxel index I is converted to
              a physical point P by : P = direction * spacing * I + origin. If not
              specified, origin is 0, spacing is 1 and direction is identity. 
            * a dictionary of metadata
        
        The data has two kinds of interpretation :
            * data_type : whether the data is scalar, vector or matrix
            * image_type : how the image should be interpreted, can be unspecified
              (normal), "rgb", "tensor_2", or spectroscopy 
        
        If data_type == "vector" or data_type == "matrix", an array of
        dimension N will have a spacing of size N-1 (resp. N-2)
        
        >>> image1 = Image((40, 128, 128, 6), data_type="scalar")
        >>> image2 = Image((40, 128, 128, 6), data_type="vector")
        >>> print image1.spacing, image2.spacing
        [ 1.  1.  1.  1.] [ 1.  1.  1.]
        
        DTI images can be conveniently created using the dti argument, 
        resulting in a vector image with the correct number of components :
        >>> image = Image((40, 128, 128), dti="tensor_2")
        >>> print image.shape, image.data_type, image.spacing
        (40, 128, 128, 6) vector [ 1.  1.  1.]
        
        The following events may be fired : 
            * modified
    """
    
    def __init__(self, shape=(0,), dtype=numpy.single, 
                 spacing=None, origin=None, direction=None,
                 data_type="scalar", image_type="normal",
                 annotations = None, metadata=None,
                 **kwargs):
        
        """ Create an image. Extra arguments may contain :
              * data or value (mutually exclusive)
              * dti
              * any other argument of numpy.ndarray.__init__
        """
        
        ##################
        # Public members #
        ##################
        
        # Numpy array holding the voxel values
        self.data = None
        
        # Type/dimensionality of the data : can be one of scalar, vector, or matrix
        self.data_type = data_type
        
        # Acquisition modality of the image : can be normal or spectroscopy. TODO : use metadata
        self.image_type = image_type
        
        # Annotations on the image. TODO : store them in metadata
        self.annotations = ObservableList(annotations or [])
        
        # Metadata
        self.metadata = metadata or {}
        
        ############################
        # Property-related members #
        ############################
        
        # Spacing between pixels, in mm
        self._spacing = None
        
        # Origin of the image, in mm
        self._origin = None
        
        # Matrix of direction cosines that specify the direction between samples
        self._direction = None
        
        self._index_to_physical_matrix = None
        self._physical_to_index_matrix = None
        
        ##################
        # Initialization #
        ##################
        Observable.__init__(self, ["modified"])
        
        # Modify the shape of DTI images
        if "dti" in kwargs :
            self.data_type = "vector"
            if kwargs["dti"] == "tensor_2" :
                self.image_type = "tensor_2"
                shape = list(shape)
                shape.append(6)
#            elif kwargs["dti"] == "tensor_4" :
#                pass
            else :
                raise exception.Exception("Unkown DTI model : %s"%kwargs["dti"])
            del kwargs["dti"]
        
        # Array initialization : either data or value
        if "data" in kwargs and "value" in kwargs :
            raise exception.Exception("Only one of data and value is allowed")
        if "data" in kwargs :
            self.data = numpy.asarray(kwargs["data"])
            del kwargs["data"]
        else :
            if "value" in kwargs :
                value = kwargs["value"]
                del kwargs["value"] 
            else :
                value = None
            self.data = numpy.ndarray(shape, dtype, **kwargs)
            if value is not None : 
                self.data.fill(value)
        
        if spacing is None :
            self._set_spacing(self._default_spacing())
        else :
            self._set_spacing(spacing)
        
        if origin is None :
            self._set_origin(self._default_origin())
        else :
            self._set_origin(origin)
        
        if direction is None :
            self._set_direction(self._default_direction())
        else :
            self._set_direction(direction)
    
    
    def modified(self):
        """ Send a modified event to the observers
        """
        
        self.notify_observers("modified")
    
    def is_inside(self, position):
        """ Test if given position is inside the image
        """
        
        shape = numpy.hstack([numpy.ones(len(position)-self.ndim), self.data.shape])
        
        for i, p in enumerate(position) :
            if not 0 <= p < shape[i] :
                return False
        
        return True
    
    def copy_information(self, source):
        """ Copy information from source image to self
        """
        self.spacing = copy.copy(source.spacing)
        self.origin = copy.copy(source.origin)
        self.direction = copy.copy(source.direction)
        self.metadata = copy.copy(source.metadata)
    
    def astype(self, dtype):
        """ Copy of the image, cast to a specified type.
        """
        other_image = Image(data = self.data.astype(dtype),
            data_type=self.data_type, image_type=self.image_type)
        other_image.copy_information(self)
        return other_image
    
    def __getitem__(self, where):
        return self.data[where]
    
    def __setitem__(self, where, value):
        self.data[where] = value
    
    def __array__(self):
        """Return data as a numpy array."""
        return numpy.asarray(self.data)
    
    def index_to_physical(self, index):
        """ Transform an index coordinate to a physical coordinate.
        """
        
        return numpy.dot(self._index_to_physical_matrix, index)+self._origin
    
    def physical_to_index(self, physical, round=False):
        """ Transform a physical coordinate to an index coordinate. If ``round``
            is ``True``, the index will be rounded and cast to the nearest 
            integer value, otherwise it will be kept as a floating-point value.
        """
        
        index = numpy.dot(self._physical_to_index_matrix, physical-self._origin)
        if round :
            index = index.round().astype(int)
        return index
    
    ##############
    # Properties #
    ##############
    
    def _get_spacing(self):
        """ Image spacing in millimeters.
        """
        
        return self._spacing
    
    def _set_spacing(self, value):
        self._spacing = numpy.asarray(value, dtype=numpy.single)
        self._compute_index_to_physical_matrix()
    
    def _get_origin(self):
        """ Physical coordinates in millimeters of the voxel with index 
            coordinates :math:`(0,0,0)`.
        """
        
        return self._origin
    
    def _set_origin(self, value):
        self._origin = numpy.asarray(value, dtype=numpy.single)
    
    def _get_direction(self):
        """ Direction cosine matrix.
        """
        
        return self._direction
    
    def _set_direction(self, value):
        self._direction = numpy.asarray(value, dtype=numpy.single)
        self._compute_index_to_physical_matrix()
    
    def _get_shape(self):
        """ Shape of the image. This property accounts for non-scalar data, i.e.
            
            >>> image = medipy.base.Image(shape=(256, 256, 256, 3), data_type="scalar", image_type="rgb")
            >>> image.shape
            (256, 256, 256)
        """
        
        if self.data_type == "scalar" :
            return self.data.shape
        elif self.data_type == "vector" :
            return self.data.shape[:-1]
        else :
            raise medipy.base.Exception("Unknown data_type: {0}".format(self.data_type))
    
    def _get_dtype(self):
        """ dtype of the underlying array.
        """
        
        return self.data.dtype
    
    def _get_ndim(self):
        """ Number of dimensions in the image. This property accounts for 
            non-scalar data, i.e.
            
            >>> image = medipy.base.Image(shape=(256, 256, 256, 3), data_type="scalar", image_type="rgb")
            >>> image.ndim
            3 
        """
        
        if self.data_type == "scalar" :
            return self.data.ndim
        elif self.data_type == "vector" :
            return self.data.ndim-1
        else :
            raise medipy.base.Exception("Unknown data_type: {0}".format(self.data_type))
    
    def _get_computed_ndim(self):
        """ Return the dimensionality of the image, neglecting the first values
            that are equal to 1. An image with a shape of (1,1,256,256,128) will
            have a ndim of 5 and a computed_ndim of 3. For this image, the 
            computed_ndim only considers (256,256,128).
        """
        
        shape = list(self.shape)
        while shape and shape[0] == 1 :
            shape = shape[1:]
        
        return len(shape)
    
    def _get_number_of_components(self):
        """ Return the number of scalar components per voxel.
        """
        
        if self.data_type == "scalar" :
            return 1
        elif self.data_type == "vector" :
            return self.data.shape[-1]
        else :
            raise medipy.base.Exception("Unknown data_type: {0}".format(self.data_type))
        
    
    spacing = property(_get_spacing, _set_spacing)
    origin = property(_get_origin, _set_origin)
    direction = property(_get_direction, _set_direction)
    shape = property(_get_shape)
    dtype = property(_get_dtype)
    ndim = property(_get_ndim)
    computed_ndim = property(_get_computed_ndim)
    number_of_components = property(_get_number_of_components)
    
    #####################    
    # Private interface #
    #####################
    
    def _compute_index_to_physical_matrix(self):
        exist = False
        for item in [self._spacing, self._origin]:
            if item is None:
                exist = True
                break
        if not exist :
            try :
                self._index_to_physical_matrix = numpy.dot(
                    self._direction, numpy.diag(self._spacing))
            except ValueError,e :
                self._index_to_physical_matrix = None
                self._physical_to_index_matrix = None
                #logging.warning("Could not compute index to physical matrix : {0}".format(e))
            else :
                self._physical_to_index_matrix = numpy.linalg.inv(self._index_to_physical_matrix)
    
    def _default_spacing(self):
        """ Return the default image spacing, according to the data type.
        """
        
        dim = None
        if self.data_type == "scalar" :
            dim = self.data.ndim
        elif self.data_type == "vector" :
            dim = self.data.ndim-1
        elif self.data_type == "matrix" :
            dim = self.data.ndim-2
        else :
            raise exception.Exception("Unknown data_type : %s"%self.data_type)
        return numpy.ones(dim, dtype=numpy.float32)
    
    def _default_origin(self):
        """ Return the default image origin, according to the data type.
        """
        
        dim = None
        if self.data_type == "scalar" :
            dim = self.data.ndim
        elif self.data_type == "vector" :
            dim = self.data.ndim-1
        elif self.data_type == "matrix" :
            dim = self.data.ndim-2
        else :
            raise exception.Exception("Unknown data_type : %s"%self.data_type)
        return numpy.zeros(dim, dtype=numpy.float32)
    
    def _default_direction(self):
        """ Return the default image direction, according to the data type.
        """
        
        dim = None
        if self.data_type == "scalar" :
            dim = self.data.ndim
        elif self.data_type == "vector" :
            dim = self.data.ndim-1
        elif self.data_type == "matrix" :
            dim = self.data.ndim-2
        else :
            raise exception.Exception("Unknown data_type : %s"%self.data_type)
        return numpy.identity(dim, dtype=numpy.float32)
