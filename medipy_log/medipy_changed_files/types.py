##########################################################################
# MediPy - Copyright (C) Universite de Strasbourg
# Distributed under the terms of the CeCILL-B license, as published by
# the CEA-CNRS-INRIA. Refer to the LICENSE file or to
# http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
# for details.
##########################################################################

import ctypes

import itk
import numpy

import medipy.base

#: Convert ImageIOBase::IOComponentType to its corresponding ITK type
io_component_type_to_type = {
    itk.ImageIOBase.UCHAR : itk.UC,
    itk.ImageIOBase.CHAR : itk.SC,
    itk.ImageIOBase.USHORT : itk.US,
    itk.ImageIOBase.SHORT : itk.SS,
    itk.ImageIOBase.UINT : itk.UI,
    itk.ImageIOBase.INT : itk.SI,
    itk.ImageIOBase.ULONG : itk.UL,
    itk.ImageIOBase.LONG : itk.SL,
    itk.ImageIOBase.FLOAT : itk.F,
    itk.ImageIOBase.DOUBLE : itk.D,
}

#: Map an ITK type to its immediately smaller ITK type
smaller_type = {
    itk.US : itk.UC,
    itk.UL : itk.US,
    itk.UI : itk.US,
    itk.SS : itk.SC,
    itk.SL : itk.SS,
    itk.SI : itk.SS,
    itk.D : itk.F
}

#: Map an ITK type to its immediately larger ITK type
larger_type = {
    itk.UC : itk.US,
    itk.US : itk.UL,
    itk.UI : itk.UL,
    itk.SC : itk.SS,
    itk.SS : itk.SL,
    itk.SI : itk.SL,
    itk.F : itk.D
}

def get_smaller_types(type_):
    """ Return the list of ITK types smaller than given type.
    """
    
    smaller_types = []
    while type_ in medipy.itk.types.smaller_type:
        type_ = medipy.itk.types.smaller_type[type_]
        smaller_types.append(type_)
    
    return smaller_types

def get_larger_types(type_):
    """ Return the list of ITK types larger than given type.
    """
    
    larger_types = []
    while type_ in medipy.itk.types.larger_type:
        type_ = medipy.itk.types.larger_type[type_]
        larger_types.append(type_)
    
    return larger_types

def get_numpy_type(c_type):
    """ Return the numpy type associated with the given C type, given as a
        string. For example:
        
        >>> get_numpy_type("unsigned long")
        numpy.uint64
        >>> get_numpy_type("float")
        numpy.float32
    """
    
    # ctypes.c_XXX do not have spaces in their names
    c_type = c_type.replace(" ", "")
    
    signedness = "u" if c_type.startswith("unsigned") else ""
    
    family = None
    integers = ["char", "short", "int", "long", "longlong"]
    floating_points = ["float", "double", "longdouble"]
    if c_type.startswith("unsigned") :
        if c_type[len("unsigned"):] in integers :
            family = "int"
        elif c_type[len("unsigned"):] in floating_points :
            family = "float"
    else :
        if c_type in integers :
            family = "int"
        elif c_type in floating_points :
            family = "float"
    
    # "unsigned xxx" are named "uxxx" in ctypes
    if c_type.startswith("unsigned") :
        c_type = "u"+c_type[len("unsigned"):]
    
    # "char" is named "byte" in ctypes
    if c_type.endswith("char") :
        c_type = c_type[:-len("char")]+"byte"
    
    c_type = "c_"+c_type
    if c_type not in dir(ctypes) :
        raise medipy.base.Exception("Unknown C type: {0!r}".format(c_type))
    
    size = 8*ctypes.sizeof(getattr(ctypes, c_type))
    numpy_type = "{signedness}{family}{size}".format(**locals())
    if numpy_type not in dir(numpy) :
        raise medipy.base.Exception("Unknown numpy type: {0!r}".format(numpy_type))
    
    return getattr(numpy, numpy_type)

def _get_type_dictionaries():
    itk_to_dtype_table = [
        # Unsigned int types (char, short, long)
        (itk.UC, get_numpy_type("unsigned char")),
        (itk.US, get_numpy_type("unsigned short")),
        (itk.UI, get_numpy_type("unsigned int")),
        (itk.UL, get_numpy_type("unsigned long")),
        # Signed int types (char, short, long)
        (itk.SC, get_numpy_type("char")),
        (itk.SS, get_numpy_type("short")),
        (itk.SI, get_numpy_type("int")),
        (itk.SL, get_numpy_type("long")),
        # Float types (float, double, long double)
        (itk.F, get_numpy_type("float")),
        (itk.D, get_numpy_type("double")),
        # On Windows 32 bits, the numpy types for double and long double
        # are the same. Add long double only if different
        # Complex types
        #(itk.COMPLEX_REALS, ),
        # Vector types
        #(itk.VECTOR_REALS, ),
        # RGB types
        #(itk.RGBS, ),
        # RGBA types
        #(itk.RGBAS, ),
        # Covariant vector types
        #(itk.COV_VECTOR_REALS, ),
    ]

    if get_numpy_type("double") != get_numpy_type("long double"):
        itk_to_dtype_table.append((itk.LD, get_numpy_type("long double")))
    
    #: Map an ITK type to a NumPy dtype
    itk_to_dtype = {}
    
    #: Map a NumPy dtype to an ITK type
    dtype_to_itk = {}
    
    for itk_type, dtype in itk_to_dtype_table :
        itk_to_dtype[itk_type] = dtype
        dtype_to_itk[dtype] = itk_type
    
    return itk_to_dtype, dtype_to_itk

#: Map ITK types to and from numpy dtypes
itk_to_dtype, dtype_to_itk = _get_type_dictionaries()
