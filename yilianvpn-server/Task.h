#ifndef TASK_H
#define TASK_H
#include <iostream>
#include <vector>
using namespace std;

std::vector<void *> tasks;

class Task
{
public:
	//是否退出
	bool isQuit;
	int tick;
	int loopCount;

	Task()
	{
		UID++;
		id = UID;
		isQuit = false;
		tick = 500;
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

int task_loop()
{
	int ret = 0;
	for(int i = 0; i < tasks.size(); i++)
	{
		Task *task = (Task *)tasks[i];
		if(task->isQuit)
		{
			task->clearJob();
			tasks.erase(tasks.begin() + i);
			i--;
			continue;
		}
		if(task->tick > 0)
		{
			task->tick--;
			ret++;
			continue;
		}
		bool addRet = true;
		for(int j = 0; j < task->loopCount; j++)
		{
			bool exit = task->loop();
			if(exit)
			{
				task->isQuit = true;
				task->clearJob();
				tasks.erase(tasks.begin() + i);
				i--; 
				addRet = false;
				j = task->loopCount;
			}
			if(task->tick > 0)
			{
				j = task->loopCount;
			}
		}
		if(addRet)
		{
			ret++;
		}

	}
	return ret;
}

#endif


