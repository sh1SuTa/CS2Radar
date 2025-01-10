#pragma once
#include<Windows.h>
#include<TlHelp32.h>
#include <iostream>
using namespace std;
namespace gamePro {
	inline DWORD pid;
	inline uintptr_t clientDll;
	 DWORD get_process_id(const wchar_t* process_name);
	 uintptr_t get_module_base(const DWORD pid, const wchar_t* module_name);

}

