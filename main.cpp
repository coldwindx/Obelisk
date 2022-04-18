#include <iostream>
#include <string>
#include "log/log.h"
#include "config/config.h"
#include "log/log_config.h"
#include "thread/thread.h"
#include "thread/scoped_lock.h"

using namespace std;
using namespace obelisk;

int cnt = 0;
//RWMutex s_mutex;
Mutex s_mutex;

Logger::ptr g_logger = LOG_NAME("system");

void func1(){
	LOG_INFO(g_logger) << "thread: name=" << Thread::GetName()
		<< ", this.name=" << Thread::GetThis()->getName()
		<< ", id=" << thread_id()
		<< ", this.id=" << Thread::GetThis()->getId();

	for(int i = 0; i < 1000000; ++i){
		//RWMutex::WriteLock lock(s_mutex);
		Mutex::Lock lock(s_mutex);
		cnt ++;
	}
}

void func2(){

}

int main()
{
	LOG_INFO(g_logger) << "thread test begin";
    vector<Thread::ptr> thrs;
	for(int i = 0; i < 5; ++i){
		Thread::ptr thr(new Thread(&func1, "name_" + to_string(i)));
		thrs.push_back(thr);
	}
	for(int i = 0; i < 5; ++i){
		thrs[i]->join();
	}
	LOG_INFO(g_logger) << "thread test end";
	LOG_INFO(g_logger) << "count = " << cnt;
	return 0;
}