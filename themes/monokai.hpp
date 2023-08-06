constexpr Theme monokai_theme = {
	Color::hsl(70, 8, 15), // background
	Color::hsl(70, 8, 15), // background_active
	Color::hsl(55, 8, 31).with_alpha(0.7f), // selection
	Color::hsl(60, 36, 96).with_alpha(0.9f), // cursor
	Color::hsl(70, 8, 15), // number_background
	Color::hsl(55, 11, 22), // number_background_active
	Style(Color::hsl(60, 30, 96).with_alpha(0.5f)), // number
	Style(Color::hsl(60, 30, 96).with_alpha(0.8f)), // number_active
	{
		Style(Color::hsl(60, 30, 96)), // text
		Style(Color::hsl(50, 11, 41)), // comments
		Style(Color::hsl(338, 95, 56)), // keywords
		Style(Color::hsl(190, 81, 67), Weight::NORMAL, true), // types
		Style(Color::hsl(54, 70, 68)) // literals
	}
};
