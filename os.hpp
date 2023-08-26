#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#endif
#include <cstddef>

constexpr bool is_path_separator(char c) {
	#ifdef _WIN32
	return c == '\\' || c == '/';
	#else
	return c == '/';
	#endif
}

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
	char operator [](std::size_t i) const {
		return map[i];
	}
	const char* begin() const {
		return map;
	}
	const char* end() const {
		return map + size;
	}
};

class Time {
public:
	static double get_monotonic() {
		#ifdef _WIN32
		static const double frequency = []() {
			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency);
			return frequency.QuadPart;
		}();
		LARGE_INTEGER count;
		QueryPerformanceCounter(&count);
		return count.QuadPart / frequency;
		#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return ts.tv_sec + ts.tv_nsec / 1e9;
		#endif
	}
};
