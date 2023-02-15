IF NOT EXIST ..\build mkdir ..\build
IF NOT EXIST ..\build\data mkdir ..\build\data
IF NOT EXIST ..\build\tools mkdir ..\build\tools

xcopy /Y lib64\*.dll ..\build
xcopy /Y sfml\bin\*.dll ..\build
xcopy /Y SDL\lib\x64\*.dll ..\build
xcopy /E /Y data ..\build\data
xcopy /E /Y tools ..\build\tools
xcopy /E /Y tools\level_selector\release\level_selector.exe ..\build
xcopy /Y deceptus.exe ..\build

IF EXIST deceptus.pdb xcopy /C /Y deceptus.pdb ..\build
