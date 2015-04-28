import unittest
import sys

import medipy.fsl

@unittest.skipIf(sys.platform.startswith("win"), "windows fsl test skipped")
class TestBET(unittest.TestCase):
    
    def setUp(self):
        self.bet = medipy.fsl.BET()
        self.bet.input = "foo.nii"
        self.bet.output = "bar.nii"
    
    def test_basic(self):
        self.assertEqual(self.bet.command, ["bet", "foo.nii", "bar.nii"])
    
    def test_outline(self):
        self.bet.create_outline = True
        self.assertEqual(self.bet.command, ["bet", "foo.nii", "bar.nii", "-o"])
        self.assertNotEqual(self.bet.outline, None)
    
    def test_brain_mask(self):
        self.bet.create_brain_mask = True
        self.assertEqual(self.bet.command, ["bet", "foo.nii", "bar.nii", "-m"])
        self.assertNotEqual(self.bet.brain_mask, None)
    
    def test_skull(self):
        self.bet.create_skull = True
        self.assertEqual(self.bet.command, ["bet", "foo.nii", "bar.nii", "-s"])
        self.assertNotEqual(self.bet.skull, None)
    
    def test_intensity_threshold(self):
        self.bet.intensity_threshold = 0.4
        self.assertEqual(self.bet.command, ["bet", "foo.nii", "bar.nii", "-f" , "0.4"])
    
    def test_gradient_threshold(self):
        self.bet.gradient_threshold = 0.7
        self.assertEqual(self.bet.command, ["bet", "foo.nii", "bar.nii", "-g" , "0.7"])
    
    def test_head_radius(self):
        self.bet.head_radius = 30
        self.assertEqual(self.bet.command, ["bet", "foo.nii", "bar.nii", "-r" , "30"])
    
    def test_center_of_gravity(self):
        self.bet.center_of_gravity = (10,20,30)
        self.assertEqual(self.bet.command, ["bet", "foo.nii", "bar.nii", 
                                            "-c" , "10", "20", "30"])
    
    def test_robust(self):
        self.bet.robust = True
        self.assertEqual(self.bet.command, ["bet", "foo.nii", "bar.nii", "-R"])
    
if __name__ == "__main__" :
    unittest.main()
