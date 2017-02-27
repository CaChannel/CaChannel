for /f "delims=" %%i in ('conda info --root') do set OUTPUT_PATH=%%i\conda-bld\win-%ARCH%

"%PYTHON%" setup.py install bdist_wheel
copy dist\*.whl %OUTPUT_PATH%
if errorlevel 1 exit 1

xcopy %RECIPE_DIR%\caRepeater.exe %PREFIX%\bin\
if errorlevel 1 exit 1

:: Add more build steps here, if they are necessary.

:: See
:: http://docs.continuum.io/conda/build.html
:: for a list of environment variables that are set during the build process.
