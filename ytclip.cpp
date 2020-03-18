#include <X11/Xlib.h>
#include <climits>
#include <csignal>
#include <cstdio>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <unistd.h>
#include <vector>

#define DEBUG 0

constexpr const uint16_t BUFF_SIZE = 256U;

static Display *display = nullptr;
static Window window;

struct DownloadInfo
{
	FILE *fp;
	const char *id;
	DownloadInfo() = delete;
	DownloadInfo(FILE *_fp, const char *vid_id)
	{
		this->fp = _fp;
		this->id = vid_id;
	}
	DownloadInfo(const DownloadInfo &other)
	{
		this->fp = other.fp;
		this->id = other.id;
	}

	//	~DownloadInfo() { pclose(this->fp); }
};

bool getSelection(const char *bufname, const char *fmtname, char buff[BUFF_SIZE])
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
			printf("Buffer is too large and INCR reading is not implemented "
				   "yet.\n");
		else
			sprintf(buff, "%.*s", static_cast<int>(ressize), result);

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

bool isYouTube(const char *url)
{
	std::regex regex(R"(^(https?://)?(www\.)?(youtube.com|youtu.be))");
	return std::regex_search(url, regex);
}

void usage()
{
	std::cerr << "Usage: -h --help,\nand -v --version for this page\n"
				 "-f --format takes in a video format like 22 or best\n"
				 "-o --output takes in an output format specified by youtube-dl"
			  << std::endl;
	exit(1);
}

const char *getYouTubeID(const char *url)
{
	const char *ret_val = nullptr;
	if(!isYouTube(url))
		return ret_val;
	std::regex regex(R"(watch\?v=([[:w:]-]+))");
	std::cmatch matches;

	if(std::regex_search(url, matches, regex))
		ret_val = strdup(matches.str(1).c_str());

	return ret_val;
}

static constexpr const char *example_urls[] = {
	"http://www.youtube.com/watch?v=9_I7nbckQU8",	 // Unlisted
	"https://youtube.com/watch?v=uIitNw0Q7yg",
	"https://www.youtube.com/watch?v=8YWl7tDGUPA",
	"https://www.youtube.com/watch?v=668nUCeBHyY",
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
	if(!url)
		return;

	if(vec.empty())
	{
		vec.push_back(url);
		return;
	}

	for(const auto &elem: vec)
		if(!strcmp(elem, url))
			return;

	vec.push_back(url);
}

bool in_vector(const std::vector<DownloadInfo> &vec, const char *elem)
{
	for(const auto &item: vec)
		if(!strcmp(elem, item.id))
			return true;

	return false;
}

void insertStrLikeSet(std::vector<DownloadInfo> &vec, FILE *elem, const char *id)
{
	if(vec.empty())
	{
		vec.push_back(DownloadInfo(elem, id));
		return;
	}

	for(const auto &item: vec)
		if(item.fp == elem)
			return;

	vec.push_back(DownloadInfo(elem, id));
}

std::ostream &operator<<(std::ostream &os, const std::vector<const char *> &vec)
{
	for(const auto &elem: vec)
		os << elem << ' ';
	return os;
}

inline void cls()
{
	printf("\033[2J\033[3J\033[H");
	fflush(stdout);
}

FILE *initDownload(char *program, const char *id, std::vector<DownloadInfo> &process_vector)
{
	if(!in_vector(process_vector, id))
	{
		FILE *retval = nullptr;
		strcat(program, id);
		strcat(program, " 2>&1 ");

#if DEBUG
		printf("ID: %s, Program: %s\n", id, program);
#endif

		retval = popen(program, "er");
		if(!retval)
			exit(2);

		// Eat up the first two unnecessary lines of output, might put this on a thread but nah,
		// don't want to mess with multithreading myself for this, can't deal with the mutexes
		char buff[128U];
		fgets(buff, sizeof(buff), retval);
		fgets(buff, sizeof(buff), retval);

		insertStrLikeSet(process_vector, retval, id);
		return retval;
	}
	return nullptr;
}

char *getProgressOfDownload(const DownloadInfo &di)
{
	char buff[128U];
	while(fgets(buff, sizeof(buff), di.fp))
	{
		// Might do regex stuff
		/*
			std::regex percentage(R"(\[download\]\s+(.+))");
			std::cmatch match;
			if(std::regex_search(buff, match, percentage))
			{
				buff[strlen(buff) - 1] = '\0';
				printf("\033[1;32mProgress: %-52s\033[m %s\n",
					   match.str(1).c_str(),
					   buff);
			}
		*/
		if(buff[0] != '[')
			return nullptr;
		return strdup(strchr(buff, ' ') + 1U);
	}

	return nullptr;
}

int main(int argc, char **argv)
{
	signal(SIGINT, handleInterrupt);

	char format[24] = "bestvideo+bestaudio";
	char output[64] = "%(title)s.%(ext)s";

	std::vector<const char *> valid_url_vector;
	std::vector<const char *> valid_id_vector;
	std::vector<DownloadInfo> processes;
	char clip_buffer[BUFF_SIZE];
	char prev_clip_buffer[BUFF_SIZE];

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
	cls();

	char program[285];
	sprintf(program, "youtube-dl --newline -wcif %s -o '%s' ", format, output);

	// Open a window and get the clipboard information from that
	initXStuff();

	// Variable to hold the current line index of the output buffer for the processes
	while(true)
	{
		bool result = getSelection("CLIPBOARD", "UTF8_STRING", clip_buffer) ||
					  getSelection("CLIPBOARD", "STRING", clip_buffer);
		if(!result || !strcmp(prev_clip_buffer, clip_buffer))
			goto loops_end;

		// clip_buffer holds the latest clipboard entry
		// Check if it's a valid youtube link, doesn't do just the id
		// If it's valid, insert URL(?) and ID to the appropriate vectors
		// and initialize the download with the specified program
		// while storing its FILE* in another vector
		if(isYouTube(clip_buffer))
		{
			insertURL(valid_url_vector, strdup(clip_buffer));
			insertURL(valid_id_vector, getYouTubeID(valid_url_vector.back()));
			initDownload(program, valid_id_vector.back(), processes);
		}

	loops_end:
		strcpy(prev_clip_buffer, clip_buffer);
		usleep(333333);
		for(uint16_t i = 0, line_index = 0; i < processes.size(); ++i)
		{
			char *buff;
			buff = getProgressOfDownload(processes[line_index]);
			if(!buff)	 // Finished Downloading
			{
				// Remove from vector and continue with loop?
				processes.erase(processes.begin() + i--);
				continue;
			}
			printf("\033[2K\033[1K\033[%uH%s\n", line_index++, buff);
			free(buff);
		}
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

	return 0;
}
