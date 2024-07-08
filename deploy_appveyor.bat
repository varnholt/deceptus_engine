IF NOT EXIST artifacts mkdir artifacts
IF NOT EXIST artifacts\data mkdir artifacts\data
IF NOT EXIST artifacts\tools mkdir artifacts\tools

xcopy /Y thirdparty\lua\lib64\*.dll artifacts
xcopy /Y thirdparty\sfml\bin\*.dll artifacts
xcopy /Y thirdparty\sdl\lib\x64\*.dll artifacts
xcopy /E /Y data artifacts\data
xcopy /E /Y tools artifacts\tools
xcopy /E /Y tools\level_selector\level_selector.exe artifacts
xcopy /Y build\Release\deceptus.exe artifacts

IF EXIST build\Release\deceptus.pdb xcopy /C /Y build\Release\deceptus.pdb artifacts
