@echo off
setlocal

set OPTIONS=AStyle.cfg

astyle --options=%OPTIONS% ..\..\*.h
astyle --options=%OPTIONS% ..\..\*.c
astyle --options=%OPTIONS% ..\..\*.pde
astyle --options=%OPTIONS% ..\..\*.ino

endlocal