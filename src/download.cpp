#include "download.hpp"

Downloader::Downloader(const uint32_t count) : thread_count(count) {}

void Downloader::download(const char* program) noexcept { processes.emplace_back(program); }
