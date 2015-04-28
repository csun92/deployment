##########################################################################
# MediPy - Copyright (C) Universite de Strasbourg
# Distributed under the terms of the CeCILL-B license, as published by
# the CEA-CNRS-INRIA. Refer to the LICENSE file or to
# http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
# for details.
##########################################################################

import numpy

from base import Base

class Square(Base):
    """ Square-shaped brush.
    
        This brush is a 2D square brush, where the square plane is defined
        by its normal direction.
    """
    
    def __init__(self, value, radius, normal):
        self._radius = None
        self._normal = None
        self._indices = None
        
        super(Square, self).__init__(value)
        self._set_radius(radius)
        self._set_normal(normal)
    
    def indices(self, position=None):
        """ See :func:`medipy.gui.brushes.Base.indices`.
        """
        
        if position is None :
            position = numpy.zeros(len(self._normal))
        return numpy.add(self._indices, position)
    
    ##############
    # Properties #
    ##############
    
    def _get_radius(self):
        """ Radius in pixels.
        """
        
        return self._radius
    
    def _set_radius(self, radius):
        self._radius = radius
        exist = False
        for item in [self._radius, self._normal]:
            if item is None:
                exist = True
                break
        if not exist :
            self._compute_indices()
    
    def _get_normal(self):
        """ Normal of the square plane (must be unit-length). 
        """
        
        return self._normal
    
    def _set_normal(self, normal):
        self._normal = numpy.asarray(normal, float)
        exist = False
        for item in [self._radius, self._normal]:
            if item is None:
                exist = True
                break
        if not exist :
            self._compute_indices()
    
    radius = property(_get_radius, _set_radius)
    normal = property(_get_normal, _set_normal)

    #####################
    # Private interface #
    #####################
    
    def _compute_indices(self):
        diameter = 2*self._radius+1
        dimension = len(self._normal)
        
        first = numpy.asarray(dimension*[-self._radius], float)
        last = numpy.asarray(dimension*[self._radius], float)
        
        bounding_box = numpy.transpose(numpy.indices(last-first+1))+first
        bounding_box = bounding_box.reshape(diameter**dimension, dimension)
        
        # Index must be in the plane defined by the normal (dot(x,normal))
        # and less than radius from origin
        self._indices = [x.astype(int) for x in bounding_box 
                         if numpy.abs(numpy.dot(x, self._normal)) < 0.5 and
                            numpy.max(x)<=self._radius]
        
