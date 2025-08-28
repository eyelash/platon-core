#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <time.h>
#endif
#include <cstddef>
#include <string>
#include <algorithm>

class Path {
	std::string path;
public:
	static constexpr char separator = '/';
	static constexpr bool is_separator(char c) {
		#ifdef _WIN32
		return c == '\\' || c == '/';
		#else
		return c == '/';
		#endif
	}
	enum class Type {
		NOT_FOUND,
		REGULAR,
		DIRECTORY
	};
	Path(std::string&& path): path(std::move(path)) {}
	Path(const char* path): path(path) {}
	Path(): path() {}
	static Path cwd() {
		#ifdef _WIN32
		#else
		char* cwd = getcwd(NULL, 0);
		Path path(cwd);
		free(cwd);
		return path;
		#endif
	}
	const char* c_str() const {
		return path.c_str();
	}
	explicit operator bool() const {
		return path.size() != 0;
	}
	Type type() const {
		#ifdef _WIN32
		#else
		struct stat s;
		if (stat(path.c_str(), &s) == 0) {
			if (S_ISREG(s.st_mode)) return Type::REGULAR;
			if (S_ISDIR(s.st_mode)) return Type::DIRECTORY;
		}
		return Type::NOT_FOUND;
		#endif
	}
	Path canonical() const {
		#ifdef _WIN32
		// GetFinalPathNameByHandle
		#else
		char* canonical = realpath(path.c_str(), nullptr);
		Path path(canonical);
		free(canonical);
		return path;
		#endif
	}
	std::string filename() const {
		for (std::size_t i = path.size(); i > 0; --i) {
			if (is_separator(path[i-1])) {
				return path.substr(i);
			}
		}
		return path;
	}
	std::string extension() const {
		for (std::size_t i = path.size(); i > 0; --i) {
			if (path[i-1] == '.') {
				return path.substr(i);
			}
			else if (is_separator(path[i-1])) {
				break;
			}
		}
		return std::string();
	}
	Path with_extension(const char* extension) const {
		for (std::size_t i = path.size(); i > 0; --i) {
			if (path[i-1] == '.') {
				return Path(path.substr(0, i) + extension);
			}
			else if (is_separator(path[i-1])) {
				break;
			}
		}
		return Path(path + '.' + extension);
	}
	Path parent() const {
		for (std::size_t i = path.size(); i > 0; --i) {
			if (is_separator(path[i-1])) {
				return Path(path.substr(0, i-1));
			}
		}
		return Path();
	}
	Path& operator /=(const char* s) {
		path += separator;
		path += s;
		return *this;
	}
	Path operator /(const char* s) const& {
		return Path(path + separator + s);
	}
	Path operator /(const char* s) && {
		return Path(std::move(path) + separator + s);
	}
	Path operator /(const std::string& s) const& {
		return Path(path + separator + s);
	}
	Path operator /(const std::string& s) && {
		return Path(std::move(path) + separator + s);
	}
};

class Directory {
	#ifdef _WIN32
	HANDLE handle;
	WIN32_FIND_DATA find_data;
	#else
	DIR* dir;
	#endif
public:
	Directory(const char* path) {
		#ifdef _WIN32
		std::string pattern(path);
		pattern.append("\\*");
		handle = FindFirstFile(pattern.c_str(), &find_data);
		#else
		dir = opendir(path);
		#endif
	}
	Directory(const Path& path): Directory(path.c_str()) {}
	Directory() {
		#ifdef _WIN32
		handle = INVALID_HANDLE_VALUE;
		#else
		dir = nullptr;
		#endif
	}
	Directory(const Directory&) = delete;
	Directory(Directory&& directory) {
		#ifdef _WIN32
		handle = directory.handle;
		directory.handle = INVALID_HANDLE_VALUE;
		#else
		dir = directory.dir;
		directory.dir = nullptr;
		#endif
	}
	~Directory() {
		#ifdef _WIN32
		if (handle != INVALID_HANDLE_VALUE) {
			FindClose(handle);
		}
		#else
		if (dir) {
			closedir(dir);
		}
		#endif
	}
	Directory& operator =(const Directory&) = delete;
	Directory& operator =(Directory&& directory) {
		#ifdef _WIN32
		std::swap(handle, directory.handle);
		#else
		std::swap(dir, directory.dir);
		#endif
		return *this;
	}
	explicit operator bool() const {
		#ifdef _WIN32
		return handle != INVALID_HANDLE_VALUE;
		#else
		return dir != nullptr;
		#endif
	}
	class Iterator {
		#ifdef _WIN32
		HANDLE handle;
		LPWIN32_FIND_DATA find_data;
		#else
		DIR* dir;
		struct dirent *ent;
		#endif
	public:
		#ifdef _WIN32
		Iterator(HANDLE handle, LPWIN32_FIND_DATA find_data): handle(handle), find_data(find_data) {}
		#else
		Iterator(DIR* dir, struct dirent* ent): dir(dir), ent(ent) {}
		#endif
		bool operator !=(const Iterator& iterator) const {
			#ifdef _WIN32
			return find_data != iterator.find_data;
			#else
			return ent != iterator.ent;
			#endif
		}
		const char* operator *() const {
			#ifdef _WIN32
			return find_data->cFileName;
			#else
			return ent->d_name;
			#endif
		}
		Iterator& operator ++() {
			#ifdef _WIN32
			if (!FindNextFile(handle, find_data)) {
				find_data = nullptr;
			}
			#else
			ent = readdir(dir);
			#endif
			return *this;
		}
	};
	Iterator begin() {
		#ifdef _WIN32
		if (handle != INVALID_HANDLE_VALUE) {
			return Iterator(handle, &find_data);
		}
		else {
			return Iterator(handle, nullptr);
		}
		#else
		if (dir != nullptr) {
			return Iterator(dir, readdir(dir));
		}
		else {
			return Iterator(dir, nullptr);
		}
		#endif
	}
	Iterator end() {
		#ifdef _WIN32
		return Iterator(handle, nullptr);
		#else
		return Iterator(dir, nullptr);
		#endif
	}
};

class Mmap {
	void* address;
	std::size_t size;
public:
	Mmap(const char* path) {
		#ifdef _WIN32
		HANDLE file = CreateFile(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		LARGE_INTEGER large_integer;
		GetFileSizeEx(file, &large_integer);
		size = large_integer.QuadPart;
		HANDLE mapping = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
		address = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, size);
		CloseHandle(mapping);
		CloseHandle(file);
		#else
		const int fd = open(path, O_RDONLY);
		if (fd != -1) {
			struct stat stat;
			fstat(fd, &stat);
			size = stat.st_size;
			address = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
			if (address == MAP_FAILED) {
				address = nullptr;
				size = 0;
			}
			close(fd);
		}
		else {
			address = nullptr;
			size = 0;
		}
		#endif
	}
	Mmap(const Path& path): Mmap(path.c_str()) {}
	Mmap(): address(nullptr), size(0) {}
	Mmap(const Mmap&) = delete;
	Mmap(Mmap&& mmap) {
		address = mmap.address;
		size = mmap.size;
		mmap.address = nullptr;
		mmap.size = 0;
	}
	~Mmap() {
		#ifdef _WIN32
		UnmapViewOfFile(address);
		#else
		if (address) {
			munmap(address, size);
		}
		#endif
	}
	Mmap& operator =(const Mmap&) = delete;
	Mmap& operator =(Mmap&& mmap) {
		std::swap(address, mmap.address);
		std::swap(size, mmap.size);
		return *this;
	}
	explicit operator bool() const {
		return address != nullptr;
	}
	const char* data() const {
		return static_cast<char*>(address);
	}
	std::size_t get_size() const {
		return size;
	}
	char operator [](std::size_t i) const {
		return static_cast<char*>(address)[i];
	}
	const char* begin() const {
		return static_cast<char*>(address);
	}
	const char* end() const {
		return static_cast<char*>(address) + size;
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
