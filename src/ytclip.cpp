#include "ctre.hpp"
#include "cxxopts.hpp"

#include <X11/Xlib.h>
#include <array>
#include <chrono>
#include <climits>
#include <csignal>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <optional>
#include <process.hpp>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#define DEBUG 1

using namespace std::chrono_literals;
using namespace ctre::literals;

static Display *display = nullptr;
static Window window;

// TODO: Design Choice!!! std::future std::async or popen()??
// Maybe tiny-process-library?

bool getSelection(const char *bufname, const char *fmtname, std::string &buff)
{
	unsigned char *result = nullptr;
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
						   &result);

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

void handleInterrupt(int sig)
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
}

std::optional<std::string> getYouTubeID(const std::string_view url)
{
	std::optional<std::string> id;
	if(!isYouTube(url))
		return std::nullopt;

	if(auto match = ctre::match<R"(watch\?v=([a-zA-Z0-9\-]+))">(url))
		return match.get<1>().str();

	return id;
}

[[maybe_unused]] static constexpr std::array example_urls {
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

	// FIXME: Why don't I just store one vector and pass that to download?
	std::vector<std::string> valid_url_vector;
	std::vector<std::string> valid_id_vector;

	// TODO: Make a cleaner DownloadInfo class/struct, no need for privacy
	//	std::vector<DownloadInfo> processes;
	std::string clip_buffer;
	std::string prev_clip_buffer;

	// Thanks clang-format, you've really fucked this part up
	cxxopts::Options options(
		"ytclip", "A wrapper around youtube-dl to download videos from the clipboard in parallel");
	options.add_options()("f,format",
						  "Specify a video/audio format",
						  cxxopts::value<std::string>()->default_value("bestvideo+bestaudio"))(
		"o,output",
		"Specify an output file format",
		cxxopts::value<std::string>()->default_value("%(title)s.%(ext)s"))("h,help", "Print usage")(
		"v,version", "Print usage");

	auto arg_result = options.parse(argc, argv);

	if(arg_result.count("help") || arg_result.count("version"))
	{
		usage();
		return 1;
	}

	/*
	TinyProcessLib::Process process1a("echo Hello World", "", [](const char *bytes, size_t n) {
		std::string temp(bytes, n);
		std::reverse(temp.begin(), temp.end());
		fmt::print("Hello World : {}\n", temp);
	});
	*/

	const std::string format = arg_result["format"].as<std::string>();
	const std::string output = arg_result["output"].as<std::string>();

	const std::string program =
		fmt::format("youtube-dl --newline -wcif {} -o '{}' ", std::move(format), std::move(output));

	// Open a window and get the clipboard information from that
	initXStuff();

	// Variable to hold the current line index of the output buffer for the processes
	while(true)
	{
		const bool result = getSelection("CLIPBOARD", "UTF8_STRING", clip_buffer) ||
							getSelection("CLIPBOARD", "STRING", clip_buffer);

		if(!result)
			fmt::print(stderr, "I have no idea what this is for\n");

		std::this_thread::sleep_for(400ms);
		fmt::print("Clipboard: {}\t", clip_buffer);
		fmt::print("It's {}valid\n", (isYouTube(clip_buffer) ? "" : "not "));
	}

	return 0;
}
