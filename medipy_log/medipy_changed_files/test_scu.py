import os
import shutil
import socket
import subprocess
import tempfile
import unittest

import medipy.io.dicom
import medipy.network.dicom


class TestSCU(unittest.TestCase):
    
    template = """HostTable BEGIN
remote = (REMOTE, localhost, {port})
local = (LOCAL, localhost, 0)
HostTable END 

AETable BEGIN
REMOTE {directory} RW (10, 1024mb) ANY
AETable END
"""

    
    def setUp(self):
        
        # Let the OS pick an available port by binding to port 0
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(("", 0))
        port = s.getsockname()[1]
        s.close()
        
        # Create a temporary directory with the configuration file for the SCP
        self.directory = tempfile.mkdtemp()
        configuration_file = os.path.join(self.directory, "dcmqrscp.cfg")
        with open(configuration_file, "w") as f :
            f.write(TestSCU.template.format(port=port, directory=self.directory))
        
        # Start the SCP process
        self.subprocess = subprocess.Popen(
           ["dcmqrscp", "-c", configuration_file, str(port)])
        
        # Populate the SCP with sample file
        store = subprocess.call(
            ["storescu", "-aet", "LOCAL", "-aec", "REMOTE", "localhost", str(port), 
             os.path.join(os.path.dirname(__file__), "..", "..", "..", "data", 
                          "input", "siemens_mosaic.dcm")])
        
        # Create the global connection object
        host = "localhost"
        port = port
        caller_ae_title = "LOCAL"
        called_ae_title = "REMOTE"
        
        self.connection = medipy.network.dicom.Connection(
            host, port, caller_ae_title, called_ae_title)
    
    def tearDown(self):
        self.subprocess.kill()
        shutil.rmtree(self.directory)
    
    def test_echo(self):
        echo = medipy.network.dicom.scu.Echo(self.connection)
        echo()
    
    def test_find(self):
        query = medipy.io.dicom.DataSet()
        query.patients_name = medipy.io.dicom.PN("*")
        query.patient_id = medipy.io.dicom.LO("")
    
        find = medipy.network.dicom.scu.Find(self.connection, "patient", "patient", query)
        result = find()
        
        self.assertTrue(len(result)>0)
        self.assertTrue(isinstance(result[0], medipy.io.dicom.DataSet))
        
        dataset = result[0]
        
        self.assertTrue("patients_name" in dataset)
        self.assertEqual(dataset.patients_name.value, "Upgrade Test^Rorden")
        
        self.assertTrue("patient_id" in dataset)
        self.assertEqual(dataset.patient_id.value, "Rorden Lab")
    
    def test_store(self):
        dataset = medipy.io.dicom.read(os.path.join(
            os.path.dirname(__file__), "..", "..", "..", "data", 
            "input", "siemens_dwi_0.dcm"))

        store = medipy.network.dicom.scu.Store(self.connection, dataset)
        store()
        
        query = medipy.io.dicom.DataSet()
        query.study_instance_uid = dataset.study_instance_uid
        
        find = medipy.network.dicom.scu.Find(self.connection, "study", "study", query)
        result = find()
        
        self.assertTrue(len(result)==1)
        self.assertTrue(isinstance(result[0], medipy.io.dicom.DataSet))
    
if __name__ == "__main__" :
    unittest.main()
