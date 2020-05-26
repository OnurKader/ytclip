#include "download.hpp"

std::pair<FILE*, pid_t> Downloader::init_download(const char* program)
{
	const pid_t fork_pid = fork();
	switch(fork_pid)
	{
		case -1:
			// error
			break;
		case 0:
			// The child
			// Args == ? array, vector, init_list?
			// execvp(program, args);
			break;
		default:
			// The parent and the pid is the correct one to push, maybe a set?, into the vector
			break;
	}

	return std::make_pair(nullptr, -1);
}
