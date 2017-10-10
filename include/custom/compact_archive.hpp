///////////////////////////////////////////////////////////////////////////////////////
//compact_archive
//supported: scalar types, containers (string and wstring are special types of container), pair, tuple and custom types.
//			vector and array of POD (plain of data) is optimized.
//unsupported: type check or integrity check. DO NOT try to deserialize to a different type. please keep (de)serialization in order. 
//warning: DO NOT pass pointer type parameter to the compact_archive operators.
//author: Hou S.D.
//date: 12-01-2012
///////////////////////////////////////////////////////////////////////////////////////
//example:
/*
struct CustomType
{
string text;
int data;

CustomType(string text)
{
this->text = text;
this->data = rand();
}

// the method below need to be explicitly defined.
// Otherwise the compiler doesn't know how to serialize the custom type.
template<typename Archive>
void serialize(Archive& ar)
{
ar & text & data;	//list the members you want to serialize and deserialize.
//any member of custom type needs to define the method in its type too.
//IMPORTANT: if the struct type is POD type (e.g. RECT, POINT, SIZE), the serialize method is not required.
//for what's POD type, please refer to MSDN.
}
};

main()
{
using namespace boost::assign;	//use the assign library to show the test example.

int i1[5] = {1, 2, 3, 4, 5};
int i2[5] = {0};			//make sure that i2 is the same length as i1

vector<string> vs1 = list_of("abc")("def")("ghi");
vector<string> vs2;			//output value of container type do not need to by initialized

map<string, CustomType> mct1 = map_list_of("a", CustomType("c-a")) ("b", CustomType("c-b")), mct2;

auto ar = make_compact_archive();	//if a stream is already exists, call the make_compact_archive(stream) instead.

ar << i1 << vs1 << mct1;
ar >> i2 >> vs2 >> mct2;
//l2, vs2, mct2 should be equals to i1, vs1, mct1 relatively.
}
*/

#pragma once
#include <memory>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <tuple>
#include <algorithm>
#include <functional>
#include <stdint.h>
#include <custom/stream_operation.hpp>
using namespace std;

#define	IF_EXISTS(cond, expr)				__if_exists(cond) {expr;}
#define	IF_EXISTS_THEN(cond, expr1, expr2)	__if_exists(cond) {expr1;} __if_not_exists(cond) {expr2;}
#define	DECLARE_SERIALIZE(...)	template<typename ArType> void serialize(ArType& ar) { deserialize_parameters(ar, __VA_ARGS__);}\
								template<typename ArType> void serialize(ArType& ar)const { serialize_parameters(ar, __VA_ARGS__);}

template<typename Type>
struct compact_serializer
{
private:
	compact_serializer();
};

template<typename Stream>
class compact_archive
{
public:
	compact_archive() : m_default_stream(make_shared<stringstream>()), m_stream(*m_default_stream) {}
	compact_archive(Stream& s) : m_stream(s) {}
	template<typename Type>
	//auto serialization or deserialization.
	//if Stream is derived type of iostream (which could be istream or ostream), a compiling error will be exerted.
	compact_archive& operator & (Type& a)
	{
		serialize_all<Stream, Type>()(m_stream, a);
		return *this;
	}
	//explicitly serialization
	template<typename Type>
	compact_archive& operator << (const Type& a)
	{
		serialize_all<ostream, const Type>()(m_stream, a);
		return *this;
	}
	//explicitly deserialization
	template<typename Type>
	compact_archive& operator >> (Type& a)
	{
		serialize_all<istream, Type>()(m_stream, a);
		return *this;
	}
	//it is not recommended to use the methods below, which is designed for compatible with unreasonable defined types.
	template<typename Type>
	void serialize(Type* objects, int number)
	{
		serialize((ostream&)m_stream, objects, number);
	}
	template<typename Type>
	void deserialize(Type* objects, int number)
	{
		serialize((istream&)m_stream, objects, number);
	}
private:
	shared_ptr<stringstream> m_default_stream;
	Stream& m_stream;

private:
	static void serialize_buffer(ostream& s, const void* buffer, streamsize size) { s.write(reinterpret_cast<const char*>(buffer), size); }
	static void serialize_buffer(istream& s, void* buffer, streamsize size) { s.read(reinterpret_cast<char*>(buffer), size); }
	template<typename Type> static Type& reference(Type& a) { return a; }
	template<typename Type> static Type& reference(Type* a) { return *a; }

	template<typename StreamType, typename Type>
	static void serialize(StreamType& s, Type* p, int n)//serialize array
	{
		if (is_pod<Type>::value)
			serialize_buffer(s, p, sizeof(Type) * n);
		else
			std::for_each(&p[0], &p[n], std::bind(serialize_all<StreamType, Type>(), std::ref(s), std::placeholders::_1));

	}
	template<typename StreamType, typename IteratorType>
	static void serialize(StreamType& s, IteratorType itr, int n)//serialize iterators
	{
		for (int k = 0; k < n; ++k)
		{
			serialize(s, &*itr, 1);
			++itr;
		}
	}

	template<typename StreamType, typename Type>
	struct serialize_all
	{
		typedef void result_type;
		void operator()(StreamType& s, Type& a)
		{
			conditional<is_pod<Type>::value,
				serialize_pod<StreamType, Type>,
				conditional<is_container<remove_const<Type>::type>::value,
				serialize_container<remove_const<Type>::type>,
				conditional<is_pair<remove_const<Type>::type>::value,
				serialize_pair<StreamType, remove_const<Type>::type>,
				conditional<is_tuple<remove_const<Type>::type>::value,
				serialize_tuple<StreamType, Type>,
				serialize_complex<StreamType, Type>
				>::type
				>::type
				>::type
			>::type()(s, a);
		}
	};

	template<typename StreamType, typename Type>
	struct serialize_pod
	{
		void operator()(StreamType& s, Type& a)
		{
			serialize_buffer(s, &a, sizeof(a));
		}
	};

	template<typename Type>
	struct serialize_container
	{
		void operator()(ostream& s, const Type& a)
		{
			int32_t size = (int32_t)a.size();
			serialize(s, &size, 1);
			serialize(s, a.begin(), size);
		}
		void operator()(istream& s, Type& a)
		{
			int32_t size;
			serialize(s, &size, 1);
			__if_not_exists(Type::assign)
			{
				while (size--)
				{
					typedef remove_const<Type::iterator::value_type>::type elem_type;
					elem_type elem;
					serialize_all<istream, elem_type>()(s, elem);
					IF_EXISTS_THEN(Type::push_back, a.push_back(elem), a.insert(elem));
				}
			}
			__if_exists(Type::assign)
			{
				a.assign(size, Type::value_type());
				serialize(s, a.begin(), size);
			}
		}
	};
	template<typename Type>
	struct serialize_container<vector<Type>>	//optimization for std::vector
	{
		void operator()(ostream& s, const vector<Type>& a)
		{
			int32_t size = (int32_t)a.size();
			serialize(s, &size, 1);
			if (size)
				serialize(s, &a[0], size);
		}
		void operator()(istream& s, vector<Type>& a)
		{
			int32_t size;
			serialize(s, &size, 1);
			a.assign(size, Type());
			if (0 < size)
				serialize(s, &a[0], size);
		}
	};

	template<typename StreamType, typename Type>
	struct serialize_complex
	{
		void operator()(StreamType& s, Type& a)
		{
			a.serialize(compact_archive<StreamType>(s));
		}
	};
	template<typename StreamType, typename Type>
	struct serialize_pair
	{
		void operator()(ostream& s, const Type& a)
		{
			serialize_all<ostream, const Type::first_type>()(s, a.first);
			serialize_all<ostream, const Type::second_type>()(s, a.second);
		}
		void operator()(istream& s, Type& a)
		{
			typedef remove_const<Type::first_type>::type first_type;
			serialize_all<istream, first_type>()(s, const_cast<first_type&>(a.first));
			serialize_all<istream, Type::second_type>()(s, a.second);
		}
	};
	template<typename StreamType, typename Type>
	struct serialize_tuple
	{
		void operator()(StreamType& s, Type& a)
		{
			serialize_element<0>(s, a);
		}
		template<int Index>
		void serialize_element(StreamType& s, Type& a)
		{
			serialize_all<StreamType, std::remove_reference<decltype(get<Index>(a))>::type>()(s, get<Index>(a));
			serialize_element<Index + 1>(s, a);
		}
		template<>
		void serialize_element<tuple_size<Type>::value>(StreamType& s, Type& a) {}
	};
	template<typename Type> struct is_pair { enum { value = false }; };
	template<typename T1, typename T2> struct is_pair<pair<T1, T2>> { enum { value = true }; };
	template<typename Type> struct is_container { IF_EXISTS_THEN(Type::iterator, enum { value = true }, enum { value = false }); };
#if _MSC_VER < 1700 || _MSC_VER >= 1900
	template<typename Type> struct is_tuple { IF_EXISTS_THEN(Type::tuple, enum { value = true }, enum { value = false }); };
#else
	template<typename Type> struct is_tuple { enum { value = (0 != tuple_size<Type>::value) }; };
#endif

};

template<typename Stream>
inline compact_archive<Stream> make_compact_archive(Stream& s)
{
	return compact_archive<Stream>(s);
}
inline compact_archive<stringstream> make_compact_archive()
{
	return compact_archive<stringstream>();
}

template<typename Type>
inline vector<char> serialize_chunk(const Type& type)
{
	stringstream ss;
	auto ar = make_compact_archive(ss);
	ar << type;
	vector<char> vec;
	custom::stream_to_vector(ss, vec);
	return vec;
}

template<typename Type>
inline Type deserialize_chunk(const vector<char>& chunk)
{
	stringstream ss;
	custom::vector_to_stream(chunk, ss);
	auto ar = make_compact_archive(ss);
	Type type;
	ar >> type;
	return type;
}

template<typename ArType, typename Head, typename... Params>
inline void serialize_parameters(ArType& ar, const Head& head, const Params&... params)
{
	ar << head;
	serialize_parameters(ar, params...);
}

template<typename ArType>
inline void serialize_parameters(ArType& ar)
{
}

template<typename ArType, typename Head, typename... Params>
inline void deserialize_parameters(ArType& ar, Head& head, Params&... params)
{
	ar >> head;
	deserialize_parameters(ar, params...);
}

template<typename ArType>
inline void deserialize_parameters(ArType& ar)
{
}

