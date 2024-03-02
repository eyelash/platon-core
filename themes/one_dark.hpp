constexpr Theme one_dark_theme = {
	Color::hsl(220, 13, 18), // background
	Color::hsl(220, 100, 80).with_alpha(0.04f), // background_active
	Color::hsl(220, 13, 18 + 10), // selection
	Color::hsl(220, 100, 66), // cursor
	Color::hsl(220, 13, 18), // number_background
	Color::hsl(220, 13, 18), // number_background_active
	Style(Color::hsl(220, 13, 18) + Color::hsl(220, 14, 71 - 26).with_alpha(0.6f)), // number
	Style(Color::hsl(220, 13, 18) + Color::hsl(220, 14, 71).with_alpha(0.6f)), // number_active
	{
		Style(Color::hsl(220, 14, 71)), // text
		Style(Color::hsl(220, 10, 40), Style::ITALIC), // comments
		Style(Color::hsl(286, 60, 67)), // keywords
		Style(Color::hsl(286, 60, 67)), // operators
		Style(Color::hsl(187, 47, 55)), // types
		Style(Color::hsl(29, 54, 61)), // literals
		Style(Color::hsl(95, 38, 62)), // strings
		Style(Color::hsl(207, 82, 66)) // function names
	}
};
