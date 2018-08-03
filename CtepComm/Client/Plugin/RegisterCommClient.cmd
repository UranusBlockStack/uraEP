@echo off
@for %%I in (%0) do @set pth=%%~dpI
@for %%I in (%0) do @set driver=%%~dI
%driver%
cd %pth%
RegSvr32.exe CtepTcRdp.dll /s



