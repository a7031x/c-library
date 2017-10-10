/*
	Author: HouSD
	Date: 2013/2/22

	The library is responsible for accessing thread local storage (TLS) and doing finishing works when current thread is about to exit. As expected, this library is THREAD SAFE.
	The library requires boost SDK.
	Though one can access TLS using TlsSetValue and TlsGetValue, this library gets the other-wise frustrating work handy.
	In this scenario, you can access arbitrary type of named value which will retain the last value set to it.
	If you try to access a value that has not been set yet, it will be created automatically.
	All the values accessed in the current thread will be erased when the thread exits.

	Here goes an example:

	#include <thread>	//Suppose that you are currently using VC11 or later version. Otherwise, you should resort to the CreateThread to create a thread.

	void procedure()	//This is the core thread function.
	{
		//Set the named value named L"abc" to 100, which is an integer implicitly.
		//If the value is not set yet, it will be automatically created and initialized.

		threading::value(L"abc") = 100;


		//Set the named value L"def" to a smart pointer.
		//If you directly set the value to a pointer typed SomeClass*, it won't be released when the thread exits, though the pointer itself will be erased.
		//Instead, using smart_pointer which automatically manages the underlying pointer is a better practice.
		//Of course, you can simply set the value to the object entity directly, such as threading::value(L"def") = SomeClass();
		//But this implementation requires an extra local construction.
		//To avoid performance decline of constructing large object as well as the underneath two-leveled pointer, you can leverage the right reference feature introduced in VC11:
		//threading::value(L"def") = std::move(SomeClass());
		//The std::move implementation requires a SomeClass(SomeClass&& other) constructor, inside which moving all data from the object named other to the current object.
		//It nevertheless doesn't worth such effort while the simple make_shared call satisfying most circumstances.


		threading::value(L"def") = make_shared<SomeClass>();


		//The following snippet illustrates how to schedule the handle to be closed on exiting the thread.
		//The underlying CloseHandle will be called with each handle added by close_handle_on_existing passed it.

		HANDLE event = CreateMutex(nullptr, true, L"test_mutex")
		threading::close_handle_on_existing(event);
		

		//queue_exit_operation is designed for the more generic work which can't be achieved through the above methods.

		threading::queue_exit_operation(
				[]()
				{
					//Note that the exiting operations are always called preceding any named values.
					TerminateProcess(any_cast<HANDLE>(threading::value(L"test_process")), 0);
				}
			);
	}

	int _tmain(int argc, _TCHAR* argv[])
	{
		thread t(procedure);
		t.join();
		return 0;
	}
*/
#pragma once
#include <boost/any.hpp>
#include <boost/thread.hpp>
#include <functional>
#include <set>
using namespace std;

class threading
{
	typedef map<wstring, boost::any>	value_map;
	class context
	{
	public:
		value_map				values;
		list<function<void()>>	exit_functions;
		set<HANDLE>				handles;
		~context()
		{
			for_each(exit_functions.begin(), exit_functions.end(), [](function<void()> f) {f();});
			for_each(handles.begin(), handles.end(), CloseHandle);
		}
	};
private:
	static context* get_context()
	{
		static boost::thread_specific_ptr<context> context_ptr;
		if(nullptr == context_ptr.get())
		{
			context_ptr.reset(new context);
		}
		return context_ptr.get();
	}
public:
	//Access named value bound to the current thread.
	template<typename ValueType>
	static ValueType& value(const wstring& key)
	{
		auto& v = get_context()->values[key];
		return *boost::any_cast<ValueType>(&v);
	}
	static boost::any& value(const wstring& key)
	{
		return get_context()->values[key];
	}
	static bool exists(const wstring& key)
	{
		auto context = get_context();
		auto it = context->values.find(key);
		if(context->values.end() == it) return false;
		return false == it->second.empty();
	}
	//Schedule void(*)() typed operation to be called when current thread exiting.
	static void queue_exit_operation(function<void()> f)
	{
		get_context()->exit_functions.push_back(f);
	}
	//Schedule a handle to be close via CloseHandle when current thread exiting.
	static void close_handle_on_existing(HANDLE handle)
	{
		get_context()->handles.insert(handle);
	}
};