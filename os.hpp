#pragma once

#ifdef _WIN32
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif
#include <cstddef>

class Mmap {
	std::size_t size;
	char* map;
public:
	Mmap(const char* path) {
		#ifdef _WIN32
		#else
		const int fd = open(path, O_RDONLY);
		struct stat stat;
		fstat(fd, &stat);
		size = stat.st_size;
		map = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
		close(fd);
		#endif
	}
	Mmap(const Mmap&) = delete;
	~Mmap() {
		#ifdef _WIN32
		#else
		munmap(map, size);
		#endif
	}
	Mmap& operator =(const Mmap&) = delete;
	std::size_t get_size() const {
		return size;
	}
	char operator[](std::size_t i) const {
		return map[i];
	}
	const char* begin() const {
		return map;
	}
	const char* end() const {
		return map + size;
	}
};
