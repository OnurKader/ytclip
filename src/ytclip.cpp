#include "ctre.hpp"
#include "cxxopts.hpp"

#include <X11/Xlib.h>
#include <array>
#include <climits>
#include <csignal>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <optional>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

#define DEBUG 1

using namespace ctre::literals;

static Display *display = nullptr;
static Window window;

bool getSelection(const char *bufname, const char *fmtname, std::string &buff)
{
	char *result;
	uint64_t ressize, restail;
	int resbits;
	Atom bufid = XInternAtom(display, bufname, False), fmtid = XInternAtom(display, fmtname, False),
		 propid = XInternAtom(display, "XSEL_DATA", False),
		 incrid = XInternAtom(display, "INCR", False);
	XEvent event;

	XConvertSelection(display, bufid, fmtid, propid, window, CurrentTime);
	do
	{
		XNextEvent(display, &event);
	} while(event.type != SelectionNotify || event.xselection.selection != bufid);

	if(event.xselection.property)
	{
		XGetWindowProperty(display,
						   window,
						   propid,
						   0,
						   LONG_MAX / 4,
						   False,
						   AnyPropertyType,
						   &fmtid,
						   &resbits,
						   &ressize,
						   &restail,
						   reinterpret_cast<unsigned char **>(&result));

		if(fmtid == incrid)
			fmt::print("Buffer is too large and INCR reading is not implemented "
					   "yet.\n");
		else
			buff = fmt::sprintf("%.*s", ressize, result);

		XFree(result);
		return true;
	}
	else	// request failed, e.g. owner can't convert to the target format
		return false;
}

void handleInterrupt(const int sig)
{
	XDestroyWindow(display, window);
	XCloseDisplay(display);

	exit(sig - 2);
}

void initXStuff()
{
	display = XOpenDisplay(nullptr);
	unsigned long color = BlackPixel(display, DefaultScreen(display));
	window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, color, color);
}

bool isYouTube(const std::string_view url)
{
	return ctre::search<R"(^(https?://)?(www\.)?(youtube.com|youtu.be))">(url);
}

void usage()
{
	std::cerr << "Usage: -h --help,\nand -v --version for this page\n"
				 "-f --format takes in a video format like 22 or best\n"
				 "-o --output takes in an output format specified by youtube-dl"
			  << std::endl;
	exit(1);
}

// FIXME: return type shouldn't be const char*
std::optional<std::string> getYouTubeID(const std::string_view url)
{
	std::optional<std::string> id;
	if(!isYouTube(url))
		return std::nullopt;

	if(auto match = ctre::match<R"(watch\?v=([a-zA-Z0-9\-]+))">(url))
		return match.get<1>().str();

	return id;
}

static constexpr std::array example_urls {
	"http://www.youtube.com/watch?v=9_I7nbckQU8",	 // Unlisted
	"https://youtube.com/watch?v=uIitNw0Q7yg",
	"https://www.youtube.com/watch?v=8YWl7tDGUPA",
	"https://www.youtube.com/watch?v=668nUCeBHyY",
	"www.google.com/",
	"http://www.youtube.com/blaze-it",
	"http://www.duckduckgo.com/?q=youtube.com",
	"https://duckduckgo.com/?q=https://www.youtube.com/"};

int main(int argc, char **argv)
{
	signal(SIGINT, handleInterrupt);

	std::string format {"bestvideo+bestaudio"};
	std::string output {"%(title)s.%(ext)s"};

	// FIXME: These should hold std::string's
	std::vector<const char *> valid_url_vector;
	std::vector<const char *> valid_id_vector;

	// TODO: Make a cleaner DownloadInfo class/struct, no need for privacy
	//	std::vector<DownloadInfo> processes;
	std::string clip_buffer;
	std::string prev_clip_buffer;

	// getopt scope
	{
		int c;
		while(true)
		{
			int option_index = 0;
			static struct option long_options[] = {{"format", required_argument, nullptr, 'f'},
												   {"output", required_argument, nullptr, 'o'},
												   {"help", no_argument, nullptr, 'h'},
												   {"version", no_argument, nullptr, 'v'},
												   {nullptr, 0, nullptr, 0}};
			c = getopt_long(argc, argv, "hvf:o:", long_options, &option_index);
			if(c == -1)
				break;

			switch(c)
			{
				case 'h':
				case 'v': usage(); break;
				case 'f': strcpy(format, optarg); break;
				case 'o': strcpy(output, optarg); break;
				default: usage();
			}
		}
	}	 // end of getopt

	// Clear the screen and put cursor at top left so reading and
	// further printing is easier

	const std::string program =
		fmt::sprintf("youtube-dl --newline -wcif %s -o '%s' ", format, output);

	// Open a window and get the clipboard information from that
	initXStuff();

	// Variable to hold the current line index of the output buffer for the processes
	while(true)
	{
		bool result = getSelection("CLIPBOARD", "UTF8_STRING", clip_buffer) ||
					  getSelection("CLIPBOARD", "STRING", clip_buffer);
	}

	return 0;
}
