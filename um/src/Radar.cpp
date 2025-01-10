#include "Radar.h"



using namespace std;
using driver::driverHandle;

void Radar::getLocalTeam() {
	DWORD64 localAddress=driver::read_memory<DWORD64>(driverHandle, gamePro::clientDll + 0x1868CC8);
	//cout << "localAddress=" << localAddress << endl;
	Radar::localTeam=driver::read_memory<uint16_t>(driverHandle, localAddress + 0xE68);
	//cout << "localteam=" << Radar::localTeam << endl;
	

}




void Radar::Traverse() {
	for (int i = 0; i < 30; i++)
	{
		Radar::objectPlayer[0] = driver::read_memory<DWORD64>(driverHandle, gamePro::clientDll + 0x1877080+ i * 0x10);
		//Radar::objectPlayer[1] = driver::read_memory<DWORD64>(driverHandle, Radar::objectPlayer[0]+0x8 + i * 0x10);
		//cout << "objectPlayer="<<Radar::objectPlayer << endl;
		int team=driver::read_memory<DWORD64>(driverHandle, Radar::objectPlayer[0] + 0xE68);
		//cout << "team=" << team << endl;
		//判断是否是敌人
		
		if (team != Radar::localTeam)
		{
			int found = driver::read_memory<int>(driverHandle, Radar::objectPlayer[0] + 0x23dc);
			//cout << "found=" << found << endl;
			//如果敌人是未发现状态
			//if (found==0)
			{
				driver::write_memory<int>(driverHandle,Radar::objectPlayer[0] + 0x23dc, 1);

			}
		}
	}
}

