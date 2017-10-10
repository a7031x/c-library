#pragma once

#include <vector>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <thread>

namespace custom
{
	template<typename StringType>
	inline std::vector<StringType> seperate(const StringType& text, const StringType& seperator)
	{
		using namespace boost::algorithm;
		std::vector<StringType> results;
		for(auto it = make_split_iterator(text, first_finder(seperator, is_equal())); split_iterator<StringType::const_iterator>() != it; ++it)
		{
			if(it->empty()) continue;
			results.push_back(StringType(it->begin(), it->end()));
		}
		return results;
	}
	inline void sleep(uint64_t milliseconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}
}