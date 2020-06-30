#pragma once
#include<string>
#include<iostream>
#include<map>
#include<vector>
#include<algorithm>
#include<thread>
#include<mutex>
#include <cstdlib>
#include <condition_variable>
#include<Windows.h>

using namespace std;

// 五种进程状态
#define PRO_NEW 1
#define PRO_READY 2
#define PRO_RUNNING 3
#define PRO_WAITING 4
#define PRO_TERMINATED 5

// 进程默认申请的内存块数
#define DEFAULT_BLOCK 1

//文件申请是读还是写
#define READ 0
#define WRITE 1

// RUNNING队列中能够支持的最大并行数
#define PRO_NUM 3

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
std::mutex mutex;//多线程标准输出同步锁

struct EquipInfo //外设信息
{
	int EquipNum; //设备编号
	int EquipTime; //设备使用时间
	int EquipType; // 设备种类
};

typedef struct FileInfo
{
	int fd;
	string filename;
}FileInfo;

typedef struct DemandInfo // 申请的设备信息
{
	char type;  // 要申请的设备:K键盘/P打印机/D硬盘
	int time;
	int mode = 0;  // 0读1写
	string filename;
	int size = 0;
}DemandInfo;

struct ItemRepository //
{
	vector<pair<int, PCB>> itemBuffer; // 产品缓冲区
	std::mutex mtx; // 互斥量,保护产品缓冲区
	std::condition_variable repo_not_full; // 条件变量, 指示产品缓冲区不为满.
	std::condition_variable repo_not_empty; // 条件变量, 指示产品缓冲区不为空.
} gItemRepository; // 产品库全局变量, 生产者和消费者操作该变量.

typedef struct ItemRepository ItemRepository;


class PCB //PCB相当于就是一个进程类
{
public:
	int PID; //这里没有想好到底怎么分配PID，要不按照创建时间分配？
	int priority;
	int WaitingTime;
	int RunTime;
	string order;


	// 逻辑地址，记录下一条要执行的指令的虚拟地址
	int pageNo;  // 起始页号
	int pageId;  // 页表项编号
	int offset; // 虚拟页内偏移量

	// 设备，记录当前进程在申请的设备信息
	vector<EquipInfo> Equipnum;  // 记录申请的设备的编号
	vector<FileInfo> openFileList;  // 记录打开的文件

	DemandInfo di;  // 申请设备时需要的信息
	int MemDemand;  // 申请内存的大小
	string cwd;

	PCB(int PID, int priority, int pageNo);
	PCB(void);

	~PCB(); // 析构函数，用于进程结束的时候释放进程

public:
	int ProcessInit(); //由于涉及到虚拟地址分配，需要调用外来接口，所以不好使用构造函数创建进程,改用专门的函数创建

};

ItemRepository lackpageList;  // 缺页中断队列
ItemRepository getpageList;  // 调页完成队列
ItemRepository seekMemList;  // 内存申请队列
ItemRepository getMemList;  // 获得内存队列
ItemRepository seekEquipList;  // 设备阻塞队列
ItemRepository getEquipList;  // 获得设备队列


typedef struct BreakRepository
{
	vector<pair<int, int>>BreakQue; // 中断队列,pair的格式为：中断类型，中断的进程PID
	std::mutex BreakLock; // 互斥锁
	std::condition_variable repo_not_full; // 条件变量, 指示产品缓冲区不为满.
	std::condition_variable repo_not_empty; // 条件变量, 指示产品缓冲区不为空.
};
BreakRepository breakList;


class CPUSimulate //模拟CPU运行
{
public:
	bool Interpt = false;  // 记录是否发生了中断事件
	std::mutex interptLock; //中断使用的锁
	std::mutex readyLock; // 就绪队列使用的锁
	string order;  //IF段读取的指令，送入ID段分析
	vector<pair<int, PCB>>READY;  //就绪队列
	vector<pair<int, PCB>>RUNNING; // 运行队列，其实每次只会有一个在运行，但这样方便调度
	vector<int>used_pid;  // 记录已经使用过的pid
	condition_variable Hearwait; //就绪队列的等待，在没有任何进程可以使用的时候wait

	
	void SplitString(const string& s, vector<string>& v, const string& c);//分割字符串的函数
	void initProcess(string filename);  // 创建进程
	void blockProcess(ItemRepository* ir, PCB process);  // 将中断的进程放入对应的阻塞队列中
	void resumeProcess(ItemRepository* ir);  // 将中断完成的指令放回就绪队列中

	int getPID(void);  // 分配PID
	int IF(int pos);  //取指函数,如果取得指令为为NULL,进入缺页中断阻塞，调用内存提供的接口加入阻塞队列
	void CalTime(int time); // 供C指令调用的占用CPU函数
	int ID(PCB process);  //分析指令
	void Preemption();  //比较两个进程的优先级，并对其进行调度
	void ProcessSchedule();  //进程调度，现在的设想是插入排序，在插入的时候直接对优先级进行比较排序

	void interrupt(ItemRepository *ir);  //提供给外部设备以及内存的接口，输入中断的类型
	int ReadApply(string Filename, int time, PCB process); // 读文件，读取指定文件名的文件
	int WriteApply(string Filename, int time, int size, PCB process); // 写文件
	int MemApply(int size, PCB process);  //内存申请，传入需要申请的内存的大小
	int EquipApply(int time, char type, PCB process); //用于进程申请设备
	int PrintApply(string filename, int time, PCB process);  // 用于进程打印文件

	int findProcess(int PID, vector<pair<int, PCB>> processList);  // 在某个队列中找到PCB下标
	void run(); // 将调入的进程按照指令去顺序执行
	void RUN_PROCESS(int pos); //执行RUNNING队列中pos位置的进程
	void Insert(PCB process); // 采用插入排列的方式，将就绪队列中的进程按照优先级从高到低排序,0号为优先级最高的

	void BreakWork(int BreadType); // 分为磁盘，打印机和键盘中断，根据输入的种类打印不同的内容
	void BreakListen(BreakRepository *BreakList); // 监听中断信号队列
};



