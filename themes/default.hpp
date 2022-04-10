constexpr Theme default_theme = {
	Color::hsv(0, 0, 100), // background
	Color::hsv(0, 0, 100), // background_active
	Color::hsv(60, 40, 100), // selection
	Color::hsv(0, 0, 20), // cursor
	Color::hsv(0, 0, 100), // number_background
	Color::hsv(0, 0, 100), // number_background_active
	Style(Color::hsv(0, 0, 60)), // number
	Style(Color::hsv(0, 0, 20)), // number_active
	{
		Style(Color::hsv(0, 0, 20)), // text
		Style(Color::hsv(0, 0, 60), Weight::NORMAL, true), // comments
		Style(Color::hsv(270, 80, 80), Weight::BOLD), // keywords
		Style(Color::hsv(210, 80, 80), Weight::BOLD), // types
		Style(Color::hsv(150, 80, 80)) // literals
	}
};
