@echo off
call benchWIN.bat %1 1 %numCalls% %2
call benchWIN.bat %1 16 %numCalls% %2
call benchWIN.bat %1 128 %numCalls% %2
call benchWIN.bat %1 1024 %numCalls% %2
call benchWIN.bat %1 2048 %numCalls% %2
