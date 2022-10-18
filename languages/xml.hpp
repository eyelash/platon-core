constexpr auto xml_comment = sequence("<!--", repetition(but("-->")), optional("-->"));

constexpr auto xml_white_space = repetition(choice(' ', '\t', '\n', '\r'));
constexpr auto xml_name_start_char = choice(range('a', 'z'), range('A', 'Z'), ':', '_');
constexpr auto xml_name_char = choice(xml_name_start_char, '-', '.', range('0', '9'));
constexpr auto xml_name = sequence(xml_name_start_char, repetition(xml_name_char));

constexpr auto xml_syntax = repetition(choice(
	highlight(Style::COMMENT, xml_comment),
	highlight(Style::KEYWORD, sequence(
		sequence('<', xml_name),
		xml_white_space,
		highlight(Style::TYPE, repetition(sequence(
			xml_name,
			xml_white_space,
			'=',
			xml_white_space,
			highlight(Style::LITERAL, sequence('"', repetition(but('"')), '"')),
			xml_white_space
		))),
		choice('>', "/>")
	)),
	highlight(Style::KEYWORD, sequence("</", xml_name, xml_white_space, '>')),
	any_char()
));
