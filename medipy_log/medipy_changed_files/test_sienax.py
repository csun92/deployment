import os
import unittest
import sys

import medipy.fsl

@unittest.skipIf(sys.platform.startswith("win"), "windows fsl test skipped")
class TestSienax(unittest.TestCase):
    
    def setUp(self):
        self.sienax = medipy.fsl.Sienax()
        self.sienax.input = "foo.nii"
    
    def test_basic(self):
        self.assertEqual(self.sienax.command, ["sienax", "foo.nii"])
    
    def test_output_directory(self):
        self.sienax.output_directory = "bar"
        self.assertEqual(self.sienax.command, ["sienax", "foo.nii", "-o", "bar"])
    
    def test_two_class_segmentation(self):
        self.sienax.two_class_segmentation = True
        self.assertEqual(self.sienax.command, ["sienax", "foo.nii", "-2"])
    
    def test_t2_weighted_input(self):
        self.sienax.t2_weighted_input = True
        self.assertEqual(self.sienax.command, ["sienax", "foo.nii", "-t2"])
    
    def test_ignore_upwards(self):
        self.sienax.ignore_upwards = 10
        self.assertEqual(self.sienax.command, ["sienax", "foo.nii", "-t", "10"])
    
    def test_ignore_downwards(self):
        self.sienax.ignore_downwards = 10
        self.assertEqual(self.sienax.command, ["sienax", "foo.nii", "-b", "10"])
    
    def test_regional(self):
        self.sienax.regional = True
        self.assertEqual(self.sienax.command, ["sienax", "foo.nii", "-r"])
    
    def test_lesion_mask(self):
        self.sienax.lesion_mask = "bar.nii"
        self.assertEqual(self.sienax.command, ["sienax", "foo.nii", "-lm", "bar.nii"])
    
    def test_parse_report(self):
        report = self.sienax.parse_report(os.path.join(os.path.dirname(__file__), "report.sienax"))
        
        self.assertEqual(report.get("version", None), "2.6")
        self.assertEqual(report.get("vscale", None), 1.2580182223)
        
        self.assertEqual(report.get("grey", {}).get("normalized", None), 724097.23) 
        self.assertEqual(report.get("grey", {}).get("raw", None), 575585.64)
        
        self.assertEqual(report.get("white", {}).get("normalized", None), 639077.80) 
        self.assertEqual(report.get("white", {}).get("raw", None), 508003.61)
        
        self.assertEqual(report.get("brain", {}).get("normalized", None), 1363175.03) 
        self.assertEqual(report.get("brain", {}).get("raw", None), 1083589.25)
        
        self.assertEqual(report.get("pgrey", {}).get("normalized", None), 556299.24) 
        self.assertEqual(report.get("pgrey", {}).get("raw", None), 442202.84)
        
        self.assertEqual(report.get("vcsf", {}).get("normalized", None), 75373.30) 
        self.assertEqual(report.get("vcsf", {}).get("raw", None), 59914.32)
    
    def test_default_output_directory(self):
        self.sienax.input = "/foo/bar.nii"
        self.assertEqual(self.sienax.default_output_directory, "bar_sienax")
    
if __name__ == "__main__" :
    unittest.main()
