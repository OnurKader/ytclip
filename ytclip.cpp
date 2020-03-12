#include <X11/Xlib.h>
#include <climits>
#include <csignal>
#include <cstdio>
#include <regex>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_set>
#include <vector>

/* TODO:
 * popen()
 * wait4()
 * clone()
 * vfork()
 */

constexpr const uint16_t BUFF_SIZE = 256U;
constexpr const uint16_t SEED = 42069U;

Display *display = nullptr;
Window window;

Bool getSelection(const char *bufname, const char *fmtname, char buff[BUFF_SIZE])
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
						   (unsigned char **)&result);

		if(fmtid == incrid)
			printf("Buffer is too large and INCR reading is not implemented yet.\n");
		else
			sprintf(buff, "%.*s", (int)ressize, result);

		XFree(result);
		return True;
	}
	else	// request failed, e.g. owner can't convert to the target format
		return False;
}

void handleInterrupt(int sig)
{
	XDestroyWindow(display, window);
	XCloseDisplay(display);

	exit(sig);
}

void printSet(const std::unordered_set<std::string>& uset)
{
	for(const auto& elem: uset)
	{
		printf("%s\t", elem.c_str());
	}
}

int main(int argc, char **argv)
{
	signal(SIGINT, handleInterrupt);

	std::unordered_set<std::string> hash_table;
	std::vector<FILE *> processes;
	char clip_buffer[BUFF_SIZE];

	display = XOpenDisplay(NULL);
	unsigned long color = BlackPixel(display, DefaultScreen(display));
	window = XCreateSimpleWindow(
		display, DefaultRootWindow(display), 0, 0, 1, 1, 0, color, color);
	while(true)
	{
		Bool result = getSelection("CLIPBOARD", "UTF8_STRING", clip_buffer) ||
					  getSelection("CLIPBOARD", "STRING", clip_buffer);

		std::string clip_string(clip_buffer);
		hash_table.insert(clip_string);
		printf("Size: %zu\n", hash_table.size());
		printSet(hash_table);
		usleep(333'333);
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

	return 0;
}

