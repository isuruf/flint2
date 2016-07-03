cd build.vc14\flint_config
python %cd%\flint_config.py --build-dll True --build-tests False --build-profiles False

cd ..
msbuild.exe dll_flint/dll_flint.vcxproj /p:Configuration=Release /p:Platform=%PLATFORM% /p:PlatformToolset=v140 /verbosity:minimal
copy dll_flint\%PLATFORM%\Release\dll_flint.lib ..\dll\%PLATFORM%\Release\dll_flint.lib

cd build_tests
python %cd%\build_tests.py --interfaces-tests False --platform %PLATFORM%

REM cd ..\run_tests
REM python %cd%\run_tests.py 0
REM cd ..\..
