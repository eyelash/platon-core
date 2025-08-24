#pragma once

#include "os.hpp"
#include <cstdint>
#include <cstddef>
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
	static std::uint8_t from_hex(char hex) {
		if (hex >= '0' && hex <= '9')
			return hex - '0';
		if (hex >= 'a' && hex <= 'f')
			return 10 + (hex - 'a');
		if (hex >= 'A' && hex <= 'F')
			return 10 + (hex - 'A');
		return 0;
	}
	static std::uint8_t from_hex(const char* s) {
		return from_hex(s[0]) << 4 | from_hex(s[1]);
	}
	static char to_hex(std::uint8_t n) {
		if (n < 10)
			return '0' + n;
		if (n < 16)
			return 'a' + (n - 10);
		return 0;
	}
	static void to_hex(std::uint8_t n, char* s) {
		s[0] = to_hex(n / 16);
		s[1] = to_hex(n % 16);
	}
	static_assert(bits % 8 == 0);
	std::uint8_t data[bits/8];
	Hash() {}
	bool operator ==(const Hash& other) const {
		for (int i = 0; i < bits / 8; ++i) {
			if (data[i] != other.data[i]) return false;
		}
		return true;
	}
	bool operator <(const Hash& other) const {
		for (int i = 0; i < bits / 8; ++i) {
			if (data[i] != other.data[i]) return data[i] < other.data[i];
		}
		return false;
	}
	/*friend std::ostream& operator <<(std::ostream& os, const Hash& h) {
		for (int i = 0; i < bits / 8; ++i) {
			os << to_hex(h.data[i] / 16) << to_hex(h.data[i] % 16);
		}
		return os;
	}*/
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
	SHA1& operator <<(const char* s) {
		for (; *s != '\0'; ++s)
			push_back(*s);
		return *this;
	}
	SHA1& operator <<(std::size_t size) {
		if (size >= 10)
			operator <<(size / 10);
		push_back('0' + size % 10);
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
	BitReader(const Mmap& mmap): BitReader(mmap.begin(), mmap.end()) {}
	BitReader(const Mmap& mmap, std::size_t offset): BitReader(mmap.begin() + offset, mmap.end()) {}
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

template <class T> class IndexIterator {
	const T* t;
	std::size_t index_;
public:
	IndexIterator(const T* t, std::size_t index_): t(t), index_(index_) {}
	using difference_type = std::size_t;
	using value_type = typename T::value_type;
	using pointer = const value_type*;
	using reference = const value_type&;
	using iterator_category = std::random_access_iterator_tag;
	auto operator *() const -> decltype((*t)[index_]) {
		return (*t)[index_];
	}
	auto operator [](difference_type i) const -> decltype((*t)[index_ + i]) {
		return (*t)[index_ + i];
	}
	IndexIterator operator +(difference_type d) const {
		return IndexIterator(t, index_ + d);
	}
	// TODO: differentce_type + IndexIterator
	IndexIterator operator -(difference_type d) const {
		return IndexIterator(t, index_ - d);
	}
	IndexIterator& operator +=(difference_type d) {
		index_ += d;
		return *this;
	}
	IndexIterator& operator -=(difference_type d) {
		index_ -= d;
		return *this;
	}
	IndexIterator& operator ++() {
		++index_;
		return *this;
	}
	IndexIterator& operator --() {
		--index_;
		return *this;
	}
	IndexIterator operator ++(int) {
		IndexIterator result = *this;
		++index_;
		return result;
	}
	IndexIterator operator --(int) {
		IndexIterator result = *this;
		--index_;
		return result;
	}
	difference_type operator -(const IndexIterator& other) const {
		return index_ - other.index_;
	}
	bool operator ==(const IndexIterator& other) const {
		return index_ == other.index_;
	}
	bool operator !=(const IndexIterator& other) const {
		return index_ != other.index_;
	}
	bool operator <(const IndexIterator& other) const {
		return index_ < other.index_;
	}
	bool operator >(const IndexIterator& other) const {
		return index_ > other.index_;
	}
	bool operator <=(const IndexIterator& other) const {
		return index_ <= other.index_;
	}
	bool operator >=(const IndexIterator& other) const {
		return index_ >= other.index_;
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

class Parser {
	const char* data_;
	const char* end_;
public:
	constexpr Parser(): data_(nullptr), end_(nullptr) {}
	constexpr Parser(const char* data_, const char* end_): data_(data_), end_(end_) {}
	Parser(const Mmap& mmap): data_(mmap.begin()), end_(mmap.end()) {}
	Parser(const std::vector<char>& v): data_(v.data()), end_(v.data() + v.size()) {}
	Parser(const char* s): data_(s), end_(s) {
		while (*end_ != '\0') {
			++end_;
		}
	}
	explicit constexpr operator bool() const {
		return data_ != end_;
	}
	template <class T> T to() const {
		return T(data_, end_);
	};
	std::string to_string() const {
		return std::string(data_, end_);
	}
	std::vector<char> to_vector() const {
		return std::vector<char>(data_, end_);
	}
	bool parse(const char* s) {
		const char* d = data_;
		while (*s != '\0') {
			if (d == end_ || *d != *s) {
				return false;
			}
			++d;
			++s;
		}
		data_ = d;
		return true;
	}
	std::size_t parse_size() {
		std::size_t size = 0;
		while (data_ != end_ && *data_ >= '0' && *data_ <= '9') {
			size = size * 10 + (*data_ - '0');
			++data_;
		}
		return size;
	}
	static constexpr bool is_hex_char(char c) {
		return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'a' && c <= 'f');
	}
	template <int bits> bool parse_hash(Hash<bits>& hash) {
		for (int i = 0; i < bits / 8 * 2; ++i) {
			if (data_ + i == end_ || !is_hex_char(data_[i])) {
				return false;
			}
		}
		for (int i = 0; i < bits / 8; ++i) {
			hash.data[i] = Hash<bits>::from_hex(data_);
			data_ += 2;
		}
		return true;
	}
	template <int bits = 160> Hash<bits> parse_hash() {
		Hash<bits> hash;
		parse_hash(hash);
		return hash;
	}
	template <int bits> bool parse_hash_binary(Hash<bits>& hash) {
		if (bits / 8 > end_ - data_) {
			return false;
		}
		for (int i = 0; i < bits / 8; ++i) {
			hash.data[i] = *data_;
			data_ += 1;
		}
		return true;
	}
	Parser parse_until(char delimiter) {
		const char* d = data_;
		while (data_ != end_ && *data_ != delimiter) {
			++data_;
		}
		const char* e = data_;
		if (data_ != end_ && *data_ == delimiter) {
			++data_;
		}
		return Parser(d, e);
	}
	Parser line() {
		return parse_until('\n');
	}
};

enum {
	OBJECT_TYPE_COMMIT = 1,
	OBJECT_TYPE_TREE = 2,
	OBJECT_TYPE_BLOB = 3,
	OBJECT_TYPE_TAG = 4,
	OBJECT_TYPE_OFS_DELTA = 6,
	OBJECT_TYPE_REF_DELTA = 7
};

class Commit {
	Hash<160> tree_;
	std::vector<Hash<160>> parents_;
	std::string message_;
public:
	Commit(Parser parser) {
		while (parser) {
			Parser line = parser.line();
			if (!line) {
				break;
			}
			if (line.parse("tree "))
				tree_ = line.parse_hash();
			else if (line.parse("parent "))
				parents_.push_back(line.parse_hash());
		}
		message_ = parser.to<std::string>();
	}
	const Hash<160>& tree() const {
		return tree_;
	}
	const std::vector<Hash<160>>& parents() const {
		return parents_;
	}
	const std::string& message() const {
		return message_;
	}
};

class Tree {
	struct Entry {
		std::string mode;
		std::string name;
		Hash<160> hash;
		Entry(Parser& parser) {
			mode = parser.parse_until(' ').to<std::string>();
			name = parser.parse_until('\0').to<std::string>();
			parser.parse_hash_binary(hash);
		}
		bool operator <(const char* name) const {
			return this->name < name;
		}
		bool operator <(const Entry& entry) const {
			return name < entry.name;
		}
	};
	std::vector<Entry> entries;
public:
	Tree(Parser parser) {
		while (parser) {
			entries.emplace_back(parser);
		}
		std::sort(entries.begin(), entries.end());
	}
	Hash<160> operator [](const char* name) const {
		auto i = std::lower_bound(entries.begin(), entries.end(), name);
		if (i != entries.end() && i->name == name) {
			return i->hash;
		}
		return {};
	}
	/*friend std::ostream& operator <<(std::ostream& os, const Tree& tree) {
		for (const Entry& entry: tree.entries) {
			os << entry.hash << " " << entry.name << std::endl;
		}
		return os;
	}*/
};

class Object {
	std::uint8_t type_;
	std::vector<char> data_;
public:
	Object(): type_(0) {}
	Object(std::uint8_t type_): type_(type_) {}
	Object(std::uint8_t type_, std::vector<char>&& data_): type_(type_), data_(std::move(data_)) {}
	Object(const std::vector<char>& data_) {
		Parser parser(data_);
		Parser header = parser.parse_until('\0');
		if (header.parse("commit "))
			type_ = 1;
		else if (header.parse("tree "))
			type_ = 2;
		else if (header.parse("blob "))
			type_ = 3;
		else
			type_ = 0;
		this->data_ = parser.to<std::vector<char>>();
	}
	void push_back(char d) {
		data_.push_back(d);
	}
	char operator [](std::size_t i) const {
		return data_[i];
	}
	const char* data() const {
		return data_.data();
	}
	std::size_t size() const {
		return data_.size();
	}
	std::uint8_t type() const {
		return type_;
	}
	explicit operator bool() const {
		return type_ != 0;
	}
	bool is_commit() const {
		return type_ == 1;
	}
	bool is_tree() const {
		return type_ == 2;
	}
	bool is_blob() const {
		return type_ == 3;
	}
	Commit get_commit() const {
		return Commit(Parser(data_));
	}
	Tree get_tree() const {
		return Tree(Parser(data_));
	}
	const std::vector<char>& get_blob() const {
		return data_;
	}
	Hash<160> hash() const {
		SHA1 hasher;
		if (is_commit())
			hasher << "commit ";
		else if (is_tree())
			hasher << "tree ";
		else if (is_blob())
			hasher << "blob ";
		hasher << size() << '\0';
		for (char c: data_)
			hasher << c;
		return hasher.finish();
	}
};

class PackfileIndex {
	Mmap data;
	static std::uint8_t read_byte(const char* s) {
		return *s;
	}
	static std::uint32_t read_word(const char* s) {
		return read_byte(s) << 24 | read_byte(s+1) << 16 | read_byte(s+2) << 8 | read_byte(s+3);
	}
	class Fanout {
		const char* data_;
	public:
		Fanout(const char* data_): data_(data_) {}
		using value_type = std::uint32_t;
		std::size_t size() const {
			return 256;
		}
		std::uint32_t operator [](std::size_t i) const {
			return read_word(data_ + i * 4);
		}
		IndexIterator<Fanout> begin() const {
			return IndexIterator<Fanout>(this, 0);
		}
		IndexIterator<Fanout> end() const {
			return IndexIterator<Fanout>(this, 256);
		}
	};
	class Entry {
		const char* hash_;
		const char* offset_;
	public:
		Entry(const char* hash_, const char* offset_): hash_(hash_), offset_(offset_) {}
		Hash<160> hash() const {
			Hash<160> result;
			for (std::size_t i = 0; i < 20; ++i)
				result.data[i] = read_byte(hash_ + i);
			return result;
		}
		std::uint32_t offset() const {
			return read_word(offset_);
		}
		bool operator <(const Hash<160>& other) const {
			return hash() < other;
		}
	};
	class Entries {
		const char* hashes;
		std::size_t hash_stride;
		const char* offsets;
		std::size_t offset_stride;
		std::size_t size_;
	public:
		Entries(const char* hashes, std::size_t hash_stride, const char* offsets, std::size_t offset_stride, std::size_t size_): hashes(hashes), hash_stride(hash_stride), offsets(offsets), offset_stride(offset_stride), size_(size_) {}
		using value_type = Entry;
		std::size_t size() const {
			return size_;
		}
		Entry operator [](std::size_t i) const {
			return Entry(hashes + i * hash_stride, offsets + i * offset_stride);
		}
		IndexIterator<Entries> begin() const {
			return IndexIterator<Entries>(this, 0);
		}
		IndexIterator<Entries> end() const {
			return IndexIterator<Entries>(this, size_);
		}
	};
	bool is_v2() const {
		Parser magic(data.data(), data.data() + 4);
		const std::uint32_t version = read_word(data.data() + 4);
		return magic.parse("\377tOc") && version == 2;
	}
	Fanout fanout() const {
		if (is_v2())
			return Fanout(data.data() + 8);
		else
			return Fanout(data.data());
	}
	Entries entries(const Fanout& fanout) const {
		std::size_t size = fanout[255];
		if (is_v2()) {
			const char* hashes = data.data() + 8 + 256 * 4;
			const char* offsets = hashes + 20 * size + 4 * size;
			return Entries(hashes, 20, offsets, 4, size);
		}
		else {
			const char* offsets = data.data() + 256 * 4;
			const char* hashes = offsets + 4;
			std::size_t stride = 4 + 20;
			return Entries(hashes, 24, offsets, 24, size);
		}
	}
public:
	PackfileIndex(const Path& path): data(path) {}
	explicit operator bool() const {
		return static_cast<bool>(data);
	}
	std::uint32_t find_object(const Hash<160>& hash) const {
		Fanout fanout = this->fanout();
		Entries entries = this->entries(fanout);
		const std::uint8_t fanout_index = hash.data[0];
		const std::uint32_t lower = fanout_index > 0 ? fanout[fanout_index - 1] : 0;
		const std::uint32_t upper = fanout[fanout_index];
		auto i = std::lower_bound(entries.begin() + lower, entries.begin() + upper, hash);
		if (i != entries.end() && (*i).hash() == hash) {
			return (*i).offset();
		}
		return 0;
	}
	/*void print() const {
		for (auto entry: entries(fanout())) {
			std::cout << entry.hash() << "  " << entry.offset() << std::endl;
		}
	}*/
};

class Packfile {
	Mmap pack;
	static std::uint32_t read_size(BitReader& reader, std::uint8_t& type) {
		std::uint8_t byte = reader.read_aligned_byte();
		type = (byte >> 4) & 0x07;
		std::uint32_t size = byte & 0x0F;
		int pos = 4;
		while (byte & 0x80) {
			byte = reader.read_aligned_byte();
			size |= (byte & 0x7F) << pos;
			pos += 7;
		}
		return size;
	}
	static std::uint32_t read_size(BitReader& reader) {
		std::uint8_t byte = reader.read_aligned_byte();
		std::uint32_t size = byte & 0x7F;
		int pos = 7;
		while (byte & 0x80) {
			byte = reader.read_aligned_byte();
			size |= (byte & 0x7F) << pos;
			pos += 7;
		}
		return size;
	}
	static std::uint32_t read_offset(BitReader& reader) {
		std::uint8_t byte = reader.read_aligned_byte();
		std::uint32_t offset = byte & 0x7F;
		while (byte & 0x80) {
			byte = reader.read_aligned_byte();
			offset = ((offset + 1) << 7) | (byte & 0x7F);
		}
		return offset;
	}
public:
	Packfile(const Path& path): pack(path) {}
	Object read_object(BitReader& reader) const {
		const std::uint32_t position = reader.get_position(pack.data());
		std::uint8_t type;
		std::uint32_t size = read_size(reader, type);
		if (type >= 1 && type <= 4) {
			return Object(type, Inflate::zlib_decompress(reader));
		}
		else if (type == 6) { // offset delta
			std::uint32_t offset = read_offset(reader);
			Object base = read_object(position - offset);
			auto delta = Inflate::zlib_decompress(reader);
			BitReader delta_reader(delta);
			std::uint32_t base_size = read_size(delta_reader);
			std::uint32_t size = read_size(delta_reader);
			Object object(base.type());
			while (delta_reader) {
				const std::uint8_t instruction = delta_reader.read_aligned_byte();
				if (instruction & 0x80) { // copy from base
					std::uint8_t arguments[7];
					for (int i = 0; i < 7; ++i) {
						if (instruction & (1 << i))
							arguments[i] = delta_reader.read_aligned_byte();
						else
							arguments[i] = 0;
					}
					std::uint32_t copy_offset = arguments[0] | arguments[1] << 8 | arguments[2] << 16 | arguments[3] << 24;
					std::uint32_t copy_size = arguments[4] | arguments[5] << 8 | arguments[6] << 16;
					if (copy_size == 0) copy_size = 0x10000;
					for (std::uint32_t i = copy_offset; i < copy_offset + copy_size; ++i) {
						object.push_back(base[i]);
					}
				}
				else { // add new data
					for (std::uint8_t i = 0; i < instruction; ++i) {
						object.push_back(delta_reader.read_aligned_byte());
					}
				}
			}
			return object;
		}
		else {
			// ref delta or invalid
			return Object();
		}
	}
	Object read_object(std::uint32_t offset) const {
		BitReader reader(pack, offset);
		return read_object(reader);
	}
};

class Repository {
	Path path;
	Path get_dir() const {
		Path p = path / ".git";
		while (true) {
			const Path::Type type = p.type();
			if (type == Path::Type::DIRECTORY) {
				return p;
			}
			if (type == Path::Type::REGULAR) {
				Mmap mmap(p);
				Parser parser(mmap);
				if (parser.parse("gitdir: ")) {
					p = path / parser.line().to<std::string>();
					continue;
				}
				return Path();
			}
			else {
				return Path();
			}
		}
	}
public:
	Repository(const char* path): path(path) {}
	Hash<160> head() const {
		Path root = get_dir();
		Path p = root / "HEAD";
		while (true) {
			Mmap mmap(p);
			Parser parser(mmap);
			if (parser.parse("ref: ")) {
				p = root / parser.line().to<std::string>();
				continue;
			}
			return parser.parse_hash();
		}
	}
	Object find_object(const Hash<160>& hash, const Path& root) const {
		char hash_head[3];
		Hash<160>::to_hex(hash.data[0], hash_head);
		hash_head[2] = '\0';
		char hash_tail[39];
		for (int i = 1; i < 160 / 8; ++i)
			Hash<160>::to_hex(hash.data[i], hash_tail + i * 2 - 2);
		hash_tail[38] = '\0';
		Mmap mmap(root / "objects" / hash_head / hash_tail);
		if (mmap) {
			BitReader reader(mmap);
			return Object(Inflate::zlib_decompress(reader));
		}
		Path pack_dir = root / "objects" / "pack";
		for (auto pack: pack_dir.children()) {
			Path pack_path = pack_dir / pack;
			if (pack_path.extension() == "pack") {
				Path index_path = pack_path.with_extension("idx");
				PackfileIndex packfile_index(index_path);
				if (packfile_index) {
					if (std::uint32_t offset = packfile_index.find_object(hash)) {
						Packfile packfile(pack_path);
						return packfile.read_object(offset);
					}
				}
			}
		}
		return Object();
	}
	Object find_object(const Hash<160>& hash) const {
		const Path root = get_dir();
		if (!root) {
			return Object();
		}
		return find_object(hash, root);
	}
	Object find_object(const char* rev) const {
		const Path root = get_dir();
		if (!root) {
			return Object();
		}
		Parser parser(rev);
		Hash<160> hash;
		if (parser.parse_hash(hash)) {
			if (Object object = find_object(hash, root)) {
				return object;
			}
		}
		Path paths[] = {
			root / rev,
			root / "refs" / rev,
			root / "refs" / "tags" / rev,
			root / "refs" / "heads" / rev,
			root / "refs" / "remotes" / rev,
			root / "refs" / "remotes" / rev / "HEAD"
		};
		for (const Path& p: paths) {
			Mmap mmap(p);
			if (!mmap) {
				continue;
			}
			while (true) {
				Parser parser(mmap);
				if (parser.parse("ref: ")) {
					mmap = Mmap(root / parser.line().to<std::string>());
					if (!mmap) {
						return Object();
					}
					continue;
				}
				Hash<160> hash;
				if (!parser.parse_hash(hash)) {
					return Object();
				}
				return find_object(hash, root);
			}
		}
		return Object();
	}
};

}
