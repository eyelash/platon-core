#pragma once

#include "os.hpp"
#include <vector>

class Newlines {
	std::vector<std::size_t> lines;
public:
	template <class I> Newlines(I first, I last) {
		std::size_t index = 0;
		for (; first != last; ++first) {
			++index;
			if (*first == '\n') {
				lines.push_back(index);
				index = 0;
			}
		}
	}
	std::size_t get_total_lines() const {
		return lines.size();
	}
	std::size_t get_index(std::size_t line) const {
		std::size_t index = 0;
		for (std::size_t l = 0; l < line; ++l) {
			index += lines[l];
		}
		return index;
	}
	std::size_t get_line(std::size_t index) const {
		for (std::size_t l = 0; l < lines.size(); ++l) {
			if (lines[l] > index) {
				return l;
			}
			index -= lines[l];
		}
	}
	void insert(std::size_t index) {
		for (std::size_t l = 0; l < lines.size(); ++l) {
			if (lines[l] > index) {
				++lines[l];
				return;
			}
			index -= lines[l];
		}
	}
	void remove(std::size_t index) {
		for (std::size_t l = 0; l < lines.size(); ++l) {
			if (lines[l] > index) {
				--lines[l];
				return;
			}
			index -= lines[l];
		}
	}
};

class Buffer {
	Mmap mmap;
public:
	Buffer(const char* path): mmap(path) {}
	const char* begin() const {
		return mmap.begin();
	}
	const char* end() const {
		return mmap.end();
	}
};
