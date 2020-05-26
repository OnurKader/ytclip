#pragma once

#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

class Downloader final
{
public:
private:
	std::vector<std::pair<FILE*, pid_t>> m_processes;

	std::pair<FILE*, pid_t> init_download(const char*);
};
