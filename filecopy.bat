@echo off
@if exist %2 attrib -r %2
@if exist %2 del %2
@copy %1 %2
@if exist %2 attrib +r %2
