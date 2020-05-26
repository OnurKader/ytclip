#include "process.hpp"

Process::Process(const char* program, const char* perms) :
	fp(popen(program, perms)), m_buffer {0U}, m_str {}
{
}

Process::~Process()
{
	if(fp)
		pclose(fp);
}

Process::Process(Process&& other) :
	fp(other.fp), m_buffer(std::move(other.m_buffer)), m_str(std::move(other.m_str))
{
	other.fp = nullptr;
}

std::string Process::read()
{
	while(fgets(m_buffer.data(), static_cast<int>(m_buffer.size()), fp))
		m_str.append(m_buffer.data());
	return std::move(m_str);
}
