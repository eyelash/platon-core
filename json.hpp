#pragma once

#include <cstdint>
#include <string>

class JSONWriter {
	std::string& string;
	static constexpr std::size_t strlen(const char* s, std::size_t i = 0) {
		return *s == '\0' ? i : strlen(s + 1, i + 1);
	}
public:
	JSONWriter(std::string& string): string(string) {}
	template <class I> void write_string(I first, I last) {
		string.push_back('"');
		for (; first != last; ++first) {
			const char c = *first;
			if (c == '\n') {
				string.push_back('\\');
				string.push_back('n');
			}
			else if (c == '\t') {
				string.push_back('\\');
				string.push_back('t');
			}
			else {
				if (c == '"' || c == '\\') {
					string.push_back('\\');
				}
				string.push_back(c);
			}
		}
		string.push_back('"');
	}
	void write_string(const char* s) {
		write_string(s, s + strlen(s));
	}
	void write_string(const std::string& s) {
		write_string(s.begin(), s.end());
	}
	void write_number(std::int64_t n) {
		if (n < 0) {
			string.push_back('-');
			n = -n;
		}
		if (n >= 10) {
			write_number(n / 10);
		}
		string.push_back('0' + (n % 10));
	}
	void write_boolean(bool b) {
		string.append(b ? "true" : "false");
	}
	template <class F> void write_object(F f) {
		string.push_back('{');
		ObjectWriter object_writer(*this);
		f(object_writer);
		string.push_back('}');
	}
	template <class F> void write_array(F f) {
		string.push_back('[');
		ArrayWriter array_writer(*this);
		f(array_writer);
		string.push_back(']');
	}
	class ObjectWriter {
		JSONWriter& writer;
		bool object_start;
	public:
		ObjectWriter(JSONWriter& writer): writer(writer), object_start(true) {}
		JSONWriter& write_member(const char* name) {
			if (object_start)
				object_start = false;
			else
				writer.string.push_back(',');
			writer.write_string(name);
			writer.string.push_back(':');
			return writer;
		}
	};
	class ArrayWriter {
		JSONWriter& writer;
		bool array_start;
	public:
		ArrayWriter(JSONWriter& writer): writer(writer), array_start(true) {}
		JSONWriter& write_element() {
			if (array_start)
				array_start = false;
			else
				writer.string.push_back(',');
			return writer;
		}
	};
};

using JSONObjectWriter = JSONWriter::ObjectWriter;
using JSONArrayWriter = JSONWriter::ArrayWriter;
