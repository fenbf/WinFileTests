@echo off
for /l %%x in (1, 1, %3) do (
WinFileTests_x64.exe clear %1
timep.exe WinFileTests_x64.exe transform crt %1 outCrt.bin %2 %4
)