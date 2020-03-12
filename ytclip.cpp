#include <X11/Xlib.h>
#include <climits>
#include <csignal>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

constexpr const uint16_t BUFF_SIZE = 256U;
constexpr const uint16_t SEED = 42069U;

static Display *display = nullptr;
static Window window;

bool getSelection(const char *bufname,
				  const char *fmtname,
				  char buff[BUFF_SIZE])
{
	char *result;
	unsigned long ressize, restail;
	int resbits;
	Atom bufid = XInternAtom(display, bufname, False),
		 fmtid = XInternAtom(display, fmtname, False),
		 propid = XInternAtom(display, "XSEL_DATA", False),
		 incrid = XInternAtom(display, "INCR", False);
	XEvent event;

	XConvertSelection(display, bufid, fmtid, propid, window, CurrentTime);
	do
	{
		XNextEvent(display, &event);
	} while(event.type != SelectionNotify ||
			event.xselection.selection != bufid);

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
						   (unsigned char **)&result);

		if(fmtid == incrid)
			printf("Buffer is too large and INCR reading is not implemented "
				   "yet.\n");
		else
			sprintf(buff, "%.*s", (int)ressize, result);

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

	exit(sig);
}

void initXStuff()
{
	display = XOpenDisplay(NULL);
	unsigned long color = BlackPixel(display, DefaultScreen(display));
	window = XCreateSimpleWindow(
		display, DefaultRootWindow(display), 0, 0, 1, 1, 0, color, color);
}

bool isYouTube(const char *url, std::cmatch *matches = nullptr)
{
	std::regex regex(R"(^(https?://)?(www\.)?(youtube.com|youtu.be))");
	if(matches)
		return std::regex_search(url, *matches, regex);
	else
		return std::regex_search(url, regex);
}

static constexpr const char *example_urls[] = {
	"https://www.youtube.com/watch?v=0_20-hj4B213k",
	"http://youtube.com/watch?v=anan_l0l-69",
	"youtu.be/watch?v=abde_F91283-23&index=27",
	"www.google.com/",
	"http://www.youtube.com/blaze-it",
	"http://www.duckduckgo.com/?q=youtube.com",
	"https://duckduckgo.com/?q=https://www.youtube.com/"};

void checkValidURL()
{
	for(const auto &url: example_urls)
	{
		std::cout << std::left << std::setw(52) << url << " -> is"
				  << (isYouTube(url) ? "     " : " \033[1;31mnot\033[m ")
				  << "a valid YouTube URL\n";
	}
}

void insertURL(std::vector<const char *> &vec, const char *url)
{
	if(vec.empty())
	{
		vec.push_back(url);
		return;
	}

	bool url_is_in_vec = false;
	for(const auto &elem: vec)
	{
		if(!strcmp(elem, url))
		{
			url_is_in_vec = true;
			break;
		}
	}

	if(!url_is_in_vec)
		vec.push_back(url);
}

std::ostream &operator<<(std::ostream &os, const std::vector<const char *> &vec)
{
	for(const auto &elem: vec)
		os << elem << ' ';
	return os;
}

inline void cls() { printf("\033[2J\033[3J\033[H"); }

int main(int argc, char **argv)
{
	signal(SIGINT, handleInterrupt);

	std::vector<const char *> valid_url_vector;
	std::vector<FILE *> processes;
	std::cmatch matches;
	char clip_buffer[BUFF_SIZE];

	initXStuff();

	// Clear the screen so reading is easier
	cls();

	while(true)
	{
		Bool result = getSelection("CLIPBOARD", "UTF8_STRING", clip_buffer) ||
					  getSelection("CLIPBOARD", "STRING", clip_buffer);

		insertURL(valid_url_vector, strdup(clip_buffer));
		printf("Size: %zu\t", valid_url_vector.size());
		printf("\033[1H");
		fflush(stdout);
		usleep(333'333);
		std::cout << valid_url_vector << std::endl;
	}

	/*
	const char prog[] = "sleep 5";
	char buff[256];
	FILE *list_output = popen(prog, "r");
	printf("first\n");
	FILE *prog2 = popen(prog, "r");
	printf("second\n");
	FILE *output = fopen("/tmp/date-data", "r");
	while(fgets(buff, sizeof(buff), output))
		fputs(buff, stdout);
	fclose(output);
	pclose(prog2);
	pclose(list_output);
	*/

	XDestroyWindow(display, window);
	XCloseDisplay(display);

	checkValidURL();

	return 0;
}
