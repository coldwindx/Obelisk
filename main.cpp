#include <iostream>
#include <string>
#include "log/log.h"
#include "config/config.h"
#include "log/log_initer.h"
#include "thread/thread.h"
#include "thread/mutex.h"

using namespace std;
using namespace obelisk;

int cnt = 0;

Mutex s_mutex;
//RWMutex s_mutex;

Logger::ptr g_logger = LOG_NAME("system");

void func1(){
	LOG_INFO(g_logger) << "thread: name=" << Thread::GetName()
		<< ", this.name=" << Thread::GetThis()->getName()
		<< ", id=" << thread_id()
		<< ", this.id=" << Thread::GetThis()->getId();

	for(int i = 0; i < 1000000; ++i){
		//WriteScopedLock<RWMutex> lock(s_mutex);
		ScopedLock<Mutex> lock(s_mutex);
		cnt ++;
	}
}

void func2(){
	while(true){
		LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	}
	//LOG_INFO(g_logger) << "--------------------------------------";
}

void func3(){
	while(true){
		LOG_INFO(g_logger) << "--------------------------------------";
	}
}

int main()
{
	LOG_INFO(g_logger) << "config test begin";
	YAML::Node root = YAML::LoadFile("/home/workspace/Obelisk/bin/conf/logs.yaml");
	Config::loadFromYaml(root);
	LOG_INFO(g_logger) << "config test end";
	LOG_INFO(g_logger) << "thread test begin";
    vector<Thread::ptr> thrs;
	for(int i = 0; i < 2; ++i){
		Thread::ptr thr1(new Thread(&func2, "name_" + to_string(i * 2)));
		Thread::ptr thr2(new Thread(&func3, "name_" + to_string(i * 2 + 1)));
		thrs.push_back(thr1);
		thrs.push_back(thr2);
	}
	for(int i = 0; i < thrs.size(); ++i){
		thrs[i]->join();
	}
	LOG_INFO(g_logger) << "thread test end";
	return 0;
}