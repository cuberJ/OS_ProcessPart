#include "Basic.h"

PCB::PCB(int priority)
{
	this->PageIndex = 0;
	this->PageNumber = 0;
	this->WaitingTime = 0;
	this->priority = 0;
	this->RunTime = 0;
	//�����ڴ��ṩ�Ľӿڣ���ȡ����������ַ
}

string CPUSimulate::IF()
{
	string order = "test";  //��������ڴ��ṩ�Ľӿ�����ȡָ��
	this->order = order;
}

void CPUSimulate::SplitString(const string& s, vector<string>& v, const string& c)
{
	string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (string::npos != pos2)
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
	if (order == "BREAK")  // ȱҳ�ж�
	{
		cout << "ȱҳ�ж�" << endl;  //��������ڴ��ṩ�Ľӿ�
		//���ｫthis->RUNNING[0]�����ڴ��ṩ�ĵȴ�����
		return 0;
	}
	//����ִ����֮�����ֱ��ִ����һ��������Ҫ�ж��Ƿ�������ָ�����1�����������ģ�����0
	else
	{
		switch (order[0])
		{
			case 'K':
			{
				vector<string>data;
				this->SplitString(order, data, " ");
				this->EquipApply(stoi(data[1]),'K');
				//�����豸�ṩ�Ľӿڣ�������̵ȴ������У����ṩִ�е�ʱ��
				return 0;
			}

			case 'P':
			{
				vector<string>data;
				this->SplitString(order, data, " ");
				this->EquipApply(stoi(data[1]), 'P');
				//�����豸��ӡ���ṩ�Ľӿڣ������ӡ���ȴ�������,���ṩִ�е�ʱ��
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
				//�����ڴ��ṩ���ڴ����ӿ�,�������ȴ�����
				return 0;
			}

			case 'F':
			{
				int size = 0;
				order.erase(order.begin());
				size = stoi(order);
				//�����ڴ��ṩ���ͷŽӿ�
				return 1;
			}

			case 'Y':
			{
				int priority = 0;
				order.erase(order.begin());
				priority = stoi(order);
				this->RUNNING[0].priority = priority; // �޸����ȼ�
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
				//�����̵����ļ��ṩ����������
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
				//�����̵����ļ��ṩ�Ķ��ļ���������
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
	if (this->isFinish == true && this->RUNNING.size() == 0) //���û�н�������ִ��
	{
		this->RUNNING.push_back(process);
		this->isFinish = false;
	}
	else //����У��ȷ����ж�������ȴ�
	{
		this->Lock = true;
		this->BREAK.push_back(process);
	}
	this->interptLock.unlock();
}

void CPUSimulate::run(int time, char type) //������ʱû���
{
	while (time > 0)
	{
		//������ô��time��CLOCKͬ����������������
		time--;
	}
	
}