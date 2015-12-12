# pin
Collection of pin tools


## General information

Unless stated otherwise, all pin tools were created with Visual Studio 2012 Express, and written in C++
You can download Visual Studio Express 2012 for Windows Desktop here: https://www.microsoft.com/en-us/download/details.aspx?id=34673

To download & install the corresponding version of Pin, check this link:
https://software.intel.com/en-us/articles/pintool-downloads


## Creating a new Pin project folder for use with Visual Studio

This repository contains a simple python script @createpintool.py@ that allows you to create a new Visual Studio project folder, based on the "MyPinTool" example folder.
You can find the script in the @win32@ folder in this repository.

Place the script into the source/tools folder inside your pin folder structure.
Run the script from that working folder, and specify the name of the new project.

Example:
```
C:\pin\vc11\source\tools>python createpintool.py MyNewProjectName
[+] Creating new PIN project MyNewProjectName
    - Copying clean project
[+] Updating project files
    - Processing makefile -> makefile
    - Processing makefile.rules -> makefile.rules
    - Processing MyPinTool.cpp -> MyNewProjectName.cpp
    - Processing MyPinTool.vcxproj -> MyNewProjectName.vcxproj
    - Processing MyPinTool.vcxproj.filters -> MyNewProjectName.vcxproj.filters
[+] Done
```


## Notes

1. For Visual Studio C++ 2012, you need vc11, not vc12!<br>
2. You may have to disable SAFESEH.
