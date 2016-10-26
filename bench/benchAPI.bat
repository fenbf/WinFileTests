@echo off
for /l %%x in (1, 1, %5) do (
WinFileTests_x64.exe clear %2
timep.exe WinFileTests_x64.exe transform %1 %2 %3 %4 %6
del %3
rem WinFileTests_x64.exe clear %3
)