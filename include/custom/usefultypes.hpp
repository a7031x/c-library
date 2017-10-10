/*
	Author:	HouSD
	Date:	2013-02-27

	This library offers several types or functors which may be used by other librarys, though some of they seems useless at the first sight.
*/

#pragma once
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/any.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/date_time.hpp>
#include <memory>
#include <string>

#define	DeclareSection(m)	auto __a7031x_custom_lock_guard = custom::generate_mutex_guard(m)
#define	DeclareSharedSection(m)	boost::shared_lock<boost::shared_mutex> __a7031x_shared_lock_guard(const_cast<boost::shared_mutex&>(m))
#define	DeclareUniqueSection(m)	boost::unique_lock<boost::shared_mutex> __a7031x_shared_lock_guard(const_cast<boost::shared_mutex&>(m))
#define	PostExitGuard(f)	custom::territory_exit_guard __a7031x_custom_exit_guard(f)
namespace custom
{
	struct iless
	{
		template<class StringType>
		bool operator()(const StringType& t1, const StringType& t2)const
		{
			return boost::algorithm::to_upper_copy(t1) < boost::algorithm::to_upper_copy(t2);
		}
	};
	template<typename ValueType> class value_map_t : public std::map<std::wstring, ValueType, custom::iless> {};
	typedef	std::map<std::wstring, boost::any, custom::iless> value_map;
	typedef value_map_t<std::wstring> text_map;
	template<typename Mutex>
	class lock_guard
	{
	private:
		Mutex& m;

	public:
		lock_guard(Mutex& m_) : m(m_)
		{
			m.lock();
		}
		~lock_guard()
		{
			m.unlock();
		}
	};
	template<typename Mutex>
	inline lock_guard<Mutex> generate_mutex_guard(Mutex& m)
	{
		return m;
	}
	template<typename Mutex>
	inline lock_guard<Mutex> generate_mutex_guard(const Mutex& m)
	{
		return const_cast<Mutex&>(m);
	}

	template<typename ReturnType>
	struct wait_wrapper_generic
	{
		boost::timed_mutex m;
		boost::function<ReturnType()> f;
		wait_wrapper_generic(boost::function<ReturnType()> _f) : f(_f) {m.lock();}
		bool wait(uint64_t milliseconds = INT32_MAX)
		{
			if(m.timed_lock(boost::posix_time::milliseconds(milliseconds)))
			{
				m.unlock();
				return true;
			}
			return false;
		}
		bool waiting()const
		{
			if(const_cast<decltype(m)&>(m).try_lock())
			{
				const_cast<decltype(m)&>(m).unlock();
				return false;
			}
			return true;
		}
		void cancel()
		{
			if(waiting())
				const_cast<decltype(m)&>(m).unlock();
		}
	};

	template<typename ReturnType>
	struct wait_wrapper : public wait_wrapper_generic<ReturnType>
	{
		ReturnType result;
		void execute()
		{
			if(waiting())
			{
				result = f();
				m.unlock();
			}
		}
		template<typename CompatibleReturnType>
		void execute(const CompatibleReturnType& result_overriden)
		{
			if(waiting())
			{
				result = result_overriden;
				f();
				m.unlock();
			}
		}
		wait_wrapper(boost::function<ReturnType()> _f) : wait_wrapper_generic<ReturnType>(_f) {}
	};
	template<> struct wait_wrapper<void> : public wait_wrapper_generic<void>
	{
		void execute()
		{
			if(waiting())
			{
				f();
				m.unlock();
			}
		}
		wait_wrapper(boost::function<void()> _f) : wait_wrapper_generic<void>(_f) {}
	};
	//The widget function will post a function of type ReturnType(*)() to the ios queue, and wait for its completion.
	//The parameter f required here MUST NOT throw any exception.
	template<typename ReturnType>
	inline std::shared_ptr<wait_wrapper<ReturnType>> generate_wait_wrapper(boost::asio::io_service& ios, boost::function<ReturnType()> f)
	{
		auto waiter = make_shared<wait_wrapper<ReturnType>>(f);
		auto agent = [waiter]()
		{
			waiter->execute();
		};
		ios.post(agent);
		return waiter;
	}

	class territory_exit_guard : boost::noncopyable
	{
	private:
		std::function<void()> m_task;
	public:
		territory_exit_guard(std::function<void()> task) : m_task(task) {}
		~territory_exit_guard() {m_task();}
	};

	class progress_timer
	{
	private:
		boost::posix_time::ptime	start_time;
	public:
		progress_timer() {reset();}
		void reset() {start_time = now();}
		int64_t milliseconds()const {return milliseconds(start_time);}
		int64_t seconds()const {return seconds(start_time);}
		int64_t minutes()const {return minutes(start_time);}
		static int64_t milliseconds(const boost::posix_time::ptime& start_time) {return (now() - start_time).total_milliseconds();}
		static int64_t seconds(const boost::posix_time::ptime& start_time) {return (now() - start_time).total_seconds();}
		static int64_t minutes(const boost::posix_time::ptime& start_time) {return seconds(start_time) / 60;}
		static boost::posix_time::ptime now() {return boost::posix_time::microsec_clock::local_time();}
	};

	template<typename Type>
	class atomic : boost::noncopyable
	{
	private:
		Type value;
		boost::shared_mutex m;
	public:
		atomic() {}
		atomic(const Type& v) : value(v) {}
		operator Type()const
		{
			DeclareSharedSection(m);
			return value;
		}

#define Operator(op)\
		template<typename OprandType>\
		Type operator op (const OprandType& v)\
		{\
			DeclareUniqueSection(m);\
			value op v;\
			return value;\
		}
		Operator(=)
		Operator(+=)
		Operator(-=)
		Operator(*=)
		Operator(/=)
#undef Operator
	};

	template<template<class, class, class, class> class Map1, template<class, class, class, class> class Map2,
		typename Tk, typename Tv1, typename Tv2, typename Less1, typename Less2, typename A1, typename A2>
		const Map1<Tk, Tv1, Less1, A1>& operator |= (Map1<Tk, Tv1, Less1, A1>& m1, const Map2<Tk, Tv2, Less2, A2>& m2)
	{
		for(auto it = m2.begin(); m2.end() != it; ++it)
		{
			m1[it->first] = it->second;
		}
		return m1;
	}

	template<template<class, class, class, class> class Map1, template<class, class, class, class> class Map2,
		typename Tk, typename Tv1, typename Tv2, typename Less1, typename Less2, typename A1, typename A2>
		const Map1<Tk, Tv1, Less1, A1>& operator += (Map1<Tk, Tv1, Less1, A1>& m1, const Map2<Tk, Tv2, Less2, A2>& m2)
	{
		for(auto it = m2.begin(); m2.end() != it; ++it)
		{
			if(m1.find(it->first) == m1.end())
				m1[it->first] = it->second;
		}
		return m1;
	}

};