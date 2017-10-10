/*
	Author:	HouSD.
	Date:	2013-03-09
*/

#pragma once
#include <time.h>
#include <boost/random.hpp>
#include <string>
#include <custom/usefultypes.hpp>
#include <vector>

namespace custom
{
	struct uniform_random_generator
	{
	private:
		inline boost::variate_generator<boost::mt19937, boost::uniform_int<uint64_t>>& distribution()
		{
			static boost::mt19937 mt((uint32_t)time(nullptr));
			static boost::variate_generator<decltype(mt), boost::uniform_int<uint64_t>> engine(mt, boost::uniform_int<uint64_t>(0, UINT64_MAX));
			return engine;
		}
	public:
		template<typename Elem>
		inline Elem operator()(Elem min, Elem max)
		{
			static boost::recursive_mutex m;
			DeclareSection(m);
			return (Elem)(distribution()() % (max - min + 1) + min);
		}
	};

	template<typename Elem, typename Iter>
	inline void uniform_rand(Elem min, Elem max, Iter& first, Iter& last)
	{
		auto d = uniform_random_generator();
		for(auto p = first; p != last; ++p)
			*p = d(min, max);
	}
	template<typename Elem>
	inline Elem uniform_rand(Elem min, Elem max)
	{
		return uniform_random_generator()(min, max);
	}
	template<typename Elem>
	inline std::vector<Elem> uniform_rand(const Elem cset[], size_t length)
	{
		int N = 0;
		while(cset[N]) ++N;
		std::vector<Elem> vec(length);
		uniform_rand<Elem>(0, N - 1, vec.begin(), vec.end());
		for(size_t k = 0; k < length; ++k)
			vec[k] = cset[vec[k]];
		return vec;
	}
	inline std::wstring uniform_rand(const wchar_t cset[], size_t length)
	{
		auto vec = uniform_rand<wchar_t>(cset, length);
		return std::wstring(vec.begin(), vec.end());
	}
};