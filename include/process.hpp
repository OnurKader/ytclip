#pragma once

#include <array>
#include <cstdio>
#include <memory>
#include <string>

struct Process final
{
	Process(const char* program, const char* perms = "r");
	~Process();
	Process(const Process&) = delete;
	Process(Process&&);

	std::string read();

	FILE* fp;

private:
	std::array<char, 512U> m_buffer;
	std::string m_str;
};
