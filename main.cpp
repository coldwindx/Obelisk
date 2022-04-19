#include <iostream>
#include <string>
#include "log.h"
#include "config/config.h"
#include "thread/thread.h"
#include "thread/mutex.h"
#include "coroutine/utils.h"
#include "coroutine/macro.h"

using namespace std;
using namespace obelisk;

int cnt = 0;

Mutex s_mutex;
//RWMutex s_mutex;
void test_config();
void test_thread();
void test_assert();

int main()
{
	test_config();
	test_assert();
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

void test_assert(){
	//LOG_INFO(LOG_SYSTEM()) << endl << BacktraceToString(10, 0, "     ");
	OBELISK_ASSERT2(false, "This is a assert");
}