
/*
	This simple pin tool allows you to trace/log Heap related operations
	written by corelanc0d3r
	www.corelan.be

	Copyright (c) 2015, Corelan GCV
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

	* Neither the name of pin nor the names of its
	  contributors may be used to endorse or promote products derived from
	  this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "pin.H"
namespace WINDOWS
{
#include<Windows.h>
}
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <ctime>


/* ================================================================== */
// Global variables 
/* ================================================================== */


BOOL LogAlloc = true;
BOOL LogFree = true;
TLS_KEY alloc_key;
FILE* LogFile;
std::map<ADDRINT,WINDOWS::DWORD> chunksizes;

/* ================================================================== */
// Classes 
/* ================================================================== */

class ModuleImage
{
public:
	string ImageName;
	ADDRINT ImageBase;
	ADDRINT ImageEnd;

	void save_to_log()
	{
		fprintf(LogFile, "** Module %s loaded at 0x%p**\n", ImageName, ImageBase);
	}
};

class HeapOperation
{
public:
	string operation_type;
	ADDRINT chunk_start;
	WINDOWS::DWORD chunk_size;
	ADDRINT chunk_end;
	ADDRINT saved_return_pointer;
	string srp_imagename;
	time_t operation_timestamp;

	void save_to_log()
	{
		int currentpid = PIN_GetPid();
		if (operation_type ==  "rtlallocateheap")
		{
			fprintf(LogFile, "PID: %u | alloc(0x%p) at 0x%p from 0x%p (%s)\n",currentpid,chunk_size,chunk_start,saved_return_pointer,srp_imagename);
		}
		else if (operation_type ==  "rtlreallocateheap")
		{
			fprintf(LogFile, "PID: %u | realloc(0x%p) at 0x%p from 0x%p (%s)\n",currentpid,chunk_size,chunk_start,saved_return_pointer,srp_imagename);
		}
		else if (operation_type ==  "virtualalloc")
		{
			fprintf(LogFile, "PID: %u | virtualalloc(0x%p) at 0x%p from 0x%p (%s)\n",currentpid,chunk_size,chunk_start,saved_return_pointer,srp_imagename);
		}
		else if (operation_type == "rtlfreeheap")
		{
			fprintf(LogFile, "PID: %u | free(0x%p) from 0x%p (size was 0x%p) (%s)\n",currentpid, chunk_start,saved_return_pointer,chunk_size,srp_imagename);
		}
	}

};

vector<HeapOperation> arrAllOperations;
vector<ModuleImage> arrLoadedModules;


/* ===================================================================== */
// Command line switches
/* ===================================================================== */

KNOB<BOOL>   KnobLogAlloc(KNOB_MODE_WRITEONCE,  "pintool",
	"logalloc", "1", "Log heap allocations (RtlAllocateHeap, RtlReAllocateHeap and VirtualAlloc)");

KNOB<BOOL>   KnobLogFree(KNOB_MODE_WRITEONCE,  "pintool",
	"logfree", "1", "Log heap free operations (RtlFreeHeap)");


/* ===================================================================== */
// Utilities
/* ===================================================================== */

INT32 Usage()
{

	return -1;
}


WINDOWS::DWORD findSize(ADDRINT address)
{
	// search in map for key address
	std::map<ADDRINT,WINDOWS::DWORD>::iterator it;

	it = chunksizes.find(address);
	if (it != chunksizes.end())
	{
		return it->second;
	}
	return 0;
}


ModuleImage addModuleToArray(IMG img)
{
	ModuleImage modimage;
	modimage.ImageName = IMG_Name(img);
	modimage.ImageEnd = IMG_HighAddress(img);
	modimage.ImageBase = IMG_LowAddress(img);
	arrLoadedModules.push_back(modimage);
	return modimage;
}


string getModuleImageNameByAddress(ADDRINT address)
{
	string returnval = "";
	IMG theimage;
	PIN_LockClient();
	theimage = IMG_FindByAddress(address);
	PIN_UnlockClient();
	if (IMG_Valid(theimage))
	{
		returnval = IMG_Name(theimage);
	}
	return returnval;
}


/* ===================================================================== */
// Analysis routines (runtime)
/* ===================================================================== */

VOID CaptureRtlAllocateHeapBefore(THREADID tid, UINT32 flags, int size)
{
	// At start of function, simply remember the requested size in TLS
	PIN_SetThreadData(alloc_key, (void *) size, tid);
}


VOID CaptureRtlAllocateHeapAfter(THREADID tid, ADDRINT addr, ADDRINT caller)
{
	// At end of function restore requested size and save data
	// avoid noise
	if (addr > 0x1000 && addr < 0x7fffffff)
	{
		//restore size (dwBytes) argument that was stored at start of function
		int size = (int) PIN_GetThreadData(alloc_key, tid);
		
		// create new object
		HeapOperation ho_alloc;
		ho_alloc.operation_type = "rtlallocateheap";
		ho_alloc.chunk_start = addr;
		ho_alloc.chunk_size = size;
		ho_alloc.chunk_end = addr + size;
		ho_alloc.saved_return_pointer = caller;
		ho_alloc.operation_timestamp = time(0);
		string imagename = getModuleImageNameByAddress(caller);
		ho_alloc.srp_imagename = imagename;

		ho_alloc.save_to_log();

		arrAllOperations.push_back(ho_alloc);
		// add to map chunksizes (or update existing entry)
		chunksizes[addr] = size;

	}
}



VOID CaptureRtlReAllocateHeapBefore(THREADID tid, UINT32 flags, int size)
{
	// At start of function, simply remember the requested size in TLS
	PIN_SetThreadData(alloc_key, (void *) size, tid);
}


VOID CaptureRtlReAllocateHeapAfter(THREADID tid, ADDRINT addr, ADDRINT caller)
{
	// At end of function restore requested size and save data
	// avoid noise
	if (addr > 0x1000 && addr < 0x7fffffff)
	{
		//restore size argument that was stored at start of function
		int size = (int) PIN_GetThreadData(alloc_key, tid);
		
		// create new object
		HeapOperation ho_alloc;
		ho_alloc.operation_type = "rtlreallocateheap";
		ho_alloc.chunk_start = addr;
		ho_alloc.chunk_size = size;
		ho_alloc.chunk_end = addr + size;
		ho_alloc.saved_return_pointer = caller;
		ho_alloc.operation_timestamp = time(0);
		string imagename = getModuleImageNameByAddress(caller);
		ho_alloc.srp_imagename = imagename;

		ho_alloc.save_to_log();

		arrAllOperations.push_back(ho_alloc);
		// add to map chunksizes
		chunksizes[addr] = size;

	}
}


VOID CaptureVirtualAllocBefore(THREADID tid, int size, int flProtect)
{
	// At start of function, simply remember the requested size in TLS
	PIN_SetThreadData(alloc_key, (void *) size, tid);
}


VOID CaptureVirtualAllocAfter(THREADID tid, ADDRINT addr, ADDRINT caller)
{
	// At end of function restore requested size and save data
	// avoid noise
		
	//restore size argument that was stored at start of function
	int size = (int) PIN_GetThreadData(alloc_key, tid);
		
	// create new object
	HeapOperation ho_alloc;
	ho_alloc.operation_type = "virtualalloc";
	ho_alloc.chunk_start = addr;
	ho_alloc.chunk_size = size;
	ho_alloc.chunk_end = addr + size;
	ho_alloc.saved_return_pointer = caller;
	ho_alloc.operation_timestamp = time(0);
	string imagename = getModuleImageNameByAddress(caller);
	ho_alloc.srp_imagename = imagename;

	ho_alloc.save_to_log();

	arrAllOperations.push_back(ho_alloc);
	// add to map chunksizes
	chunksizes[addr] = size;


}


VOID CaptureRtlFreeHeapBefore(ADDRINT addr, ADDRINT caller)
{
	
	// avoid noise
	if (addr > 0x1000 && addr < 0x7fffffff)
	{
		// create new object
		HeapOperation ho_free;
		ho_free.operation_type = "rtlfreeheap";
		ho_free.chunk_start = addr;
		ho_free.saved_return_pointer = caller;
		ho_free.operation_timestamp = time(0);

		string imagename = getModuleImageNameByAddress(caller);


		// see if we can get size from previous allocation
		WINDOWS::DWORD size = findSize(addr);		
		ho_free.chunk_size = size;
		ho_free.chunk_end = ho_free.chunk_start + size;
		ho_free.srp_imagename = imagename;

		ho_free.save_to_log();

		arrAllOperations.push_back(ho_free);

		// remove from chunksizes, because no longer relevant
		chunksizes.erase(addr);

	}
}



/* ===================================================================== */
// Instrumentation callbacks (instrumentation time)
/* ===================================================================== */

VOID AddInstrumentation(IMG img, VOID *v)
{
	// this instrumentation routine gets executed when an image is loaded

	// first, add image information to global array
	ModuleImage thisimage;
	thisimage = addModuleToArray(img);
	thisimage.save_to_log();

	// next, walk through the symbols in the symbol table to see if it contains the Heap related functions that we want to monitor
	//
	for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym))
	{
		string undFuncName = PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY);

		//  Find the RtlAllocateHeap() function.
		if (undFuncName == "RtlAllocateHeap" && LogAlloc)
		{
			RTN allocRtn = RTN_FindByAddress(IMG_LowAddress(img) + SYM_Value(sym));
            
			if (RTN_Valid(allocRtn))
			{
				// Instrument to capture allocation address and original function arguments
				// at end of the RtlAllocateHeap function

				RTN_Open(allocRtn);

				fprintf(LogFile,"Adding instrumentation for RtlAllocateHeap (0x%p)\n", allocRtn);
                
				RTN_InsertCall(allocRtn, IPOINT_BEFORE, (AFUNPTR) &CaptureRtlAllocateHeapBefore,
					IARG_THREAD_ID, IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
					IARG_FUNCARG_ENTRYPOINT_VALUE, 2, IARG_END);

				// return value is the address that has been allocated
				RTN_InsertCall(allocRtn, IPOINT_AFTER, (AFUNPTR) &CaptureRtlAllocateHeapAfter,
					IARG_THREAD_ID, IARG_FUNCRET_EXITPOINT_VALUE, IARG_G_ARG0_CALLER, IARG_END);

         
				RTN_Close(allocRtn);
			}
		}

		//  Find the RtlReAllocateHeap() function.
		else if (undFuncName == "RtlReAllocateHeap" && LogAlloc)
		{
			RTN reallocRtn = RTN_FindByAddress(IMG_LowAddress(img) + SYM_Value(sym));
            
			if (RTN_Valid(reallocRtn))
			{
				// Instrument to capture allocation address and original function arguments
				// at end of the RtlAllocateHeap function

				RTN_Open(reallocRtn);

				fprintf(LogFile,"Adding instrumentation for RtlReAllocateHeap (0x%p)\n", reallocRtn);
				// HeapHandle
				// Flags
				// MemoryPointer
				// Size
				RTN_InsertCall(reallocRtn, IPOINT_BEFORE, (AFUNPTR) &CaptureRtlReAllocateHeapBefore,
					IARG_THREAD_ID, IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
					IARG_FUNCARG_ENTRYPOINT_VALUE, 3, IARG_END);

				// return value is the address that has been allocated
				RTN_InsertCall(reallocRtn, IPOINT_AFTER, (AFUNPTR) &CaptureRtlReAllocateHeapAfter,
					IARG_THREAD_ID, IARG_FUNCRET_EXITPOINT_VALUE, IARG_G_ARG0_CALLER, IARG_END);

				RTN_Close(reallocRtn);
			}
		}

		//  Find the VirtualAlloc() function.
		else if (undFuncName == "VirtualAlloc" && LogAlloc)
		{
			RTN vaallocRtn = RTN_FindByAddress(IMG_LowAddress(img) + SYM_Value(sym));
            
			if (RTN_Valid(vaallocRtn))
			{
				// Instrument to capture allocation address and original function arguments
				// at end of the VirtualAlloc function

				RTN_Open(vaallocRtn);

				fprintf(LogFile,"Adding instrumentation for VirtualAlloc (0x%p)\n", vaallocRtn);
				// lpAddress
				// dwSize
				// flAllocationType
				// flProtect
				RTN_InsertCall(vaallocRtn, IPOINT_BEFORE, (AFUNPTR) &CaptureVirtualAllocBefore,
					IARG_THREAD_ID, IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
					IARG_FUNCARG_ENTRYPOINT_VALUE, 3, IARG_END);

				// return value is the address that has been allocated
				RTN_InsertCall(vaallocRtn, IPOINT_AFTER, (AFUNPTR) &CaptureVirtualAllocAfter,
					IARG_THREAD_ID, IARG_FUNCRET_EXITPOINT_VALUE, IARG_G_ARG0_CALLER, IARG_END);

				RTN_Close(vaallocRtn);
			}
		}


		//  Find the RtlFreeHeap() function.
		else if (undFuncName == "RtlFreeHeap" && LogFree)
		{
			RTN freeRtn = RTN_FindByAddress(IMG_LowAddress(img) + SYM_Value(sym));
            
			if (RTN_Valid(freeRtn))
			{
				RTN_Open(freeRtn);

				fprintf(LogFile,"Adding instrumentation for RtlFreeHeap (0x%p)\n", freeRtn);
                
				RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR) &CaptureRtlFreeHeapBefore,
					IARG_FUNCARG_ENTRYPOINT_VALUE, 2,	// address
					IARG_G_ARG0_CALLER,					// saved return pointer
					IARG_END);

				RTN_Close(freeRtn);
			}
		}


	}
}


BOOL FollowChild(CHILD_PROCESS childProcess, VOID * userData)
{
	fprintf(LogFile, "\n*******************************\nCreating child process from parent PID %u\n*******************************\n\n", PIN_GetPid());
	return true;
}



VOID Fini(INT32 code, VOID *v)
{
	fprintf(LogFile,"\n\nNumber of heap operations logged: %d\n",arrAllOperations.size());
	fprintf(LogFile, "# EOF\n");
	fclose(LogFile);
}


/*!
 */
int main(int argc, char *argv[])
{
    // Initialize PIN library.
	PIN_Init(argc,argv);

	int currentpid = PIN_GetPid();
	stringstream ss;
	ss << "corelan_heaplog_";
	ss << currentpid;
	ss << ".log";
	string fileName = ss.str();
	LogAlloc = KnobLogAlloc.Value();
	LogFree = KnobLogFree.Value();

	if (!fileName.empty()) { LogFile = fopen(fileName.c_str(),"wb");}

	fprintf(LogFile, "Instrumentation started\n");

	PIN_InitSymbols();

	// we will need a way to pass data around, so we'll store stuff in TLS

	alloc_key = PIN_CreateThreadDataKey(0);


	if (LogAlloc)
	{
		fprintf(LogFile, "Logging heap alloc: YES\n");
	}
	else
	{
		fprintf(LogFile, "Logging heap alloc: NO\n");
	}

	if (LogFree)
	{
		fprintf(LogFile, "Logging heap free: YES\n");
	}
	else
	{
		fprintf(LogFile, "Logging heap free: NO\n");
	}


	// notify when following child process
	PIN_AddFollowChildProcessFunction(FollowChild, 0);

	// only add instrumentation if we have to :)

	if (LogAlloc || LogFree)
	{
		// Register function to be called to instrument traces
		IMG_AddInstrumentFunction(AddInstrumentation, 0);

		// Register function to be called when the application exits
		PIN_AddFiniFunction(Fini, 0);
	}

	fprintf(LogFile, "==========================================\n\n");

	// Start the program, never returns
	PIN_StartProgram();
    
	return 0;
}
