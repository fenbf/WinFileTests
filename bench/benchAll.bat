clear.exe
timep.exe WinFileTests_x64.exe transform crt test512.bin outCrt.bin %1
clear.exe
timep.exe WinFileTests_x64.exe transform std test512.bin outStd.bin %1
clear.exe
timep.exe WinFileTests_x64.exe transform win test512.bin outWin.bin %1
clear.exe
timep.exe WinFileTests_x64.exe transform winmap test512.bin outMap.bin %1
delOutFiles.bat