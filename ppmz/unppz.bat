@echo off
if exist %1.ppz goto found
echo ERROR: file not found
goto end
:found
ppmz %1.ppz %1.zip
iunzip %1.zip %2 %3 %4 %5
REM iunzip is info-unzip
REM pkunzip %1.zip %2 %3 %4 %5
del %1.zip
:end
