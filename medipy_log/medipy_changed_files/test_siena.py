import os
import unittest
import sys

import medipy.fsl

@unittest.skipIf(sys.platform.startswith("win"), "windows fsl test skipped")
class TestSiena(unittest.TestCase):
    
    def setUp(self):
        self.siena = medipy.fsl.Siena()
        self.siena.input1 = "foo1.nii"
        self.siena.input2 = "foo2.nii"
    
    def test_basic(self):
        self.assertEqual(self.siena.command, ["siena", "foo1.nii", "foo2.nii"])
    
    def test_output_directory(self):
        self.siena.output_directory = "bar"
        self.assertEqual(self.siena.command, ["siena", "foo1.nii", "foo2.nii", "-o", "bar"])
    
    def test_two_class_segmentation(self):
        self.siena.two_class_segmentation = True
        self.assertEqual(self.siena.command, ["siena", "foo1.nii", "foo2.nii", "-2"])
    
    def test_t2_weighted_input(self):
        self.siena.t2_weighted_input = True
        self.assertEqual(self.siena.command, ["siena", "foo1.nii", "foo2.nii", "-t2"])
    
    def test_masking(self):
        self.siena.masking = True
        self.assertEqual(self.siena.command, ["siena", "foo1.nii", "foo2.nii", "-m"])
    
    def test_ignore_upwards(self):
        self.siena.ignore_upwards = 10
        self.assertEqual(self.siena.command, ["siena", "foo1.nii", "foo2.nii", "-t", "10"])
    
    def test_ignore_downwards(self):
        self.siena.ignore_downwards = 10
        self.assertEqual(self.siena.command, ["siena", "foo1.nii", "foo2.nii", "-b", "10"])
    
    def test_parse_report(self):
        report = self.siena.parse_report(os.path.join(os.path.dirname(__file__), "report.siena"))
        
        self.assertEqual(report.get("version", None), "2.6")
        
        self.assertEqual(report.get("correction", {}).get("A_to_B", None), 1.8183019155)
        self.assertEqual(report.get("correction", {}).get("B_to_A", None), 1.7967950429)
        self.assertEqual(report.get("correction", {}).get("value", None), 1.8075484792)
        
        self.assertEqual(report.get("B_to_A", {}).get("area", None), 297361)
        self.assertEqual(report.get("B_to_A", {}).get("volume_change", None), -10995.9)
        self.assertEqual(report.get("B_to_A", {}).get("ratio", None), -0.0369784)
        self.assertEqual(report.get("B_to_A", {}).get("pbvc", None), -2.00521)
        
        self.assertEqual(report.get("A_to_B", {}).get("area", None), 295525)
        self.assertEqual(report.get("A_to_B", {}).get("volume_change", None), -1277.55)
        self.assertEqual(report.get("A_to_B", {}).get("ratio", None), -0.00432297)
        self.assertEqual(report.get("A_to_B", {}).get("pbvc", None), -0.234419)
        
        self.assertEqual(report.get("pbvc", None), 0.8853955000)
    
    def test_default_output_directory(self):
        self.siena.input1 = "/foo/bar1.nii"
        self.siena.input2 = "/foo/bar2.nii"
        self.assertEqual(self.siena.default_output_directory, "bar1_to_bar2_siena")
    
if __name__ == "__main__" :
    unittest.main()
