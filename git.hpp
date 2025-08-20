#pragma once

#include <cstdint>
#include <algorithm>
#include <vector>

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

class BitReader {
	const char* data;
	const char* end;
	int bit_position;
public:
	BitReader(const char* data, const char* end): data(data), end(end), bit_position(0) {}
	BitReader(const std::vector<char>& v): BitReader(v.data(), v.data() + v.size()) {}
	explicit operator bool() const {
		return data != end;
	}
	unsigned char get() const {
		return *data;
	}
	std::size_t get_position(const char* start) const {
		return data - start;
	}
	int read_bit() {
		int result = get() >> bit_position;
		if (++bit_position == 8) {
			++data;
			bit_position = 0;
		}
		return result & 1;
	}
	int read_int(int bits) {
		int result = 0;
		for (int i = 0; i < bits; ++i)
			result |= read_bit() << i;
		return result;
	}
	void skip_to_next_byte() {
		if (bit_position > 0) {
			++data;
			bit_position = 0;
		}
	}
	char read_aligned_byte() {
		char result = *data;
		++data;
		return result;
	}
	template <class T> T read_aligned() {
		T result = 0;
		for (std::size_t i = 0; i < sizeof(T); ++i) {
			result |= get() << ((sizeof(T) - 1 - i) * 8);
			++data;
		}
		return result;
	}
};

class Inflate {
	// https://datatracker.ietf.org/doc/html/rfc1951
	class Tree {
	public:
		class Node {
		public:
			unsigned int value;
			static constexpr unsigned int value_bit = 1u << (sizeof(unsigned int) * 8 - 1);
			void set_right_child(int right_child) {
				this->value = right_child;
			}
			void set_value(int value) {
				this->value = value | value_bit;
			}
		};
		Node nodes[288+287];
		void set_right_child(int index, int right_child) {
			nodes[index].value = right_child;
		}
		void set_value(int index, int value) {
			nodes[index].value = value | Node::value_bit;
		}
		bool has_value(int index) const {
			return nodes[index].value & Node::value_bit;
		}
		int get_value(int index) const {
			return nodes[index].value & (~Node::value_bit);
		}
		int get_left_child(int index) const {
			return index + 1;
		}
		int get_right_child(int index) const {
			return nodes[index].value;
		}
		int decode_value(BitReader& reader) const {
			int index = 0;
			while (!has_value(index)) {
				if (reader.read_bit())
					index = get_right_child(index);
				else
					index = get_left_child(index);
			}
			return get_value(index);
		}
	};
	class TreeBuilder {
		struct Entry {
			int value;
			char bits;
		};
		Entry entries[288];
		int size = 0;
	public:
		void set_bits(int value, char bits) {
			if (bits == 0) return;
			entries[size] = {value, bits};
			++size;
		}
		Tree* tree;
		int index = 0;
		int tree_index = 0;
		static bool compare_entries(Entry lhs, Entry rhs) {
			if (lhs.bits != rhs.bits)
				return lhs.bits < rhs.bits;
			return lhs.value < rhs.value;
		}
		int add_node(char bits) {
			const int current_tree_index = tree_index;
			Tree::Node* node = &tree->nodes[current_tree_index];
			++tree_index;
			if (entries[index].bits == bits) {
				node->set_value(entries[index].value);
				++index;
			}
			else {
				add_node(bits + 1);
				node->set_right_child(add_node(bits + 1));
			}
			return current_tree_index;
		}
		void build_tree(Tree* tree) {
			std::sort(entries, entries + size, compare_entries);
			this->tree = tree;
			add_node(0);
		}
		Tree build_tree() {
			Tree tree;
			build_tree(&tree);
			return tree;
		}
	};
	static void build_fixed_literal_tree(Tree* tree) {
		TreeBuilder tree_builder;
		for (int value = 0; value <= 143; ++value)
			tree_builder.set_bits(value, 8);
		for (int value = 144; value <= 255; ++value)
			tree_builder.set_bits(value, 9);
		for (int value = 256; value <= 279; ++value)
			tree_builder.set_bits(value, 7);
		for (int value = 280; value <= 287; ++value)
			tree_builder.set_bits(value, 8);
		tree_builder.build_tree(tree);
	}
	static void build_fixed_distance_tree(Tree* tree) {
		TreeBuilder tree_builder;
		for (int value = 0; value <= 31; ++value)
			tree_builder.set_bits(value, 5);
		tree_builder.build_tree(tree);
	}
	static void build_length_tree(BitReader& reader, int codes, Tree* tree) {
		TreeBuilder tree_builder;
		static constexpr int values[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
		for (int i = 0; i < codes; ++i) {
			tree_builder.set_bits(values[i], reader.read_int(3));
		}
		tree_builder.build_tree(tree);
	}
	static void build_dynamic_trees(BitReader& reader, const Tree& length_tree, int literal_lengths, int distance_lengths, Tree* literal_tree, Tree* distance_tree) {
		TreeBuilder literal_builder;
		TreeBuilder distance_builder;
		int i = 0;
		int previous_length;
		auto set_bits = [&](char bits) {
			if (i < literal_lengths)
				literal_builder.set_bits(i, bits);
			else
				distance_builder.set_bits(i - literal_lengths, bits);
			++i;
			previous_length = bits;
		};
		while (i < literal_lengths + distance_lengths) {
			const int value = length_tree.decode_value(reader);
			if (value <= 15) {
				set_bits(value);
			}
			else if (value == 16) {
				const int end = i + 3 + reader.read_int(2);
				while (i < end) {
					set_bits(previous_length);
				}
			}
			else if (value == 17) {
				i += 3 + reader.read_int(3);
				previous_length = 0;
			}
			else if (value == 18) {
				i += 11 + reader.read_int(7);
				previous_length = 0;
			}
		}
		literal_builder.build_tree(literal_tree);
		distance_builder.build_tree(distance_tree);
	}
	static int get_length(BitReader& reader, int code) {
		if (code < 265) {
			return code - 257 + 3;
		}
		if (code < 285) {
			code = code - 261;
			const int extra_bits = code / 4;
			return 3 + ((4 + code % 4) << extra_bits) + reader.read_int(extra_bits);
		}
		return 258;
	}
	static int get_distance(BitReader& reader, int code) {
		if (code < 4) {
			return code + 1;
		}
		code = code - 2;
		const int extra_bits = code / 2;
		return 1 + ((2 + code % 2) << extra_bits) + reader.read_int(extra_bits);
	}
public:
	static std::vector<char> inflate(BitReader& reader) {
		std::vector<char> result;
		while (true) {
			const int bfinal = reader.read_bit();
			const int btype = reader.read_int(2);
			if (btype == 0) {
				reader.skip_to_next_byte();
				const int len = reader.read_int(16);
				const int nlen = reader.read_int(16);
				for (int i = 0; reader && i < len; ++i) {
					result.push_back(reader.read_aligned_byte());
				}
			}
			else {
				Tree literal_tree;
				Tree distance_tree;
				if (btype == 1) {
					build_fixed_literal_tree(&literal_tree);
					build_fixed_distance_tree(&distance_tree);
				}
				else if (btype == 2) {
					const int hlit = reader.read_int(5);
					const int hdist = reader.read_int(5);
					const int hclen = reader.read_int(4);
					Tree length_tree;
					build_length_tree(reader, hclen + 4, &length_tree);
					build_dynamic_trees(reader, length_tree, hlit + 257, hdist + 1, &literal_tree, &distance_tree);
				}
				while (true) {
					const int value = literal_tree.decode_value(reader);
					if (value < 256) {
						result.push_back(value);
					}
					else if (value == 256) {
						break;
					}
					else {
						const int length = get_length(reader, value);
						const int distance = get_distance(reader, distance_tree.decode_value(reader));
						for (int i = result.size() - distance, end = i + length; i < end; ++i) {
							result.push_back(result[i]);
						}
					}
				}
			}
			if (bfinal) break;
		}
		return result;
	}
	static std::vector<char> zlib_decompress(BitReader& reader) {
		// https://datatracker.ietf.org/doc/html/rfc1950
		const int cm = reader.read_int(4);
		const int cinfo = reader.read_int(4);
		const int fcheck = reader.read_int(5);
		const int fdict = reader.read_bit();
		const int flevel = reader.read_int(2);
		if (fdict) return {};
		std::vector<char> result = inflate(reader);
		reader.skip_to_next_byte();
		const std::uint32_t adler32_expected = reader.read_aligned<std::uint32_t>();
		Adler32 adler32;
		for (char c: result) adler32 << c;
		if (adler32 != adler32_expected) return {};
		return result;
	}
};

namespace git {

}
