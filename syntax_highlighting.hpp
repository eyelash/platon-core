#pragma once

#include "json.hpp"

class Color {
	static constexpr Color hue(float h) {
		return
		    h <= 1.f ? Color(1.f, h-0.f, 0.f)
		  : h <= 2.f ? Color(2.f-h, 1.f, 0.f)
		  : h <= 3.f ? Color(0.f, 1.f, h-2.f)
		  : h <= 4.f ? Color(0.f, 4.f-h, 1.f)
		  : h <= 5.f ? Color(h-4.f, 0.f, 1.f)
		  : Color(1.f, 0.f, 6.f-h);
	}
	constexpr Color saturation(float s) const {
		return Color(red * s + 1.f - s, green * s + 1.f - s, blue * s + 1.f - s, alpha);
	}
	constexpr Color value(float v) const {
		return Color(red * v, green * v, blue * v, alpha);
	}
public:
	float red;
	float green;
	float blue;
	float alpha;
	constexpr Color(float red, float green, float blue, float alpha = 1.f): red(red), green(green), blue(blue), alpha(alpha) {}
	static constexpr Color hsv(float h, float s, float v) {
		return hue(h / 60.f).saturation(s / 100.f).value(v / 100.f);
	}
	void write(JSONWriter& writer) const {
		writer.write_array([&](JSONArrayWriter& writer) {
			writer.write_element().write_number(red * 255.f + .5f);
			writer.write_element().write_number(green * 255.f + .5f);
			writer.write_element().write_number(blue * 255.f + .5f);
			writer.write_element().write_number(alpha * 255.f + .5f);
		});
	}
};

struct Theme {
	Color background;
	Color cursor;
	Color text[5];
	void write(JSONWriter& writer) const {
		writer.write_object([&](JSONObjectWriter& writer) {
			background.write(writer.write_member("background"));
			cursor.write(writer.write_member("cursor"));
			writer.write_member("text").write_array([&](JSONArrayWriter& writer) {
				for (const Color& color: text) {
					color.write(writer.write_element());
				}
			});
		});
	}
};

constexpr Theme default_theme = {
	Color::hsv(0, 0, 100), // background
	Color::hsv(210, 100, 100), // cursor
	{
		Color::hsv(0, 0, 20), // text
		Color::hsv(0, 0, 60), // comments
		Color::hsv(270, 80, 80), // keywords
		Color::hsv(210, 80, 80), // types
		Color::hsv(150, 80, 80) // literals
	}
};
