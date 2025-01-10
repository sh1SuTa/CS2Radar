#pragma once
#include <iostream>
#include<windows.h>
#include"Driver.h"
#include "GameProcess.h"

namespace Radar {
	inline int localTeam;
	inline DWORD64 objectPlayer[2];
	inline int foundTrue = 1;
	
	void getLocalTeam();

	void Traverse();
}