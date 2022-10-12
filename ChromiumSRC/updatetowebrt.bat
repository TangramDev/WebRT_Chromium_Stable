@echo off
xcopy /s %1 %2&&git add . &&git commit -am %3