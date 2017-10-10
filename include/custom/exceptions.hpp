/*
	Author:	HouSD
	Date:	2013/02/27
*/
#pragma once

#include <boost/exception/all.hpp>
#include <string>
using namespace std;

#define	DEFINE_ERROR_INFO(type, name)	typedef boost::error_info<struct tag_##__FILE__##__LINE__##name, type> name

//DEFINE_ERROR_INFO(string, error_text);
DEFINE_ERROR_INFO(wstring, error_wtext);
struct exception2 : virtual std::exception, virtual boost::exception {};

inline string display_error(const exception2& e)
{
	return diagnostic_information(e);
}
/*
inline void commit_error(const string& text)
{
	throw exception2() << error_text(text);
}
*/
#define	commit_error(text)	throw exception2() << error_wtext(text)
/*inline void commit_error(const wstring& text)
{
	throw exception2() << error_wtext(text);
}*/

inline wstring dump_error(const exception2& e)
{
	return *boost::get_error_info<error_wtext>(e);
}