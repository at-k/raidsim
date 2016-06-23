/*
    This file is part of M's INI file format library

    M's INI file format library is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    M's INI file format library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with M's INI file format library.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef Ini_H
#define Ini_H

#include <string>
#include <iostream>
#include <vector>

#define BUFFER_SIZE 1024*8

#ifdef WIN32
#pragma warning(disable:4099)
#endif

class ini_document
{
	public:
	class section
	{
		struct option
		{
			std::string label;
			std::string value;
			std::string comment;
			public:
				option();
				option(std::string label, std::string value, std::string comment = std::string());
				~option();
		};

		std::string name;
		std::string comment;	/* in case there is a comment on the same line as the section header */
		std::vector<class option> option_list;

		public:
		section();
		section(std::string name, std::string comment = std::string());
		~section();

		std::string operator[](const std::string &label);

		void pushOption(std::string name, std::string value, std::string comment = std::string());
		void pushComment(std::string comment);

		section & operator=(const section &rhs);

		friend class ini_document;
	}w;


	private:
	std::vector<ini_document::section> section_list;
	// temp vars needed by the lexer
	char buffer[BUFFER_SIZE];	// lexer input buffer
	char *p, *q, *marker, *limit, *eos;	// position markers
	std::string token;

	std::ostream *out;

	public:
		ini_document();
		~ini_document();

		section & operator[](const std::string);

		ini_document::section & createSection(std::string name, std::string comment = std::string());

		void toOutputStream(std::ostream &os);
		ini_document fromInputStream(std::istream &is);

		void set_error_output(std::ostream &o);

	protected:
		void fill(std::istream &is);
		int lexer(std::istream &is);
		enum lexer_tokens {SECTION = 256, TEXT, ESCAPED_TEXT, COMMENT, EOL, EOS, WS, ERROR};

};


#endif

