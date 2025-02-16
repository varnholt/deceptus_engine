IF NOT EXIST ..\build mkdir ..\build
IF NOT EXIST ..\build\data mkdir ..\build\data
IF NOT EXIST ..\build\tools mkdir ..\build\tools

xcopy /E /Y data ..\build\data
xcopy /E /Y tools ..\build\tools
xcopy /E /Y tools\level_selector\level_selector.exe ..\build
xcopy /Y build_deb\install\bin\*.dll ..\build
xcopy /Y build_deb\deceptus.exe ..\build

IF EXIST deceptus.pdb xcopy /C /Y deceptus.pdb ..\build
