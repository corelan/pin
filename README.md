# pin
Collection of pin tools


## General information

Unless stated otherwise, all pin tools were created with Visual Studio 2012 Express, and written in C++
You can download Visual Studio Express 2012 for Windows Desktop here: https://www.microsoft.com/en-us/download/details.aspx?id=34673

To download & install the corresponding version of Pin, check this link:
https://software.intel.com/en-us/articles/pintool-downloads


## Creating a new Pin project folder for use with Visual Studio

This repository contains a simple python script `createpintool.py` that allows you to create a new Visual Studio project folder, based on the "MyPinTool" example folder.
You can find the script in the `win32` folder in this repository.

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

1. For Visual Studio C++ 2012, you need vc11, not vc12!
2. You may have to disable SAFESEH.
3. All pintools must be considered "proof of concept" and come without any warranty whatsoever.  The pintools have only been tested in a very small number of situations and may not work on your system. I do, however, encourage everyone to improve the code, performance, stability and/or submit Pull Requests to add interesting functionality.
4. I have compiled all pin tools in "Debug" mode.
5. If your pin tool doesn't run (i.e. if it terminates almost immediately after launching it), launch pin with the `-xyzzy -mesgon log_win` options, to activate some verbose logging.  A file `pintool.log` will be created in case of C++ errors.
6. The Corelan_HeapLog pin tool doesn't always end up launching the process.  Try again after a few seconds and it should work. (Weird error in pin.log. Not sure what to do with it).
7. Remove *.log files before running the Corelan_HeapLog pin tool again. Also, make sure any relevant previous processes are killed before launching a new pin instance.

## Available pintools in this repository

### 1. Corelan_HeapLog
This pintool allows you to log all calls to RtlAllocateHeap, RtlReAllocateHeap, VirtualAlloc and RtlFreeHeap.<br>
Output is written to `corelan_heaplog_<pid>.log` (Fresh file for every process).<br>
Exceptions are written to `corelan_heaplog_exception.log` (One file, exceptions are appended to this file)

You can specify 2 command line options: <br>
`-logalloc <value>`  : enable or disable logging allocations by setting value to 1 or 0<br>
`-logfree <value>`   : enable or disable logging free operations by setting value to 1 or 0<br>
`-timestamp <value>` : enable or disable showing timestamp of heap operation by setting value to 1 or 0<br>
Both log settings are enabled by default. Timestamp is disabled by default (as it may slow down the process a tiny little bit).

The pintool *should* be capable of instrumenting child processes, provided that you have specified the `-follow-execv` pin command line option.

Example:<br>
```C:\pin\vc11>pin -follow-execv -t c:\pin\vc11\source\tools\Corelan_HeapLog\Debug\Corelan_HeapLog.dll -timestamp 1 -logfree 0 -- "c:\Program Files (x86)\Internet Explorer\iexplore.exe" http://127.0.0.1:8080/blah.html```

