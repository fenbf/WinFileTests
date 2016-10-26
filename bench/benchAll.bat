@echo off
if "%2" == "seq" echo SEQUENTIAL
call benchCRT_byblocks.bat %1 %2
call benchSTD_byblocks.bat %1 %2 
call benchWIN_byblocks.bat %1 %2
call benchWINMAP_byblocks.bat %1 %2