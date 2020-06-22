#include "Basic.h"

PCB::PCB(string PID, int priority)
{
	this->PID = PID;
	this->PageIndex = 0;
	this->PageNumber = 0;
	this->WaitingTime = 0;
	this->priority = priority;
	this->RunTime = 0;
	this->status = PRO_NEW;
	//调用内存提供的接口，获取分配的虚拟地址
}

string CPUSimulate::IF()
{
	string order = "test";  //这里调用内存提供的接口来获取指令
	this->order = order;
}

void CPUSimulate::SplitString(const string& s, vector<string>& v, const string& c)
{
	string::size_type pos1, pos2;
	pos2 = s.find(c); // 返回下标
	pos1 = 0;
	while (string::npos != pos2) // 如果能找到
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

int CPUSimulate::ID()
{
	string order = this->order;
	if (order == "BREAK")  // 缺页中断
	{
		cout << "缺页中断" << endl;  //这里调用内存提供的接口
		//这里将this->RUNNING[0]放入内存提供的等待队列
		return 0;
	}
	//所有执行完之后可以直接执行下一条，不需要判断是否阻塞的指令，返回1，导致阻塞的，返回0
	else
	{
		switch (order[0])
		{
		case 'K':
		{
			vector<string>data;
			this->SplitString(order, data, " ");
			this->EquipApply(stoi(data[1]), 'K');
			//调用设备提供的接口，放入键盘等待队列中，并提供执行的时间
			return 0;
		}

		case 'P':
		{
			vector<string>data;
			this->SplitString(order, data, " ");
			this->EquipApply(stoi(data[1]), 'P');
			//调用设备打印机提供的接口，放入打印机等待队列中,并提供执行的时间
			return 0;

		}

		case 'C':
		{
			int time = 0;
			order.erase(order.begin());
			time = stoi(order);
			this->run(time, 'C');
			return 1;
		}

		case 'M':
		{
			int size = 0;
			order.erase(order.begin());
			size = stoi(order);
			this->MemApply(size);
			//调用内存提供的内存分配接口,加入分配等待队列
			return 0;
		}

		case 'F':
		{
			int size = 0;
			order.erase(order.begin());
			size = stoi(order);
			//调用内存提供的释放接口
			return 1;
		}

		case 'Y':
		{
			int priority = 0;
			order.erase(order.begin());
			priority = stoi(order);
			this->RUNNING[0].priority = priority; // 修改优先级
			return 1;
		}

		case 'W':
		{
			vector<string>data;
			this->SplitString(order, data, " ");
			string filename = data[1];
			int time = stoi(data[2]);
			int size = stoi(data[3]);
			this->WriteApply(filename, time, size);
			//将进程调入文件提供的阻塞队列
			return 0;
		}

		case 'R':
		{
			vector<string>data;
			this->SplitString(order, data, " ");
			string filename = data[1];
			int time = stoi(data[2]);
			int size = stoi(data[3]);
			this->ReadApply(filename, time, size);
			//将进程调入文件提供的读文件阻塞队列
			return 0;
		}

		case 'Q':
			return 0;

		}
	}

}

void CPUSimulate::interrupt(PCB process)
{
	this->interptLock.lock();
	if (this->isFinish == true && this->RUNNING.size() == 0) //如果没有进程正在执行
	{
		this->RUNNING.push_back(process);
		this->isFinish = false;
	}
	else //如果有，先放入中断向量表等待
	{
		this->Lock = true;
		this->BREAK.push_back(process);
	}
	this->interptLock.unlock();
}

void CPUSimulate::run(int time, char type) //这里暂时没想好
{
	while (time > 0)
	{
		//这里怎么让time和CLOCK同步？？？？？？？
		time--;
	}

}