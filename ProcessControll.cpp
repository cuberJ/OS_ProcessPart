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


int CPUSimulate::findProcess(int PID, vector<pair<int, PCB>> processList) { // 输入PID，返回这个进程在进程队列中的位置
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

	std::unique_lock<std::mutex> ready(readyLock);
	this->Insert(process);  // 加入就绪队列
	ready.unlock();
}


void CPUSimulate::blockProcess(ItemRepository* ir, PCB process) {  // 阻塞进程，相当于生产者
	std::unique_lock<std::mutex> lock(ir->mtx);  // 互斥访问
	
	int index = findProcess(process.PID, this->RUNNING);
	this->RUNNING.erase(this->RUNNING.begin() + index);  // 从运行队列中删除
	(ir->itemBuffer).push_back(make_pair(process.PID, process));  // 写入加入阻塞队列
	(ir->repo_not_empty).notify_all();  // 通知队列非空
	lock.unlock();
}


void CPUSimulate::resumeProcess(ItemRepository* ir) {  // 将阻塞进程放到就绪队列，相当于消费者,采用了插入排序的方式，按照优先级从大到小的顺序
	std::unique_lock<std::mutex> lock(ir->mtx);


	while (ir->itemBuffer.size() != 0)
	{
		//this->READY.push_back(ir->itemBuffer[0]); 
		this->Insert(ir->itemBuffer[0].second);
		ir->itemBuffer.erase(ir->itemBuffer.begin());
	}

	(ir->repo_not_full).notify_all();
	lock.unlock();

}


int CPUSimulate::IF(int pos) // 返回值为表明是否取指成功的操作码
{
	// 读指令
	string command = Memory::viewMemory(this->RUNNING[pos].second.pageNo, this->RUNNING[pos].second.pageId, this->RUNNING[pos].second.offset, this->RUNNING[pos].second.PID);
	
	if (command == "") {  // 缺页中断
		blockProcess(&lackpageList, this->RUNNING[pos].second);
		return -1;
	}
	else if (command == "wrong") {  // 出现错误
		cout << "something wrong with IF" << endl;
		// 后期在调度处是否需要加入杀死进程的处理？
		return -2;
	}
	else
	{
		this->order = command;

		// 取指之后，修改指令的取指地址，指向下一条指令的地址
		if (this->RUNNING[pos].second.offset == 0)
			this->RUNNING[pos].second.offset += 16;
		else
		{
			this->RUNNING[pos].second.offset = 0;
			this->RUNNING[pos].second.pageId += 1;
		}
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


int CPUSimulate::ID(PCB process) // 返回0表示进程可以继续执行，不需要调度。返回1表示进程已经被调入某个阻塞队列中
{
	string order = this->order;
	
	switch (order[0])
	{
	case 'C':  // 模拟进程使用CPU，时长time
	{
		int time = 0;
		order.erase(order.begin());
		time = stoi(order);
		this->CalTime(time);
		return 0;
	}
	
	case 'K':  // 模拟进程从键盘输入，时长time
	{
		vector<string>data;
		this->SplitString(order, data, " ");
		this->EquipApply(stoi(data[1]), 'K', process);
		//调用设备提供的接口，放入设备等待队列中，并提供执行的时间
		return 1;
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
		return 1;
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
		return 1;
	}

	case 'M':  // 模拟进程申请内存，大小block
	{
		int size = 0;
		order.erase(order.begin());
		size = stoi(order);
		this->MemApply(size, process);
		//调用内存提供的内存分配接口,加入分配等待队列
		return 1;
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
		return 1;
	}

	case 'p':  // print指令，将文件名发送给打印机
	{
		vector<string>data;
		this->SplitString(order, data, " ");
		string filename = data[1];
		int time = stoi(data[2]);
		this->PrintApply(filename, time, process);
		return 1;
	}

	default:
		cout << "invalid command: " << order << endl;
		return 2;
	}
}


void CPUSimulate::interrupt(ItemRepository *ir)
{
	std::unique_lock<std::mutex> run(this->interptLock);
	ir->repo_not_empty.wait(run);

	std::unique_lock<std::mutex> ready(readyLock);
	this->resumeProcess(ir);
	// -----------------------这里执行一次调度检查，判断是否有需要抢占RUNNING的进程，如果有，调入RUNNING------------------

	ready.unlock();
	run.unlock();
}


void CPUSimulate::CalTime(int time)  // 利用现实时间模拟对CPU的占用
{
	while (time > 0)
	{
		cout << "remaining time:" << time << endl;
		time--;
		Sleep(1000);
	}
	
}

void CPUSimulate::run() // 运行函数
{
	// 4个监听队列的进程，依次分别监听：缺页中断，内存，设备,以及设备的中断队列
	thread HearingPage(&interrupt, getpageList);
	thread HearingMem(&interrupt, getMemList);
	thread HearingEquip(&interrupt, getEquipList);
	thread HearingBreak(&BreakListen, breakList);
	HearingPage.join();
	HearingMem.join();
	HearingEquip.join();
	HearingBreak.join();

	//开始持续运行进程部分
	while (true)
	{

		// ---------------------这里补充：调度模块向RUNNING中放入进程，注意处理一下这种情况：就绪队列和运行队列都是空的时候，需要等待---------------------------
		
		for (int i = 0;!RUNNING.empty() ; i = (i+1)%RUNNING.size())  // RUNNING队列中有最多三个进程，每个进程循环执行两个指令
		{
			std::unique_lock<std::mutex> lock(interptLock);
			this->RUN_PROCESS(i);
			// -----------------------------执行完一个进程的时间片后，检查就绪队列的优先级情况，判断是否需要调度-------------------------------
			lock.unlock();
		}
	}
}

void CPUSimulate::RUN_PROCESS(int pos)
{
	for (int i = 0; i < 2; i++) //每个进程分配执行两条指令的时间片
	{
		if (this->IF(pos) == 0)  //取指成功
		{
			int code = this->ID(this->RUNNING[pos].second);
			if (code != 0) // 如果这次执行的是一个会引起调度的指令
			{
				// 由于会产生调度，所以当前的Running进程会被换出
				RUNNING[pos].second.priority += 1;
				std::unique_lock<std::mutex> lock(readyLock);
				// -------------------------------这里采用优先级方式选择下一个需要处理的进程，换入RUNNING队列---------------------------
				readyLock.unlock();
				return;
			}

		}
		else  // 取指不成功，此时RUNNING[i]进程已经被调走，需要通过调度模块向RUNNING中加入一个新的进程
		{
			std::unique_lock<std::mutex> lock(readyLock);
			// ------------------------------------这里同样用调度模块加入新的进程-----------------------------------
			readyLock.unlock();
			cout << "加入新的进程" << endl;
			return;
		}
	}
}

void CPUSimulate::Insert(PCB process)
{
	for (int i = 0; i < this->READY.size(); i++)
	{
		if (this->READY[i].second.priority < process.priority)
		{
			this->READY.insert(READY.begin() + i, make_pair(process.PID, process));
			return;
		}
	}
	READY.push_back(make_pair(process.PID, process));
}


void CPUSimulate::BreakListen(BreakRepository *breakList)
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(breakList->BreakLock);
		breakList->repo_not_empty.wait(lock);
		std::unique_lock<std::mutex> CPULock(interptLock); // 将当前CPU上正在运行的进程全部暂停


		while (!breakList->BreakQue.empty())
		{
			this->BreakWork(breakList->BreakQue[0].first);
			breakList->BreakQue.erase(breakList->BreakQue.begin());
		}
		(breakList->repo_not_full).notify_all();
		CPULock.unlock();
		lock.unlock();
	}
}

void CPUSimulate::BreakWork(int BreakType)
{
	switch (BreakType)
	{
		case 1:printf("计算机正在执行打印机中断...");
			break;
		case 2:printf("计算机正在执行键盘中断...");
			break;
		case 3:printf("计算机正在执行磁盘中断...");
			break;
		default:break;
	}
}


int main(void) {
	cout << "hello world!" << endl;

}