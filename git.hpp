#pragma once

#include <cstdint>

class Adler32 {
public:
	std::uint32_t s1 = 1;
	std::uint32_t s2 = 0;
	void push_back(std::uint8_t c) {
		s1 = (s1 + c) % 65521;
		s2 = (s2 + s1) % 65521;
	}
	Adler32& operator <<(char c) {
		push_back(c);
		return *this;
	}
	operator std::uint32_t() const {
		return s2 << 16 | s1;
	}
};

template <int bits> class Hash {
public:
	static_assert(bits % 8 == 0);
	std::uint8_t data[bits/8];
	Hash() {}
};

class SHA1 {
	// https://datatracker.ietf.org/doc/html/rfc3174
	using Word = std::uint32_t;
	std::uint8_t block[64];
	std::uint64_t size = 0;
	Word H0 = 0x67452301;
	Word H1 = 0xEFCDAB89;
	Word H2 = 0x98BADCFE;
	Word H3 = 0x10325476;
	Word H4 = 0xC3D2E1F0;
	template <int n> static constexpr Word S(Word X) {
		return (X << n) | (X >> (32-n));
	}
	void process_block() {
		Word W[80];
		for (int t = 0; t < 16; ++t) {
			std::uint8_t* b = block + t * 4;
			W[t] = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
		}
		for (int t = 16; t < 80; ++t) {
			W[t] = S<1>(W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
		}
		Word A = H0, B = H1, C = H2, D = H3, E = H4;
		for (int t = 0; t < 20; ++t) {
			const Word f = (B & C) | ((~B) & D);
			const Word K = 0x5A827999;
			Word TEMP = S<5>(A) + f + E + W[t] + K;
			E = D; D = C; C = S<30>(B); B = A; A = TEMP;
		}
		for (int t = 20; t < 40; ++t) {
			const Word f = B ^ C ^ D;
			const Word K = 0x6ED9EBA1;
			Word TEMP = S<5>(A) + f + E + W[t] + K;
			E = D; D = C; C = S<30>(B); B = A; A = TEMP;
		}
		for (int t = 40; t < 60; ++t) {
			const Word f = (B & C) | (B & D) | (C & D);
			const Word K = 0x8F1BBCDC;
			Word TEMP = S<5>(A) + f + E + W[t] + K;
			E = D; D = C; C = S<30>(B); B = A; A = TEMP;
		}
		for (int t = 60; t < 80; ++t) {
			const Word f = B ^ C ^ D;
			const Word K = 0xCA62C1D6;
			Word TEMP = S<5>(A) + f + E + W[t] + K;
			E = D; D = C; C = S<30>(B); B = A; A = TEMP;
		}
		H0 = H0 + A, H1 = H1 + B, H2 = H2 + C, H3 = H3 + D, H4 = H4 + E;
	}
	template <int i> static void write_word(Hash<160>& h, Word w) {
		h.data[i*4+0] = w >> 24;
		h.data[i*4+1] = w >> 16;
		h.data[i*4+2] = w >> 8;
		h.data[i*4+3] = w;
	}
public:
	void push_back(std::uint8_t c) {
		block[size % 64] = c;
		++size;
		if (size % 64 == 0) {
			process_block();
		}
	}
	SHA1& operator <<(char c) {
		push_back(c);
		return *this;
	}
	Hash<160> finish() {
		const std::uint64_t bit_size = size * 8;
		push_back(0x80);
		while (size % 64 != 56)
			push_back(0);
		for (int i = 0; i < 8; ++i)
			push_back(bit_size >> ((7-i) * 8));
		Hash<160> hash;
		write_word<0>(hash, H0);
		write_word<1>(hash, H1);
		write_word<2>(hash, H2);
		write_word<3>(hash, H3);
		write_word<4>(hash, H4);
		return hash;
	}
};

namespace git {

}
