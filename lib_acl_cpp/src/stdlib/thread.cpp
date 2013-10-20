#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/thread.hpp"

namespace acl
{

thread::thread()
: detachable_(true)
, stack_size_(0)
, thread_id_(0)
{
#ifdef WIN32
	thread_ = (acl_pthread_t*) acl_mycalloc(1, sizeof(acl_pthread_t));
#endif
	return_arg_ = NULL;
}

thread::~thread()
{
#ifdef WIN32
	acl_myfree(thread_);
#endif
}

thread& thread::set_detachable(bool yes /* = true */)
{
	detachable_ = yes;
	return *this;
}

thread& thread::set_stacksize(size_t size)
{
	stack_size_ = size;
	return *this;
}

void* thread::thread_run(void* arg)
{
	thread* thr = (thread*) arg;
	thr->return_arg_ = thr->run();
	return thr->return_arg_;
}

bool thread::start()
{
	acl_pthread_attr_t attr;
	acl_pthread_attr_init(&attr);

	if (detachable_)
		acl_pthread_attr_setdetachstate(&attr, 1);
	if (stack_size_ > 0)
		acl_pthread_attr_setstacksize(&attr, stack_size_);

#ifdef WIN32
	int   ret = acl_pthread_create((acl_pthread_t*) thread_,
		&attr, thread_run, this);
#else
	int   ret = acl_pthread_create(&thread_id_, &attr, thread_run, this);
#endif
	if (ret != 0)
	{
		acl_set_error(ret);
		logger_error("create thread error", last_serror());
		return false;
	}

#ifdef WIN32
	thread_id_ = ((acl_pthread_t*) thread_)->id;
#endif

	return true;
}

bool thread::wait(void** out /* = NULL */)
{
	if (thread_id_ == 0)
	{
		logger_error("thread not running!");
		return false;
	}

	if (detachable_)
	{
		logger_error("detachable thread can't be wait!");
		return false;
	}

	void* ptr;

#ifdef WIN32
	int   ret = acl_pthread_join(*((acl_pthread_t*) thread_), &ptr);
#else
	int   ret = acl_pthread_join(thread_id_, &ptr);
#endif

	if (ret != 0)
	{
		acl_set_error(ret);
		logger_error("pthread_join error: %s", last_serror());
		return false;
	}

	// �Ƚ�ͨ���� thread_run �нػ�Ĳ����� pthread_join ��õĲ����Ƿ���ͬ
	if (ptr != return_arg_)
		logger_warn("pthread_josin's arg invalid?");

	if (out)
		*out = ptr;
	return true;
}

unsigned long thread::thread_id() const
{
	return thread_id_;
}

unsigned long thread::thread_self()
{
	return (unsigned long) acl_pthread_self();
}

} // namespace acl