#include <X11/Xlib.h>
#include "xxHash/xxhash.h"
#include <climits>
#include <cstdio>
#include <regex>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

/* TODO:
 * popen()
 * wait4()
 * clone()
 * vfork()
*/

constexpr const uint8_t BUFF_SIZE = 128U;
constexpr const uint8_t SEED = 4U;

Bool getSelection(Display *display,
				  Window window,
				  const char *bufname,
				  const char *fmtname,
				  char buff[128])
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

// Probably popen, store the outputs in a vector of strings
// printf on seperate lines, mutex stdio (?)

// TODO Hash
/*
	XXH64_hash_t hash = XXH64(buffer, size, seed);
*/

int main(int argc, char **argv)
{
	char clip_buffer[BUFF_SIZE];
	Display *display = XOpenDisplay(NULL);
	unsigned long color = BlackPixel(display, DefaultScreen(display));
	Window window = XCreateSimpleWindow(
		display, DefaultRootWindow(display), 0, 0, 1, 1, 0, color, color);
	while(true)
	{
		Bool result =
			getSelection(display, window, "CLIPBOARD", "UTF8_STRING", clip_buffer) ||
			getSelection(display, window, "CLIPBOARD", "STRING", clip_buffer);
		printf("Clip String: %s\n", clip_buffer);
		XXH64_hash_t clip_hash = XXH64(clip_buffer, sizeof(clip_buffer), SEED);
		printf("Clip Hash: %lu\n", clip_hash);
		usleep(333'333);
	}

	/* printf("\n"); */
	/* const char prog[] = "list -a"; */
	/* FILE *list_output = popen(prog, "r"); */
	/* char buff[256]; */
	/* size_t read_count = 0ULL; */
	/* while(fgets(buff, sizeof(buff), list_output)) */
	/* 	printf("%s", buff); */
	/* pclose(list_output); */

	XDestroyWindow(display, window);
	XCloseDisplay(display);

	return 0;
}

