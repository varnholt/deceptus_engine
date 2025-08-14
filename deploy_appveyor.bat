IF NOT EXIST artifacts mkdir artifacts
IF NOT EXIST artifacts\data mkdir artifacts\data
IF NOT EXIST artifacts\tools mkdir artifacts\tools

xcopy /E /Y data artifacts\data
xcopy /E /Y tools artifacts\tools
xcopy /E /Y tools\level_selector\level_selector.exe artifacts
xcopy /Y build\install\bin\* artifacts
xcopy /E /Y tools\deceptus_updater\deceptus_updater.exe artifacts
xcopy /E /Y tools\deceptus_updater\files.json artifacts

IF EXIST build\Release\deceptus.pdb xcopy /C /Y build\Release\deceptus.pdb artifacts
