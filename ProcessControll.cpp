#include "Basic.h"

PCB::PCB(int PID, int priority, int pageNo)
{
	this->PID = PID;
	this->priority = priority;
	this->WaitingTime = 0;
	this->RunTime = 0;

	this->pageNo = pageNo;
	this->pageId = 0;
	this->offset = 0;
	//调用内存提供的接口，获取分配的虚拟地址
}

PCB::PCB(void){}

int CPUSimulate::findProcess(int PID, vector<pair<int, PCB>> processList) {
	for (int i = 0; i < processList.size(); i++) {
		if (processList[i].first == PID) {
			return i;
		}
	}
	return -1;
}

int CPUSimulate::getPID(void) {  // 获取10000以内不重复的整数作为PID
	srand((unsigned)time(0));
	int pid;
	vector<int>::iterator ret;
	do {
		pid = rand() % 10000;  // 最多开10000个进程
		ret = find(this->used_pid.begin(), this->used_pid.end(), pid);
	} while (ret != this->used_pid.end());
	this->used_pid.push_back(pid);
	return pid;
}

void CPUSimulate::initProcess(string filename) {  // 创建进程
	int PID = CPUSimulate::getPID();  // 获取PID
	int pageNo = Memory::reqSpace(DEFAULT_BLOCK, PID, filename);  // 申请内存
	PCB process = PCB(PID, 1, pageNo);  // 创建PCB
	this->READY.push_back(make_pair(PID, process));  // 加入就绪队列
}

void CPUSimulate::blockProcess(ItemRepository* ir, PCB process) {  // 阻塞进程
	std::unique_lock<std::mutex> lock(ir->mtx);  // 互斥访问
	
	int index = findProcess(process.PID, this->RUNNING);
	this->RUNNING.erase(this->RUNNING.begin() + index);
	(ir->itemBuffer).push_back(make_pair(process.PID, process));  // 写入加入阻塞队列
	(ir->repo_not_empty).notify_all();  // 通知队列非空
	lock.unlock();
}

void CPUSimulate::resumeProcess(ItemRepository* ir) {
	std::unique_lock<std::mutex> lock(ir->mtx);


	while (ir->itemBuffer.size() != 0)
	{
		this->READY.push_back(ir->itemBuffer[0]);
		ir->itemBuffer.erase(ir->itemBuffer.begin());
	}

	(ir->repo_not_full).notify_all();
	lock.unlock();

}

string CPUSimulate::IF(PCB process)
{
	//string order = "test";  //这里调用内存提供的接口来获取指令
	//this->order = order;
	//return this->order;

	// 读指令
	string command = Memory::viewMemory(process.pageNo, process.pageId, process.offset, process.PID);
	
	if (command == "") {  // 缺页中断
		blockProcess(&lackpageList, process);

	}
	else if (command == "wrong") {  // 出现错误

	}
	else
	{

	}
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

		default:
			return 0;

		}

	}

}

void CPUSimulate::interrupt(PCB process)
{
	//this->interptLock.lock();
	//if (this->isFinish == true && this->RUNNING.size() == 0) //如果没有进程正在执行
	//{
	//	this->RUNNING.push_back(process);
	//	this->isFinish = false;
	//}
	//else //如果有，先放入中断向量表等待
	//{
	//	this->Lock = true;
	//	this->BREAK.push_back(process);
	//}
	//this->interptLock.unlock();
}

void CPUSimulate::run(int time, char type) //这里暂时没想好
{
	while (time > 0)
	{
		//这里怎么让time和CLOCK同步？？？？？？？
		time--;
	}

}

int main(void) {
	cout << "hello world!" << endl;
}