#pragma once
#include <ostream>
#include <vector>
#include <unordered_map>
#include <set>
#include <mutex>
#include <algorithm>
#include <Windows.h>
#include "dbghelp.h"

const int backtraceSize = 64;
typedef unsigned long StackHash;

struct InstNode {
	unsigned long long inst_ptr;
	int idx;
	std::vector<InstNode> childrenNodes;
};


class InstructionGraph {
public:
	void clear() 
	{
		rootInst.childrenNodes.clear();
		lastidx = 0;
	}

	int index(intptr_t* backtrace, FILE* output)
	{
		HANDLE process = GetCurrentProcess();

		const int MAXSYMBOLNAME = 128 - sizeof(IMAGEHLP_SYMBOL);
		char symbol64_buf[sizeof(IMAGEHLP_SYMBOL) + MAXSYMBOLNAME] = { 0 };
		IMAGEHLP_SYMBOL *symbol = reinterpret_cast<IMAGEHLP_SYMBOL*>(symbol64_buf);
		symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
		symbol->MaxNameLength = MAXSYMBOLNAME - 1;
		IMAGEHLP_MODULE module;
		module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

		int idx = 0;
		for (size_t i = backtraceSize - 1; i > 0; --i){
			unsigned long long ip = backtrace[i];
			if (!ip) continue;
			if (SymGetSymFromAddr(process, (DWORD64)ip, 0, symbol)){
				ip = symbol->Address;
			}
			else { continue; }
			InstNode* pInst = &rootInst;
			auto inst = std::lower_bound(pInst->childrenNodes.begin(), pInst->childrenNodes.end(), ip, [](InstNode& inst, unsigned long long ip) { return inst.inst_ptr < ip; });
			if (inst == pInst->childrenNodes.end() || inst->inst_ptr != ip)
			{
				idx = lastidx++;
				inst = pInst->childrenNodes.insert(inst, { ip, idx, {} });
				fprintf(output, "t %lx %lx\n", idx, pInst->idx);
			}
			rootInst = *(inst);
		}
		return idx;
	}

private:
	InstNode rootInst = InstNode{ 0, 0, {} };
	int lastidx = 1;
};

struct ProfileData {
	FILE* output;
	InstructionGraph instGraph;
	std::unordered_map<std::string, size_t> strings;
	std::unordered_map<StackHash, int> stacks;
	int intern(std::string str)
	{
		if (str.empty())return 0;
		std::unordered_map<std::string, size_t>::const_iterator interned = strings.find(str);

		if (interned != strings.end()) return interned->second;
		size_t val = strings.size() + 1;
		strings.insert(interned, std::make_pair(str, val));
		fprintf(output, "s %s\n", str.c_str());
		return val;
	}

};

struct StackTrace{
	void *backtrace[backtraceSize];
	StackHash hash;

	StackTrace();
	void trace(ProfileData &data);
	int index() const;
private:
	int idx;
};

class HeapProfiler{
public:
	HeapProfiler();
	ProfileData data;
	void malloc(void *ptr, size_t size, const StackTrace &trace);
	void free(void *ptr, const StackTrace &trace);

private:
	std::mutex mutex;
	std::unordered_map<void*, StackHash> ptrs;

};