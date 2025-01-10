#pragma once
#include"usermode.h"
using  std::cout;
using  std::endl;
using  driver::driverHandle;
using  namespace driver;









            
int main()
{
    driverHandle = CreateFile(L"\\\\.\\SexyDriver", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    cout << "driver:" << driverHandle << endl;
    if (driverHandle == INVALID_HANDLE_VALUE)
    {
        std::cout << "驱动句柄创建失败" << std::endl;
        std::cin.get();
        CloseHandle(driverHandle);
        return 1;
    }
    gamePro::pid = gamePro::get_process_id(L"cs2.exe");
	cout << "pid:" << gamePro::pid << endl;
    if (gamePro::pid == 0) {
        std::cout << "未能找到游戏\n";
        std::cin.get();
        CloseHandle(driverHandle);
        return 1;
    }

    
    
    if (attach_to_process(driverHandle, gamePro::pid)==true)
    {
        cout << "附加成功" << std::endl;
        gamePro::clientDll = gamePro::get_module_base(gamePro::pid, L"client.dll");
        cout << "client.dll:" << gamePro::clientDll << endl;
        if (gamePro::clientDll != 0)
        {
            
            while (true)
            {
                Radar::getLocalTeam();
				Radar::Traverse();
				Sleep(3000);
            }
		}
        else
        {
            cout << "未能找到client.dll" << endl;
            CloseHandle(driverHandle);
            return 1;
        }
	}
	else
	{
		cout << "附加失败" << endl;
        CloseHandle(driverHandle);
		return 1;
	}
    CloseHandle(driverHandle);
    std::cin.get();
    return 0;
}