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

// ���ֽ���״̬
#define PRO_NEW 1
#define PRO_READY 2
#define PRO_RUNNING 3
#define PRO_WAITING 4
#define PRO_TERMINATED 5

// ����Ĭ��������ڴ����
#define DEFAULT_BLOCK 1

//�ļ������Ƕ�����д
#define READ 0
#define WRITE 1

// RUNNING�������ܹ�֧�ֵ��������
#define PRO_NUM 3

/*
1. ���߳�ִ������Ӧ��Ϊ���Ӿ������е������PCB��CPU�࣬����ȡָ����IF
   ���ȡָ���صĽ���ǡ�NULL��������ȱҳ�жϣ����̽�������
   ����ָ�����ָ������ִ�У������ڴ棬�ͷ��ڴ棬��������ȴ����У�����IO��д���У��������̣�
   ָ��ִ����Ҫʱ�䣬��ʱ�䴫��CPU���run�����������߳�1��������ģ������ʱ�䣬ͬʱ�޸�isFinish������
   ��PCB����ָ���Ӧ�ĵȴ������У���ģ������ʱ�䵽������һ������

2. ��֧�߳�1������������ת�����̣߳�������ȱҳ�жϣ�
	�ֱ�����ڴ��ṩ�������ڴ溯�����豸���뺯�����ļ���д�����������ֺ���ִ����Ϻ󣬵���CPU��interrupt�����鿴CPU��ǰ�Ƿ����������н���B��ָ��
	���û�У�ֱ�ӽ���ǰ���̷���CPU
	����У��޸�CPU�Ĳ���LOCK������run����run��ǰ��ָ��ִ����Ϻ󣬱Ƚ��������̵����ȼ���

	*****�ص�******
	�������IO֮��������жϵĽ���ΪA������CPU���ܵ��ǽ���B��
	���A�����ȼ�����B����A����CPU��B�����ж϶��С�
	���A���ȼ�����B��A����������У������ж϶��У���B��������

3. ��֧�߳�3��ȱҳ�ж���ת�����̣߳�
	�����ڴ��ṩ�ĺ���������δ֪�����ȴ����غ�ͬ������interrupt��������2ͬ��

4.  һЩ�����Ӱ˵�Сϸ�ڣ�
	���ǵĽṹ��û���������У������������ڴ棬�豸���ļ����ṩ��
	ͬ���������߳�Ҳ����������������Ҫ���Ǹ�������ֻ��Ҫ�������Ǹ��Ľӿڴ������ݡ��ټ򵥵�˵������ֻ����1���ᵽ�����̣߳������û����Ļ���
	���ڽ���ִ��ʱ���ģ�⣬�Ҹо����е�©������˵����������������һ�¿�����û�и��õķ���
			
	 */	 

int CLOCK = 0;  // ��ʱ��
std::mutex mutex;//���̱߳�׼���ͬ����

struct EquipInfo //������Ϣ
{
	int EquipNum; //�豸���
	int EquipTime; //�豸ʹ��ʱ��
	int EquipType; // �豸����
};

typedef struct FileInfo
{
	int fd;
	string filename;
}FileInfo;

typedef struct DemandInfo // ������豸��Ϣ
{
	char type;  // Ҫ������豸:K����/P��ӡ��/DӲ��
	int time;
	int mode = 0;  // 0��1д
	string filename;
	int size = 0;
}DemandInfo;

struct ItemRepository //
{
	vector<pair<int, PCB>> itemBuffer; // ��Ʒ������
	std::mutex mtx; // ������,������Ʒ������
	std::condition_variable repo_not_full; // ��������, ָʾ��Ʒ��������Ϊ��.
	std::condition_variable repo_not_empty; // ��������, ָʾ��Ʒ��������Ϊ��.
} gItemRepository; // ��Ʒ��ȫ�ֱ���, �����ߺ������߲����ñ���.

typedef struct ItemRepository ItemRepository;


class PCB //PCB�൱�ھ���һ��������
{
public:
	int PID; //����û����õ�����ô����PID��Ҫ�����մ���ʱ����䣿
	int priority;
	int WaitingTime;
	int RunTime;
	string order;


	// �߼���ַ����¼��һ��Ҫִ�е�ָ��������ַ
	int pageNo;  // ��ʼҳ��
	int pageId;  // ҳ������
	int offset; // ����ҳ��ƫ����

	// �豸����¼��ǰ������������豸��Ϣ
	vector<EquipInfo> Equipnum;  // ��¼������豸�ı��
	vector<FileInfo> openFileList;  // ��¼�򿪵��ļ�

	DemandInfo di;  // �����豸ʱ��Ҫ����Ϣ
	int MemDemand;  // �����ڴ�Ĵ�С
	string cwd;

	PCB(int PID, int priority, int pageNo);
	PCB(void);

	~PCB(); // �������������ڽ��̽�����ʱ���ͷŽ���

public:
	int ProcessInit(); //�����漰�������ַ���䣬��Ҫ���������ӿڣ����Բ���ʹ�ù��캯����������,����ר�ŵĺ�������

};

ItemRepository lackpageList;  // ȱҳ�ж϶���
ItemRepository getpageList;  // ��ҳ��ɶ���
ItemRepository seekMemList;  // �ڴ��������
ItemRepository getMemList;  // ����ڴ����
ItemRepository seekEquipList;  // �豸��������
ItemRepository getEquipList;  // ����豸����


typedef struct BreakRepository
{
	vector<pair<int, int>>BreakQue; // �ж϶���,pair�ĸ�ʽΪ���ж����ͣ��жϵĽ���PID
	std::mutex BreakLock; // ������
	std::condition_variable repo_not_full; // ��������, ָʾ��Ʒ��������Ϊ��.
	std::condition_variable repo_not_empty; // ��������, ָʾ��Ʒ��������Ϊ��.
};
BreakRepository breakList;


class CPUSimulate //ģ��CPU����
{
public:
	bool Interpt = false;  // ��¼�Ƿ������ж��¼�
	std::mutex interptLock; //�ж�ʹ�õ���
	std::mutex readyLock; // ��������ʹ�õ���
	string order;  //IF�ζ�ȡ��ָ�����ID�η���
	vector<pair<int, PCB>>READY;  //��������
	vector<pair<int, PCB>>RUNNING; // ���ж��У���ʵÿ��ֻ����һ�������У��������������
	vector<int>used_pid;  // ��¼�Ѿ�ʹ�ù���pid
	condition_variable Hearwait; //�������еĵȴ�����û���κν��̿���ʹ�õ�ʱ��wait

	
	void SplitString(const string& s, vector<string>& v, const string& c);//�ָ��ַ����ĺ���
	void initProcess(string filename);  // ��������
	void blockProcess(ItemRepository* ir, PCB process);  // ���жϵĽ��̷����Ӧ������������
	void resumeProcess(ItemRepository* ir);  // ���ж���ɵ�ָ��Żؾ���������

	int getPID(void);  // ����PID
	int IF(int pos);  //ȡָ����,���ȡ��ָ��ΪΪNULL,����ȱҳ�ж������������ڴ��ṩ�Ľӿڼ�����������
	void CalTime(int time); // ��Cָ����õ�ռ��CPU����
	int ID(PCB process);  //����ָ��
	void Preemption();  //�Ƚ��������̵����ȼ�����������е���
	void ProcessSchedule();  //���̵��ȣ����ڵ������ǲ��������ڲ����ʱ��ֱ�Ӷ����ȼ����бȽ�����

	void interrupt(ItemRepository *ir);  //�ṩ���ⲿ�豸�Լ��ڴ�Ľӿڣ������жϵ�����
	int ReadApply(string Filename, int time, PCB process); // ���ļ�����ȡָ���ļ������ļ�
	int WriteApply(string Filename, int time, int size, PCB process); // д�ļ�
	int MemApply(int size, PCB process);  //�ڴ����룬������Ҫ������ڴ�Ĵ�С
	int EquipApply(int time, char type, PCB process); //���ڽ��������豸
	int PrintApply(string filename, int time, PCB process);  // ���ڽ��̴�ӡ�ļ�

	int findProcess(int PID, vector<pair<int, PCB>> processList);  // ��ĳ���������ҵ�PCB�±�
	void run(); // ������Ľ��̰���ָ��ȥ˳��ִ��
	void RUN_PROCESS(int pos); //ִ��RUNNING������posλ�õĽ���
	void Insert(PCB process); // ���ò������еķ�ʽ�������������еĽ��̰������ȼ��Ӹߵ�������,0��Ϊ���ȼ���ߵ�

	void BreakWork(int BreadType); // ��Ϊ���̣���ӡ���ͼ����жϣ���������������ӡ��ͬ������
	void BreakListen(BreakRepository *BreakList); // �����ж��źŶ���
};



