#include "xclipboard.hpp"

#include <climits>
#include <fmt/printf.hpp>

XClipBoard::XClipBoard() :
	m_display(XOpenDisplay(nullptr)),
	m_window(XCreateSimpleWindow(m_display,
								 DefaultRootWindow(m_display),
								 0,
								 0,
								 1U,
								 1U,
								 0U,
								 BlackPixel(m_display, DefaultScreen(m_display)),
								 BlackPixel(m_display, DefaultScreen(m_display))))
{
}

XClipBoard::~XClipBoard()
{
	XDestroyWindow(m_display, m_window);
	XCloseDisplay(m_display);
}

std::optional<std::string> XClipBoard::get_selection(const char* bufname, const char* fmtname)
{
	unsigned char* result = nullptr;
	uint64_t ressize, restail;
	int resbits;
	Atom bufid = XInternAtom(m_display, bufname, False),
		 fmtid = XInternAtom(m_display, fmtname, False),
		 propid = XInternAtom(m_display, "XSEL_DATA", False),
		 incrid = XInternAtom(m_display, "INCR", False);
	XEvent event;

	XConvertSelection(m_display, bufid, fmtid, propid, m_window, CurrentTime);
	do
	{
		XNextEvent(m_display, &event);
	} while(event.type != SelectionNotify || event.xselection.selection != bufid);

	if(event.xselection.property)
	{
		XGetWindowProperty(m_display,
						   m_window,
						   propid,
						   0,
						   LONG_MAX / 4,
						   False,
						   AnyPropertyType,
						   &fmtid,
						   &resbits,
						   &ressize,
						   &restail,
						   &result);

		if(fmtid == incrid)
		{
			fmt::print("Buffer is too large and INCR reading is not implemented "
					   "yet.\n");
			return std::nullopt;
		}

		const std::string return_value = fmt::sprintf("%.*s", ressize, result);

		XFree(result);
		return return_value;
	}
	else	// request failed, e.g. owner can't convert to the target format
		return std::nullopt;
}
