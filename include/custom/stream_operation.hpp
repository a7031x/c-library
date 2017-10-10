#pragma once
#include <iostream>
#include <vector>

namespace custom
{
	//hera requires iostream other than istream, because I supposed that the stream has been writen and not yet been read.
	template<typename Elem>
	inline void stream_to_vector(std::iostream& s, std::vector<Elem>& v)
	{
		v.resize((size_t)s.tellp() / sizeof(Elem));
		if(0 == v.size()) return;
		s.read(reinterpret_cast<char*>(&v[0]), v.size() * sizeof(Elem));
	}
	template<typename Elem>
	inline void vector_to_stream(const std::vector<Elem>& v, std::ostream& s)
	{
		if(0 == v.size()) return;
		s.write(reinterpret_cast<const char*>(&v[0]), v.size() * sizeof(Elem));
	}
};