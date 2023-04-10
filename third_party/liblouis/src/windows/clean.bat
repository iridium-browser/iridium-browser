@echo off
rem Cleans away all binary output from building liblouis with VS.
pushd "%CD%" 
cd %~dp0
erase *.obj
erase liblouis*.dll
erase liblouis*.exp
erase liblouis*.lib
popd
@echo on
