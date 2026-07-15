#ifndef TASK_H
#define TASK_H
#include <iostream>
#include <vector>
using namespace std;

std::vector<void *> tasks;
struct taskInfo
{
	int size;
	int destoryed;
	int sleeped;
}; 

class Task
{
public:
	//是否退出
	bool isQuit;
	int asleep;
	int loopCount;

	Task()
	{
		UID++;
		id = UID;
		isQuit = false;
		asleep = 500;
		loopCount = 1;
		create_task(this);
	}

	~Task()
	{
		for (int i = 0; i < tasks.size(); i++)
		{
			Task *task = (Task *)tasks[i];
			if(*task == *this)
			{
				tasks.erase(tasks.begin() + i);
				break;
			}
		}
		clearJob();
	}

	void quit()
	{
		isQuit = true;
	}

	// 虚构函数loop
	virtual bool loop()
	{
		return false;
	}
	
	// 虚构函数clearJob
	virtual bool clearJob()
	{
		return false;
	}

	bool operator==(const Task &task)
	{
		return id == task.id;
	}

	long getId()
	{
		return id;
	}

	void create_task(Task *task)
	{
		tasks.push_back(task);
	}

private:
	// 任务id
	long id;
	static long UID;
};

long Task::UID = 0;

struct taskInfo task_loop()
{
	struct taskInfo info = {0};
	for(int i = 0; i < tasks.size(); i++)
	{
		Task *task = (Task *)tasks[i];
		if(task->isQuit)
		{
			tasks.erase(tasks.begin() + i);
			i--;
			info.destoryed++;
			continue;
		} 
		if(task->asleep > 0)
		{
			task->asleep--;
			info.sleeped++;
			continue;
		}
		int lcount = task->loopCount; 
		for(int j = 0; j < lcount; j++)
		{
			bool exit = task->loop();
			if(exit)
			{
				task->quit();
				tasks.erase(tasks.begin() + i);
				i--;
				info.destoryed++;
				break;
			}
			if(task->asleep > 0)
			{
				info.sleeped++;
				break;
			}
		}
	}
	info.size = tasks.size();
	return info;
}

#endif


