#pragma once
#include<string>
#include<iostream>
#include<map>
#include<vector>
#include<algorithm>
#include<thread>
#include<mutex>
using namespace std;

/*
1. 主线程执行流程应当为：从就绪队列调入进程PCB到CPU类，调用取指函数IF
   如果取指返回的结果是“NULL”，引发缺页中断，进程进入阻塞
   分析指令，按照指令内容执行（申请内存，释放内存，加入外设等待队列，加入IO读写队列，结束进程）
   指令执行需要时间，将时间传入CPU类的run函数（创建线程1），令其模拟运行时间，同时修改isFinish参数。
   将PCB放入指令对应的等待队列中，等模拟运行时间到，换下一个进程

2. 分支线程1：三种阻塞跳转到的线程（不包括缺页中断）
	分别调用内存提供的申请内存函数，设备申请函数，文件读写函数，在三种函数执行完毕后，调用CPU的interrupt函数查看CPU当前是否有正在运行进程B的指令
	如果没有，直接将当前进程放入CPU
	如果有，修改CPU的参数LOCK，锁定run，当run当前的指令执行完毕后，比较两个进程的优先级。

	*****重点******
	现在完成IO之类的引发中断的进程为A，正在CPU上跑的是进程B。
	如果A的优先级高于B，将A放入CPU，B放入中断队列。
	如果A优先级低于B，A放入就绪队列（不是中断队列），B继续运行

3. 分支线程3：缺页中断跳转到的线程：
	调用内存提供的函数（暂且未知），等待返回后，同样调用interrupt，后面与2同理

4.  一些杂七杂八的小细节：
	我们的结构里没有阻塞队列，阻塞队列由内存，设备，文件来提供。
	同理，这三个线程也是他们来开，不需要我们负责。我们只需要调用他们给的接口传入数据。再简单点说，我们只负责1里提到的主线程（如果我没理解错的话）
	关于进程执行时间的模拟，我感觉还有点漏洞，但说不上来。你们再想一下看看有没有更好的方法
			
	 */	 

int CLOCK = 0;  // 总时钟

struct EquipInfo //外设信息
{
	int EquipNum; //设备编号
	int EquipTime; //设备使用时间
	int EquipType; // 设备种类
};


class PCB //PCB相当于就是一个进程类
{
public:
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

	PCB(int priority);

	~PCB(); // 析构函数，用于进程结束的时候释放进程

public:
	int ProcessInit(); //由于涉及到虚拟地址分配，需要调用外来接口，所以不好使用构造函数创建进程,改用专门的函数创建

};


class CPUSimulate //模拟CPU运行
{
public:
	bool isFinish = true; //标记当前执行的指令是否执行完毕
	bool Lock = false; // 运行锁，当interrupt执行的时候将其锁住，禁止下一个指令进入
	mutex interptLock; //中断使用的锁
	string order;  //IF段读取的指令，送入ID段分析
	vector<PCB>READY;  //就绪队列
	vector<PCB>BREAK;  // 中断队列
	vector<PCB>RUNNING; // 运行队列，其实每次只会有一个在运行，但这样方便调度
	void SplitString(const string& s, vector<string>& v, const string& c);//分割字符串的函数

	string IF();  //取指函数,如果取得指令为为NULL,进入缺页中断阻塞，调用内存提供的接口加入阻塞队列
	void run(int time, char type); //输入指令运行的时间和种类,对于没有明确执行时间的指令，默认为1
	int ID();  //分析指令
	int Priority(PCB a, PCB b);  //比较两个进程的优先级，并对其进行调度
	void ProcessSchedule();  //进程调度，现在的设想是插入排序，在插入的时候直接对优先级进行比较排序

	void interrupt(PCB process);  //提供给外部设备以及内存的接口，输入中断的类型
	int ReadApply(string Filename, int time, int size); // 读文件，读取指定文件名的文件
	int WriteApply(string Filename, int time, int size); // 写文件
	int MemApply(int size);  //内存申请，传入需要申请的内存的大小
	int EquipApply(int time, char type); //用于进程申请设备
};



