IF NOT EXIST ..\build mkdir ..\build
IF NOT EXIST ..\build\data mkdir ..\build\data
xcopy /Y lib64\*.dll ..\build
xcopy /Y sfml\bin\*.dll ..\build
xcopy /Y SDL\lib\x64\*.dll ..\build
xcopy /E /Y data ..\build\data
xcopy /Y deceptus.exe ..\build
IF EXIST deceptus.pdb xcopy /C /Y deceptus.pdb ..\build
