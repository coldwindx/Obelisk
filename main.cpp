#include <iostream>
#include <string>
#include "log.h"
#include "config/config.h"
#include "thread/thread.h"
#include "thread/mutex.h"
#include "coroutine/coroutine_macro.h"
#include "coroutine/coroutine.h"
#include "coroutine/scheduler.h"

using namespace std;
using namespace obelisk;

int cnt = 0;

Mutex s_mutex;
//RWMutex s_mutex;
void test_config();
void test_thread();
void test_coroutine();
void test_schedule();

int main()
{
	test_config();
	//test_coroutine();
	test_schedule();
	return 0;
}

void func1(){
	LOG_INFO(LOG_SYSTEM()) << "thread: name=" << Thread::GetName()
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
		LOG_INFO(LOG_SYSTEM()) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	}
	//LOG_INFO(g_logger) << "--------------------------------------";
}

void func3(){
	while(true){
		LOG_INFO(LOG_SYSTEM()) << "--------------------------------------";
	}
}

void test_config(){
	LOG_INFO(LOG_SYSTEM()) << "config test begin";
	YAML::Node root = YAML::LoadFile("/home/workspace/Obelisk/bin/conf/logs.yaml");
	Config::loadFromYaml(root);
	Config::visit([](ConfigVarBase::ptr var){
		LOG_INFO(LOG_SYSTEM()) << "name=" << var->name()
				<< " description=" << var->description()
				<< " typename=" << var->valueType()
				<< " value=" << var->toString();
	});
	LOG_INFO(LOG_SYSTEM()) << "config test end";
}

void test_thread(){
	LOG_INFO(LOG_SYSTEM()) << "thread test begin";
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
	LOG_INFO(LOG_SYSTEM()) << "thread test end";
}

Logger::ptr g_logger = LOG_SYSTEM();

void func4(){
	LOG_INFO(g_logger) << "func4 run in coroutine begin";
	//Coroutine::GetSelf()->swapOut();
	Coroutine::YieldToHold();
	LOG_INFO(g_logger) << "func4 run in coroutine end";
	Coroutine::YieldToHold();
}

void test_coroutine(){
	Thread::SetName("main");
	LOG_INFO(g_logger) << "coroutine test in beginning";
	{
		Coroutine::GetSelf();		// 初始化主协程
		LOG_INFO(g_logger) << "coroutine test begin";
		Coroutine::ptr c(new Coroutine(func4));
		LOG_INFO(g_logger) << "coroutine create success";
		c->call();
		LOG_INFO(g_logger) << "coroutine after call";
		c->call();
		LOG_INFO(g_logger) << "coroutine test end";
		c->call();
	}
	LOG_INFO(g_logger) << "coroutine test in ended";
}

void test_schedule(){
	Scheduler sc;
	sc.start();
	sc.stop();
}