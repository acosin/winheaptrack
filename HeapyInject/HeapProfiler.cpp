#include "HeapProfiler.h"
#include <stdio.h>
#include <iomanip>

StackTrace::StackTrace() : hash(0){
	memset(backtrace, 0, sizeof(void*)*backtraceSize);
}

int StackTrace::index() const {
	return idx;
}

void StackTrace::trace(ProfileData &data){
	CaptureStackBackTrace(0, backtraceSize, backtrace, &hash);
	auto stk = data.stacks.find(hash);
	if (stk != data.stacks.end()) { idx = stk->second; return; };
	HANDLE process = GetCurrentProcess();

	const int MAXSYMBOLNAME = 128 - sizeof(IMAGEHLP_SYMBOL);
	char symbol64_buf[sizeof(IMAGEHLP_SYMBOL) + MAXSYMBOLNAME] = { 0 };
	IMAGEHLP_SYMBOL *symbol = reinterpret_cast<IMAGEHLP_SYMBOL*>(symbol64_buf);
	symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	symbol->MaxNameLength = MAXSYMBOLNAME - 1;
	IMAGEHLP_MODULE module;
	module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

	for (size_t i = backtraceSize - 1; i > 0; --i){
		int fileidx = 0;
		int linenum = 0;
		int symidx = 0;
		int modidx = 0;

		if (backtrace[i]){
			// Output stack frame symbols if available.
			if (SymGetSymFromAddr(process, (DWORD64)backtrace[i], 0, symbol)){
				std::string symbol_name = symbol->Name;
				if (strncmp("mallocHook<", symbol->Name, 11) == 0)
				{
					symbol_name = "malloc";
				}
				if (strncmp("freeHook<", symbol->Name, 9) == 0)
				{
					symbol_name = "free";
				}

				symidx =  data.intern(symbol_name);

				if (SymGetModuleInfo(process, (DWORD64)backtrace[i], &module))
				{
					modidx = data.intern(module.ModuleName);
				}
				
				// Output filename + line info if available.
				IMAGEHLP_LINE lineSymbol = { 0 };
				lineSymbol.SizeOfStruct = sizeof(IMAGEHLP_LINE);
				DWORD displacement;
				if (SymGetLineFromAddr(process, (DWORD64)backtrace[i], &displacement, &lineSymbol)){
					fileidx = data.intern(lineSymbol.FileName);
					linenum = lineSymbol.LineNumber;
				}
			}
			fprintf(data.output, "i %lx %lx", symbol->Address, modidx);
			if (symidx || fileidx)
			{
				fprintf(data.output, " %lx", symidx);
				if (fileidx) fprintf(data.output, " %lx %lx", fileidx, linenum);
			}
			fprintf(data.output, "\n");
		}
		else{
			continue;
		}
	}
	idx = data.instGraph.index((intptr_t*)backtrace, data.output);
	data.stacks.insert(std::make_pair(hash, idx));
}


HeapProfiler::HeapProfiler() 
{
	char fileName[MAX_PATH];
	wchar_t processName[MAX_PATH];
	wchar_t baseName[MAX_PATH];
	GetModuleFileName(NULL, processName, sizeof(processName));
	_wsplitpath(processName, NULL, NULL, baseName, NULL);
	sprintf(fileName, "heapy.%S.%d", baseName, GetCurrentProcessId());
	printf("Saving data to %s...\n", fileName);
	data.output = fopen(fileName, "w+b");
	fprintf(data.output, "X %s\n", processName);
}
void HeapProfiler::malloc(void *ptr, size_t size, const StackTrace &trace){
	std::lock_guard<std::mutex> lk(mutex);
	fprintf(data.output, "+ %lx %lx %lx\n", size, trace.index(), reinterpret_cast<uintptr_t>(ptr));
	data.tick();
	// Store the stracktrace hash of this allocation in the pointers map.
	ptrs[ptr] = trace.hash;
}

void HeapProfiler::free(void *ptr, const StackTrace &trace){
	std::lock_guard<std::mutex> lk(mutex);

	// On a free we remove the pointer from the ptrs map and the
	// allocating stack traces map.
	auto it = ptrs.find(ptr);
	if(it != ptrs.end()){
		ptrs.erase(it);
		fprintf(data.output, "- %lx\n", reinterpret_cast<uintptr_t>(ptr));
		data.tick();
	}else{
		// Do anything with wild pointer frees?
	}
}
