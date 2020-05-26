#include "ctre.hpp"
#include "cxxopts.hpp"
#include "xclipboard.hpp"

#include <array>
#include <chrono>
#include <fmt/format.hpp>
#include <fmt/printf.hpp>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>

using namespace std::chrono_literals;
using namespace ctre::literals;

// TODO: Design Choice!!! std::future std::async or popen()??
constexpr bool is_youtube_url(const std::string_view url)
{
	return ctre::search<R"(^(https?://)?(www\.)?(youtube.com|youtu.be))">(url);
}

std::optional<std::string> get_youtube_id(const std::string& url)
{
	if(!is_youtube_url(url))
		return std::nullopt;

	if(const auto match = ctre::match<R"(.+watch\?v=([a-zA-Z0-9\-_]+).*)">(url))
	{
		const auto str = match.get<1>().str();
		return str;
	}

	return std::nullopt;
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

int main(int argc, char** argv)
{
	// Thanks clang-format, you've really fucked this part up
	cxxopts::Options options("ytclip",
							 "A wrapper around youtube-dl to download videos from the clipboard in "
							 "parallel\nVersion: v0.1");
	options.add_options()("f,format",
						  "Specify a video/audio format",
						  cxxopts::value<std::string>()->default_value("bestvideo+bestaudio"))(
		"o,output",
		"Specify an output file format",
		cxxopts::value<std::string>()->default_value("%(title)s.%(ext)s"))("h,help", "Print usage")(
		"v,version", "Print usage");

	const auto arg_result = options.parse(argc, argv);

	if(arg_result.count("help") || arg_result.count("version"))
	{
		fmt::print(stderr, options.help());
		return 1;
	}

	const std::string format = arg_result["format"].as<std::string>();
	const std::string output = arg_result["output"].as<std::string>();

	const std::string program =
		fmt::format("youtube-dl --newline -wcif {} -o '{}' ", std::move(format), std::move(output));

	// Open a window and get the clipboard information from that
	XClipBoard xcb;

	// Put the video id's in a set so you don't try downloading them over and over again
	std::unordered_set<std::string> id_set;

	// Variable to hold the current line index of the output buffer for the processes
	while(true)
	{
		const auto utf_clip_optional = xcb.get_selection("CLIPBOARD", "UTF8_STRING");
		const auto reg_clip_optional = xcb.get_selection("CLIPBOARD", "STRING");

		if(!utf_clip_optional && !reg_clip_optional)
			fmt::print(stderr, "I have no idea what this is for\n");

		const std::string clip_buffer = [&]() -> std::string {
			if(utf_clip_optional)
				return utf_clip_optional.value();
			else if(reg_clip_optional)
				return reg_clip_optional.value();
			return {};
		}();

		if(const auto id_optional = get_youtube_id(clip_buffer); id_optional.has_value())
		{
			const auto& [in_set_iter, id_exists] = id_set.emplace(id_optional.value());
			if(!id_exists)
			{
				// ID Has been inserted, haven't seen before, so initiate download
				fmt::print("id: {}\n", id_optional.value());
				// TODO: download(id);
				// Then of course the printing aspect of it
				// Will printing happen on a different thread?
			}
		}

		std::this_thread::sleep_for(420ms);
	}

	return 0;
}
