#pragma once

#include "process.hpp"

#include <cstdio>
#include <vector>

struct Downloader final
{
	explicit Downloader(const uint32_t);
	void download(const char*) noexcept;

	std::vector<Process> processes;
	uint32_t thread_count;
};
