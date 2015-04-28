import filecmp
import os
import shutil
import tempfile
import unittest
import sys

import numpy

import medipy.io.dicom

class TestEncapsulatedDocument(unittest.TestCase):
    
    def generate_file(self, destination, size):
        # Random return float, we don't need as many bytes
        itemsize = numpy.dtype(float).itemsize
        data = numpy.random.random((1+size/itemsize)).tostring()[:size]
        
        fd = open(destination, "wb")
        fd.write(data)
        fd.close()
    
    def test_encapsulate_even_size(self):
        temporary_directory = tempfile.mkdtemp()
        
        input = os.path.join(temporary_directory, "input")
        self.generate_file(input, 40000)
        
        dataset = medipy.io.dicom.encapsulated_document.encapsulate(input, 
            mime_type_of_encapsulated_document="foo/bar")
        self._test_encapsulate(dataset, "foo/bar", "EVEN")
        
        shutil.rmtree(temporary_directory)

    def test_encapsulate_odd_size(self):
        temporary_directory = tempfile.mkdtemp()
        
        input = os.path.join(temporary_directory, "input")
        self.generate_file(input, 40001)
        
        dataset = medipy.io.dicom.encapsulated_document.encapsulate(input, 
            mime_type_of_encapsulated_document="foo/bar")
        self._test_encapsulate(dataset, "foo/bar", "ODD")
    
        shutil.rmtree(temporary_directory)

    @unittest.skipIf(sys.platform.startswith("win"), "windows has no program \"file\"")
    def test_expose_even_size(self):
        temporary_directory = tempfile.mkdtemp()
        
        input = os.path.join(temporary_directory, "input")
        output = os.path.join(temporary_directory, "output")
        self.generate_file(input, 40000)
        
        dataset = medipy.io.dicom.encapsulated_document.encapsulate(input)
        medipy.io.dicom.encapsulated_document.expose(dataset, output)
        
        self.assertTrue(filecmp.cmp(input, output, False))
    
        shutil.rmtree(temporary_directory)

    @unittest.skipIf(sys.platform.startswith("win"), "windows has no program \"file\"")
    def test_expose_odd_size(self):
        temporary_directory = tempfile.mkdtemp()
        
        input = os.path.join(temporary_directory, "input")
        output = os.path.join(temporary_directory, "output")
        self.generate_file(input, 40001)
        
        dataset = medipy.io.dicom.encapsulated_document.encapsulate(input)
        medipy.io.dicom.encapsulated_document.expose(dataset, output)
        
        self.assertTrue(filecmp.cmp(input, output, False))
    
        shutil.rmtree(temporary_directory)

    def _test_encapsulate(self, dataset, mime_type, parity):
        
        # Document Title
        self.assertTrue("document_title" in dataset)
        document_title = dataset.document_title
        self.assertTrue(isinstance(document_title, medipy.io.dicom.ST))
        self.assertTrue(isinstance(document_title.value, str))
        self.assertEqual(document_title.value, "input")
        
        # MIME type
        self.assertTrue("mime_type_of_encapsulated_document" in dataset)
        mime_type_of_encapsulated_document = dataset.mime_type_of_encapsulated_document
        self.assertTrue(isinstance(mime_type_of_encapsulated_document, medipy.io.dicom.LO))
        self.assertEqual(mime_type_of_encapsulated_document.value, mime_type)
        
        # File length parity
        self.assertTrue("document_class_code_sequence" in dataset)
        document_class_code_sequence = dataset.document_class_code_sequence
        self.assertTrue(isinstance(document_class_code_sequence, medipy.io.dicom.SQ))
        self.assertEqual(len(document_class_code_sequence.value), 1)
        
        code = document_class_code_sequence.value[0]
        self.assertTrue(isinstance(code, medipy.io.dicom.DataSet))
        
        self.assertTrue("code_value" in code)
        self.assertTrue(isinstance(code.code_value, medipy.io.dicom.SH))
        self.assertTrue(isinstance(code.code_value.value, str))
        self.assertEqual(code.code_value.value, "LENGTH_PARITY:{0}".format(parity))
        
        self.assertTrue("coding_scheme_designator" in code)
        self.assertTrue(isinstance(code.coding_scheme_designator, medipy.io.dicom.SH))
        self.assertTrue(isinstance(code.coding_scheme_designator.value, str))
        self.assertEqual(code.coding_scheme_designator.value, "MEDIPY")
        
        self.assertTrue("code_meaning" in code)
        self.assertTrue(isinstance(code.code_meaning, medipy.io.dicom.LO))
        self.assertTrue(isinstance(code.code_meaning.value, str))
        self.assertEqual(code.code_meaning.value, "LENGTH_PARITY:{0}".format(parity))
        
        # Encapsulated document
        self.assertTrue("encapsulated_document" in dataset)
        self.assertTrue(isinstance(dataset.encapsulated_document, medipy.io.dicom.OB))
        self.assertTrue(isinstance(dataset.encapsulated_document.value, str))
    
if __name__ == "__main__" :
    unittest.main()
