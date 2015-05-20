chdir>>path
set /p CHDIR=<path
del path
set VTKDIR=%CHDIR%\3rdLibs\VTK\bin
set ITKDIR=%CHDIR%\3rdLibs\ITK\bin
set DCMTKDIR=%CHDIR%\3rdLibs\DCMTK\bin
set GDCMDIR=%CHDIR%\3rdLibs\GDCM\bin
set PATH=%PATH%;%VTKDIR%;%ITKDIR%;%DCMTKDIR%;%GDCMDIR%
set MEDIPY_HOME=%CHDIR%\medipy
set MEDIPY_PLUGINS_PATH=%CHDIR%\medipy\plugins
set PYTHONPATH=%CHDIR%\medipy\lib;%CHDIR%\medipy\lib\Release

"./Python27/python.exe" "./medipy/apps/medipy/medipy"