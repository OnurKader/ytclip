#pragma once

#include <X11/Xlib.h>
#include <optional>
#include <string>

class XClipBoard final
{
public:
	XClipBoard();
	~XClipBoard();

	std::optional<std::string> get_selection(const char*, const char*);

private:
	Display* m_display;
	Window m_window;
};
