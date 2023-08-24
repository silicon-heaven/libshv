#include "crypt.h"

#include "../log.h"

#include <array>
#include <chrono>
#include <regex>

namespace shv::core::utils {

//===================================================================
//                                         Crypt
//===================================================================
/// http://www.math.utah.edu/~pa/Random/Random.html
Crypt::Crypt(Crypt::Generator gen)
	: m_generator(gen)
{
	if(m_generator == nullptr)
		m_generator = Crypt::createGenerator(16811, 7, 2147483647);
}

Crypt::Generator Crypt::createGenerator(uint32_t a, uint32_t b, uint32_t max_rand)
{
	auto ret = [a, b, max_rand](uint32_t val) -> uint32_t {
		uint64_t val_64bit = val;
		val_64bit *= a;
		val_64bit += b;
		val_64bit %= max_rand;
		return static_cast<uint32_t>(val_64bit);
	};
	return ret;
}

void Crypt::setGenerator(Generator gen)
{
	m_generator = gen;
}

static std::string code_byte(uint8_t b)
{
	std::string ret;
	std::array<char, 3> buff{0,0,0};
	if((b>='A' && b<='Z') || (b>='a' && b<='z')) {
		ret.push_back(static_cast<char>(b));
	}
	else {
		uint8_t b1 = b%10;
		b /= 10;
		buff[0] = static_cast<char>(b1 + '0');
		buff[1] = static_cast<char>((b1 % 2)? b + 'A': b + 'a');
		ret.append(buff.data());
	}
	return ret;
}

std::string Crypt::encrypt(const std::string &data, size_t min_length) const
{
	std::string dest;

	/// take random number and store it in first byte
	uint8_t b = 0;
	{
		using namespace std::chrono;
		nanoseconds ns = duration_cast< nanoseconds >( system_clock::now().time_since_epoch() );
		b = static_cast<uint8_t>(ns.count());
		if(b == 0)
			b++;

	}
	uint32_t seed = b;
	dest += code_byte(b);

	/// a tou se to zaxoruje
	for(char i : data) {
		b = (static_cast<uint8_t>(i));
		if(b == 0)
			break;
		seed = m_generator(seed);
		b = b ^ static_cast<uint8_t>(seed);
		dest += code_byte(b);
	}
	uint8_t bb = 0;
	while(dest.size() < min_length) {
		seed = m_generator(seed);
		b = bb ^ static_cast<uint8_t>(seed);
		dest += code_byte(b);
		bb = static_cast<uint8_t>(std::rand());
	}
	return dest;
}

static uint8_t take_byte(const std::string &ba, size_t &i)
{
	uint8_t b = (static_cast<uint8_t>(ba[i++]));
	if((b>='A' && b<='Z') || (b>='a' && b<='z')) {
	}
	else {
		uint8_t b1 = b;
		b1 = b1 - '0';
		if(i < ba.size()) {
			b = static_cast<uint8_t>(ba[i++]);
			b = (b1 % 2)? (b - 'A'): (b - 'a');
			b = static_cast<uint8_t>(10 * b + b1);
		}
		else {
			shvError() << __FUNCTION__ << ": byte array corupted:" << ba;
		}
	}
	return b;
}

std::string Crypt::decodeArray(const std::string &ba) const
{
	/// vyuziva toho, ze generator nahodnych cisel generuje pokazde stejnou sekvenci
	/// precte si seed ze zacatku \a ba a pak odxorovava nahodnymi cisly, jen to svisti
	shvLogFuncFrame() << "decoding:" << ba;
	std::string ret;
	if(ba.empty())
		return ret;
	size_t i = 0;
	uint32_t seed = take_byte(ba, i);
	while(i<ba.length()) {
		seed = m_generator(seed);
		uint8_t b = take_byte(ba, i);
		b = b ^ static_cast<uint8_t>(seed);
		ret.push_back(static_cast<char>(b));
	}
	return ret;
}

std::string Crypt::decrypt(const std::string &data) const
{
	/// odstran vsechny bile znaky, v zakodovanem textu nemohou byt, muzou to byt ale zalomeni radku
	std::string ba = std::regex_replace(data, std::regex("\\s+"), "");
	ba = decodeArray(ba);
	///odstran \0 na konci, byly tam asi umele pridany
	size_t pos = 0;
	while(pos < ba.size()) {
		if(ba[pos] == '\0')
			break;
		pos++;
	}
	ba = ba.substr(0, pos);
	return ba;
}

}
