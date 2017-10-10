#pragma once
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK	1
#include <custom/codepage.hpp>
#include <crypto++/cryptlib.h>
#include <crypto++/md5.h>
#include <boost/format.hpp>
#include <array>
using namespace std;
#ifdef _DEBUG
#pragma comment(lib, "cryptlibd.lib")
#else
#pragma comment(lib, "cryptlib.lib")
#endif // DEBUG

namespace custom
{
	inline string md5(const void* data, size_t size)
	{
		using namespace CryptoPP::Weak;
		array<byte, 16> m;
		MD5 md5;
		md5.Update(reinterpret_cast<const byte*>(data), size);
		md5.Final(m.data());
		std::reverse(m.begin(), m.end());
		return (boost::format("%016X%016X") % *reinterpret_cast<uint64_t*>(&m[8]) % *reinterpret_cast<uint64_t*>(&m[0])).str();
	}
	template<typename Container>
	inline string md5(const Container& content)
	{
		return md5(content.data(), content.size() * sizeof(Container::value_type));
	}
}
