@echo off
if exist %1.ppz ppmz %1.ppz %1.zip
izip -0 %1.zip %2 %3 %4 %5
REM pkzip -e0 %1.zip %2 %3 %4 %5
ppmz %1.zip %1.ppz
del %1.zip
