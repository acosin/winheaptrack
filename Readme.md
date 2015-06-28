winheaptrack
=====

winheaptrack is a very simple heap profiler (or memory profiler) for windows applications with [heaptrack](http://milianw.de/blog/heaptrack-a-heap-memory-profiler-for-linux) compatible output.

It lets you see what parts of an application are allocating the most memory.

winheaptrack supports 32 and 64 bit applications written in C/C++. You do not need to modify your application in any way to use winheaptrack.

winheaptrack will hook and profile any `malloc` and `free` functions it can find. This will in turn cause `new` and `delete` to be profiled too (at least on MSVC `new` and `delete` call `malloc` and `free`.)

Build
-----

Simply clone this repository and build the `winheaptrack.sln` in Visual Studio 2012. More recent versions of visual studio should work, older versions will not.

Be sure to select the correct configuration for your needs: a release Win32 or x64 configuration depending on whether you want to profile 32 or 64 bit applications.


Usage
-----

Once winheaptrack is built the executables are put into the Release directory. To profile an application simply run winheaptrack.exe with the first argument as the path to the exe you wish to profile. Make sure that the debug database (`.pdb` file) is in the same directory as your target application so that you get symbol information in your stack traces. You can profile release builds but profiling debug or unoptimised builds gives the nicest stack traces.

```
winheaptrack_x64.exe C:\Windows\SysWOW64\notepad.exe
```

By default winheaptrack will run the given executable from the same folder as that executable. You can specify a working directory with an optional second argument:

```
winheaptrack_x64.exe C:\Windows\SysWOW64\notepad.exe C:\A\Working\Dir
```

Remember to call `winheaptrack_x64.exe` to profile 64 bit applications and `winheaptrack_Win32.exe` to profile 32 bit applications. 

Results
-------

Once your application is running winheaptrack will start writing profiling results to a file called "winheaptrack.%FILENAME%.%PID%" in the applications working directory, where FILENAME is the name of the executable and PID was the process id for the run.

Every 10 milliseconds and on the termination of your program information will be added to the report.

Currently to view the output you have to copy it to a Linux system and run heaptrack_print to interpret the results. The upside is you can also use heaptrack_print to generate [massif](http://valgrind.org/docs/manual/ms-manual.html) compatible output, allowing you to also view results in the excellent [massif-visualizer](http://valgrind.org/docs/manual/ms-manual.html).

Example
-------

This tiny test program `TestApplication`:

```C++
#include <windows.h>
#include <iostream>

void LeakyFunction(){
	malloc(1024*1024*5); // leak 5Mb
}

void NonLeakyFunction(){
	auto p = malloc(1024*1024); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	free(p); // free the Mb
}

int main()
{
	std::cout << "TestApplication: Creating some leaks..." << std::endl;
	for(int i = 0; i < 5; ++i){
		LeakyFunction();
	}
	NonLeakyFunction();
	std::cout << "TestApplication: Exiting..." << std::endl;
	return 0;
}
```

Running heaptrack_print out the output produces the following report (with extraneous stack traces removed):

```

reading file "winheaptrack.TestApplication_x64.5336" - please wait, this might take some time...
Debuggee command was: C
finished reading file, now analyzing data:

MOST CALLS TO ALLOCATION FUNCTIONS
5 calls to allocation functions with 26.21MB peak consumption from
LeakyFunction
  at c:\git\winheaptrack\testapplication\main.cpp:6
  in TestApplication_x64
5 calls with 26.21MB peak consumption from:
    main
      at c:\git\winheaptrack\testapplication\main.cpp:20
      in TestApplication_x64

1 calls to allocation functions with 1.05MB peak consumption from
NonLeakyFunction
  at c:\git\winheaptrack\testapplication\main.cpp:9
  in TestApplication_x64
1 calls with 1.05MB peak consumption from:
    main
      at c:\git\winheaptrack\testapplication\main.cpp:22
      in TestApplication_x64


PEAK MEMORY CONSUMERS

WARNING - the data below is not an accurate calcuation of the total peak consumption and can easily be wrong.
 For an accurate overview, disable backtrace merging.
26.21MB peak memory consumed over 5 calls from
LeakyFunction
  at c:\git\winheaptrack\testapplication\main.cpp:6
  in TestApplication_x64
26.21MB consumed over 5 calls from:
    main
      at c:\git\winheaptrack\testapplication\main.cpp:20
      in TestApplication_x64

1.05MB peak memory consumed over 1 calls from
NonLeakyFunction
  at c:\git\winheaptrack\testapplication\main.cpp:9
  in TestApplication_x64
1.05MB consumed over 1 calls from:
    main
      at c:\git\winheaptrack\testapplication\main.cpp:22
      in TestApplication_x64

total runtime: 15.09s.
bytes allocated in total (ignoring deallocations): 27.27MB (1.81MB/s)
calls to allocation functions: 11 (0/s)
peak heap memory consumption: 27.26MB
total memory leaked: 26.21MB


```

You can run winheaptrack on the test application above by building the `ProfileTestApplication` project in the solution (you must manually click to build that project, it's not set to build on "Build All".)

How It Works
-----------

winheaptrack is based on Heapy. This [blog post](http://www.lukedodd.com/Heapy-heap-profiler/) describes Heapy in detail.

Future
------

A GUI for viewing output reports is planned.