@echo off
REM
REM     Copyright (C) 2016-2017 Moddable Tech, Inc.
REM     All rights reserved.
REM
@echo on
nmake GOAL=debug /c /f "xsc.mak" /s
nmake GOAL=debug /c /f "xsl.mak" /s
nmake GOAL=debug /c /f "xsr.mak" /s


