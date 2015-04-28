##########################################################################
# MediPy - Copyright (C) Universite de Strasbourg
# Distributed under the terms of the CeCILL-B license, as published by
# the CEA-CNRS-INRIA. Refer to the LICENSE file or to
# http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
# for details.
##########################################################################

import re
import os
import subprocess
import sys

import medipy.base
from dataset import DataSet
from vr import *

def encapsulate(filename, **kwargs):
    """ Create a Data Set encapsulating the content of a file. The generated
        Data Set only contains the Encapsulated Document modules, i.e.
        Encapsulated Document Series (PS 3.3-2011, C.24.1, p. 1328) and
        Encapsulated Document (PS 3.3-2011, C.24.2, p. 1329).
        
        The following elements get a default value if not specified in kwargs :
          * modality: defaults to OT
          * series_instance_uid: defaults to a new UID
          * series_number: defaults to 1
          * instance_number: defaults to 1
          * content_date: defaults to ""
          * acquisition_date_time: defaults to ""
          * burned_in_annotation: defaults to YES (assume that the encapsulated
            document *can* identify the patient)
          * document_title: defaults to the basename of the ``filename``
          * concept_name_code_sequence: defaults to []
          * document_class_code_sequence: defaults to a sequence containing the
            length parity information (see below).
          * mime_type_of_encapsulated document: defaults to a guessed MIME type
            using the contents of the file.
        
        Since the OB VR must be padded to be of even length, we need to be able
        to determine whether the original file has an even or odd length. This 
        is done by adding an item in Document Class Code Sequence (0040,e008). 
        The item has a Code Value (0008,0100) and Code Meaning (0008,0104) of
        ``"LENGTH_PARITY:EVEN"`` or ``"LENGTH_PARITY:ODD"``, and a Coding Scheme 
        Designator (0008,0102) of ``"MEDIPY"``.
        
        Example : ::
        
            dataset = medipy.io.dicom.encapsulated_document.encapsulate(filename)

            # Use Raw Data Storage as a SOP class since Encapsulated Document SOP classes
            # are limited to PDF and HL7 documents
            dataset.header.media_storage_sop_class_uid = medipy.io.dicom.UI(
                medipy.io.dicom.dictionary.uid_name_dictionary["raw_data_storage"])
            dataset.sop_class_uid = medipy.io.dicom.UI(
                medipy.io.dicom.dictionary.uid_name_dictionary["raw_data_storage"])
            
            # New file, hence new SOP Instance UID
            dataset.sop_instance_uid = medipy.io.dicom.generate_uid()
            
            # Most DICOM nodes will complain without Patient information
            dataset.patient_id = medipy.io.dicom.LO(medipy.io.dicom.generate_uid(False))
            dataset.patients_name = medipy.io.dicom.PN(dataset.patient_id.value)
            
            # Generate a new Study Instance UID
            dataset.study_instance_uid = medipy.io.dicom.generate_uid()
            
            connection = medipy.network.dicom.Connection(host, port, aet, aec)
            store = medipy.network.dicom.scu.Store(connection, dataset)
            store()
         
    """
    
    # Encapsulated Document Series (PS 3.3-2011, C.24.1, p. 1328)
    kwargs.setdefault("modality", "OT")
    kwargs.setdefault("series_instance_uid", medipy.io.dicom.generate_uid())
    kwargs.setdefault("series_number", 1)
    
    # Encapsulated Document (PS 3.3-2011, C.24.2, p. 1329)
    kwargs.setdefault("instance_number", 1)
    kwargs.setdefault("content_date", "")
    kwargs.setdefault("acquisition_date_time", "")
    kwargs.setdefault("burned_in_annotation", "YES") 
    kwargs.setdefault("document_title", os.path.basename(filename))
    kwargs.setdefault("concept_name_code_sequence", [])
    
    kwargs.setdefault("document_class_code_sequence", [])
    length_parity = os.stat(filename).st_size%2
    length_parity = "LENGTH_PARITY:{0}".format({0:"EVEN", 1:"ODD"}[length_parity])
    length_parity_ds = medipy.io.dicom.DataSet()
    length_parity_ds.code_value = SH(length_parity)
    length_parity_ds.coding_scheme_designator = SH("MEDIPY")
    length_parity_ds.code_meaning = LO(length_parity)
    kwargs["document_class_code_sequence"].append(length_parity_ds)
    
    if not kwargs.setdefault("mime_type_of_encapsulated_document", None) :
        if sys.platform != "win32":
            process = subprocess.Popen(["file", "--mime-type", "--brief", filename],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()
            if process.returncode != 0 :
                raise medipy.base.Exception("Cannot determine MIME type: {0!r}".format(stderr))
            kwargs["mime_type_of_encapsulated_document"] = stdout
        else:
            raise medipy.base.Exception("Windows cannot determine MIME type.")
    
    fd = open(filename, "rb")
    data = fd.read()
    fd.close()    
    
    dataset = DataSet()
    
    # Encapsulated Document Series (PS 3.3-2011, C.24.1, p. 1328)
    _set_element(kwargs, "modality", 1, medipy.io.dicom.CS, dataset)
    _set_element(kwargs, "series_instance_uid", 1, medipy.io.dicom.UI, dataset)
    _set_element(kwargs, "series_number", 1, medipy.io.dicom.IS, dataset)
    _set_element(kwargs, "referenced_performed_procedure_step_sequence", 
                 3, medipy.io.dicom.SQ, dataset)
    _set_element(kwargs, "series_description", 3, medipy.io.dicom.LO, dataset)
    _set_element(kwargs, "series_description_code_sequence", 
                 3, medipy.io.dicom.SQ, dataset)
    _set_element(kwargs, "requested_attributes_sequence", 
                 3, medipy.io.dicom.SQ, dataset)
    # TODO : Performed Procedure Step Summary
    
    # Encapsulated Document (PS 3.3-2011, C.24.2, p. 1329)
    _set_element(kwargs, "instance_number", 1, medipy.io.dicom.IS, dataset)
    _set_element(kwargs, "content_date", 2, medipy.io.dicom.DA, dataset)
    _set_element(kwargs, "acquisition_date_time", 2, medipy.io.dicom.DT, dataset)
    _set_element(kwargs, "image_laterality", 3, medipy.io.dicom.CS, dataset)
    _set_element(kwargs, "burned_in_annotation", 1, medipy.io.dicom.CS, dataset)
    _set_element(kwargs, "recognizable_visual_features", 3, medipy.io.dicom.CS, dataset)
    _set_element(kwargs, "source_instance_sequence", "1C", medipy.io.dicom.SQ, dataset)
    _set_element(kwargs, "document_title", 2, medipy.io.dicom.ST, dataset)
    _set_element(kwargs, "concept_name_code_sequence", 2, medipy.io.dicom.SQ, dataset)
    _set_element(kwargs, "document_class_code_sequence", 3, medipy.io.dicom.SQ, dataset)
    _set_element(kwargs, "verification_flag", 3, medipy.io.dicom.CS, dataset)
    _set_element(kwargs, "hl7_instance_identifier", "1C", medipy.io.dicom.ST, dataset)
    _set_element(kwargs, "mime_type_of_encapsulated_document", 1, medipy.io.dicom.LO, dataset)
    _set_element(kwargs, "list_of_mime_types", "1C", medipy.io.dicom.LO, dataset)
    dataset.encapsulated_document = OB(data)
    
    return dataset

def expose(dataset, filename):
    """ Save the Encapsulated Document in the dataset to a file. See the remark
        in :func:`encapsulate` concerning the parity of the file size.
    """
    
    parity = None
    for item in dataset.get("document_class_code_sequence", SQ([])).value :
        if item.get("coding_scheme_designator", "") == "MEDIPY" :
            match = re.match(r"LENGTH_PARITY:(EVEN|ODD)", item.code_value.value)
            if match :
                parity = match.group(1)
    
    fd = open(filename, "wb")
    if parity == "ODD" and len(dataset.encapsulated_document.value)%2==0 :
        fd.write(dataset.encapsulated_document.value[:-1])
    else :
        fd.write(dataset.encapsulated_document.value)
    fd.close()

def _set_element(kwargs, name, type, vr, dataset):
    if type in [1, 2] and name not in kwargs :
        raise medipy.base.Exception("{0} must be specified".format(name))
    if name not in kwargs :
        # Type 3 or 1C : it's up to the user to specify those. Since he didn't
        # specify any value, do nothing.
        return
    
    # Encapsulate the value in a VR if not already done
    value = kwargs[name]
    if isinstance(value, medipy.io.dicom.VR) and not isinstance(value, vr) :
        raise medipy.base.Exception("{0} must be of VR {1}".format(name, vr.__name__))
    elif not isinstance(value, medipy.io.dicom.VR) :
        value = vr(value)
    
    dataset[name] = value
