#include "Basic.h"

PCB::PCB(int priority)
{
	string PID; //这里没有想好到底怎么分配PID，要不按照创建时间分配？
	int priority = priority;
	int WaitingTime;
	int RunTime;
	int status = 0;  //进程所处的状态


	// 逻辑地址，记录下一条要执行的指令的虚拟地址
	int PageNumber;  //虚拟页号
	int PageIndex; //虚拟页内偏移量

	// 设备，记录当前进程在申请的设备信息
	vector<EquipInfo> Equipnum; // 记录申请的设备的编号
}