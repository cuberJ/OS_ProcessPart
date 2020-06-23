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


void CPUSimulate::blockProcess(ItemRepository* ir, PCB process) {  // 阻塞进程，相当于生产者
	std::unique_lock<std::mutex> lock(ir->mtx);  // 互斥访问
	
	int index = findProcess(process.PID, this->RUNNING);
	this->RUNNING.erase(this->RUNNING.begin() + index);  // 从运行队列中删除
	(ir->itemBuffer).push_back(make_pair(process.PID, process));  // 写入加入阻塞队列
	(ir->repo_not_empty).notify_all();  // 通知队列非空
	lock.unlock();
}


void CPUSimulate::resumeProcess(ItemRepository* ir) {  // 将阻塞进程放到就绪队列，相当于消费者
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
		cout << "something wrong with IF" << endl;
		// 后期在调度处是否需要加入杀死进程的处理？
	}
	else
	{
		this->order = command;
		return 0;
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


int CPUSimulate::EquipApply(int time, char type, PCB process) {
	process.di.time = time;
	process.di.type = type;
	blockProcess(&seekEquipList, process);
	return 0;
}


int CPUSimulate::ReadApply(string Filename, int time, PCB process) {
	process.di.type = 'D';
	process.di.mode = READ;
	process.di.filename = Filename;
	process.di.time = time;
	blockProcess(&seekEquipList, process);
	return 0;
}


int CPUSimulate::WriteApply(string Filename, int time, int size, PCB process) {
	process.di.type = 'D';
	process.di.mode = WRITE;
	process.di.filename = Filename;
	process.di.time = time;
	process.di.size = size;
	blockProcess(&seekEquipList, process);
	return 0;
}


int CPUSimulate::MemApply(int size, PCB process) {
	process.MemDemand = size;
	blockProcess(&seekMemList, process);
	return 0;
}


int CPUSimulate::PrintApply(string filename, int time, PCB process) {
	process.di.type = 'p';
	process.di.filename = filename;
	process.di.time = time;
	blockProcess(&seekEquipList, process);
}


int CPUSimulate::ID(PCB process)
{
	string order = this->order;
	
	switch (order[0])
	{
	case 'C':  // 模拟进程使用CPU，时长time
	{
		int time = 0;
		order.erase(order.begin());
		time = stoi(order);
		this->run(time);
		return 0;
	}
	
	case 'K':  // 模拟进程从键盘输入，时长time
	{
		vector<string>data;
		this->SplitString(order, data, " ");
		this->EquipApply(stoi(data[1]), 'K', process);
		//调用设备提供的接口，放入设备等待队列中，并提供执行的时间
		return 0;
	}

	case 'P':  // 模拟进程使用打印机，时长time
	{
		vector<string>data;
		this->SplitString(order, data, " ");
		this->EquipApply(stoi(data[1]), 'P', process);
		//调用设备打印机提供的接口，放入打印机等待队列中,并提供执行的时间
		return 0;

	}

	case 'R':  // 模拟进程读取文件，文件名filename， 时长time
	{
		vector<string>data;
		this->SplitString(order, data, " ");
		string filename = data[1];
		int time = stoi(data[2]);
		this->ReadApply(filename, time, process);
		//将进程调入文件提供的读文件阻塞队列
		return 0;
	}

	case 'W':  // 模拟进程写文件，文件名filename，时长time，大小size
	{
		vector<string>data;
		this->SplitString(order, data, " ");
		string filename = data[1];
		int time = stoi(data[2]);
		int size = stoi(data[3]);
		this->WriteApply(filename, time, size, process);
		//将进程调入文件提供的阻塞队列
		return 0;
	}

	case 'M':  // 模拟进程申请内存，大小block
	{
		int size = 0;
		order.erase(order.begin());
		size = stoi(order);
		this->MemApply(size, process);
		//调用内存提供的内存分配接口,加入分配等待队列
		return 0;
	}

	case 'Y':  // 进程的优先数number
	{
		int priority = 0;
		order.erase(order.begin());
		priority = stoi(order);
		this->RUNNING[0].second.priority = priority; // 修改优先级
		return 0;
	}

	case 'Q':  // 进程结束
	{
		Memory::rlsSpace(process.pageNo);
		int index = findProcess(process.PID, this->RUNNING);
		this->RUNNING.erase(this->RUNNING.begin() + index);
		return 0;
	}

	case 'p':  // print指令，将文件名发送给打印机
	{
		vector<string>data;
		this->SplitString(order, data, " ");
		string filename = data[1];
		int time = stoi(data[2]);
		this->PrintApply(filename, time, process);
		return 0;
	}

	default:
		cout << "invalid command: " << order << endl;
		return 1;
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


void CPUSimulate::run(int time)  // 利用现实时间模拟对CPU的占用
{
	while (time > 0)
	{
		cout << "remaining time:" << time << endl;
		time--;
		Sleep(1000);
	}
}


int main(void) {
	cout << "hello world!" << endl;
}