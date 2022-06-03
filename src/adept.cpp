/*
 * abuild.cpp
 *
 * A simple cmdline utility for building Android APKs.
 */

#include "adept.h"

// Instantiation of static members
adept* adept::singleton = 0;

const std::string adept::Strings::packageFormat = "Expected format: <package path>:<package version>, ie com.google.android.material:1.4.0";
const std::string adept::Strings::invalidPackageName = "Invalid package name: ";
const std::string adept::Strings::helpIntro = "\nadept\n\nA simple command line utility for managing Android dependencies.\n\n"
    "Usage: adept [options] <packages>, where each package is formatted as <path>:<version>.\n"
    "For example, to fetch com.google.android.material version 1.4.0, use: adept com.google.android.material:1.4.0\n\n"
    "Available options:";
const std::string adept::Strings::msgFetch = "Attempting to fetch file ";
const std::string adept::Strings::msgFetchFail = "Failed to fetch file ";
const std::string adept::Strings::msgRedirect = "Redirecting to ";
const std::string adept::Strings::msgSuccess = "Success!";
const std::string adept::Strings::msgHttpCode = "Server returned code ";

/*
 * Initializes the adept singleton and returns it.
 * If singleton is non-null, then we simply return the existing object.
 */
adept* adept::init(void)
{
    if (!singleton)
	singleton = new adept;
    return singleton;
}

/*
 * Deinitializes the adept singleton, if it has been initialized.
 */
void adept::deinit(void)
{
    if (singleton)
	delete singleton;
    singleton = 0;

    return;
}

/*
 * adept constructor
 * This is where new command line options should be added and the help text should be updated.
 */
adept::adept(void) :
    optHelp("-h", "--help", 0, "Displays this help menu."),
    optOutDir("-d", "--out-dir", 1, "Specifies the directory to which the fetched libraries will be written.", {"."}, {"path"}),
    optFetchDeps("-D", "--deps", 1, "If this option is specified, all subdependencies will also be fetched.", {"."}, {"path"}),
    optRepo("-r", "--repo", 1, "Specifies the url of the repository to download the libraries from. Current default: %0", {"maven.google.com"}, {"url"}),
    optForce("-f", "--force", 0, "Forces the action to complete, overwriting existing files even if they are up-to-date."),
    optIndex("-i", "--index", 1, "Specifies an alternate master index file to search for in the repository. Current default: %0", {"master-index.xml"}, {"index"}),
    curlHandle(0),
    fileBuffer(),
    headerBuffer()
{
    // Initialize cURL
    curl_global_init(CURL_GLOBAL_ALL);
    curlHandle = curl_easy_init();

    // Set our write callback and set up the pointers to the buffers.
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, &adept::curlWriteData);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &fileBuffer);
    curl_easy_setopt(curlHandle, CURLOPT_HEADERDATA, &headerBuffer);

    return;
}

/*
 * Destructor
 *
 * Performs some cURL cleanup.
 */
adept::~adept(void)
{
    curl_easy_cleanup(curlHandle);
    curl_global_cleanup();

    return;
}

/*
 * cURL write callback - writes data to curlBuffer.
 */
size_t adept::curlWriteData(void* buffer, size_t size, size_t n, void* userp)
{
    size_t nBytes = size * n;
    auto dst = reinterpret_cast<std::vector<char>*>(userp);

    // Reserve more buffer space, if necessary - this will probably use more memory than we actually need, but will reduce reallocations.
    if (dst->capacity() < dst->size() + nBytes)
    {
	dst->reserve(dst->capacity() + CURL_MAX_WRITE_SIZE);
    }

    // Expand to be able to fit the additional bytes and then copy them over.
    int offset = dst->size();
    dst->resize(offset + nBytes);
    std::memcpy(dst->data() + offset, buffer, nBytes);

    return nBytes;
}

/*
 * Helper function to correctly set up and null-terminate the buffers
 * before / after a call to curl_easy_perform.
 */
CURLcode adept::curlPerform(const std::string& url)
{
    // Prepare the url and clear the buffers
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    fileBuffer.clear();
    headerBuffer.clear();

    CURLcode status = curl_easy_perform(curlHandle);

    // Null-terminate the buffers for convenience when using regex.
    // This shouldn't need a reallocation, since we reserve in big chunks
    fileBuffer.push_back('\0');
    headerBuffer.push_back('\0');

    return status;
}

/*
 * Attempts to fetch the file located at url.
 *
 * If successful, returns true.
 * If not, returns false and prints an error to stderr.
 */
bool adept::fetchFile(std::string url)
{
    // Try to download
    std::cout << Strings::msgFetch << url << std::endl;
    while (true)
    {
	CURLcode status = curlPerform(url);
    
	switch (status)
	{
	case CURLE_OK:
	    break;
	default:
	    std::cout << Strings::msgFetchFail << url << std::endl;
	    return false;
	}

	// Connection was successful
	int httpCode = getHttpCode();

	if (httpCode == 200)
	{
	    std:: cout << Strings::msgSuccess;
	    return true;
	}
	else if (httpCode >= 300 && httpCode < 400) // Codes representing a redirect
	{
	    url = getRedirectUrl();
	    if (url.empty())
	    {
		return false;
	    }
	    std::cout << Strings::msgRedirect << url << std::endl;
	}
	else // Other return code that we don't know what to do with
	{
	    std::cout << Strings::msgFetchFail << url << ". " << Strings::msgHttpCode << httpCode << std::endl;
	    return false;
	}
    }
}

/*
 * Extracts the HTTP return code from the header buffer.
 * Assumes the buffer is a null-terminated C string, which
 * is the case if curlPerform is called.
 *
 * Returns the HTTP code as an integer (ie, 404, 301, 200, etc.) or 0 if no HTTP code was found.
 */
int adept::getHttpCode(void) const
{
    std::cmatch matches;
    if (!std::regex_search(headerBuffer.data(), matches, httpCodeRegex))
	return 0;

    // Index 2 will contain the HTTP code.
    std::string httpCode = std::string(matches[httpCodeIndex].first, matches[httpCodeIndex].second);

    return std::stoi(httpCode);
}

/*
 * Finds the Location tag for HTTP redirects.
 *
 * Returns the redirect url, or an empty string if not found.
 */
std::string adept::getRedirectUrl(void) const
{
    std::cmatch matches;
    if (!std::regex_search(headerBuffer.data(), matches, httpLocationRegex))
	return std::string();

    // Index 1 will contain the url
    return std::string(matches[httpLocationIndex].first, matches[httpLocationIndex].second);
}

/*
 * run
 */
int adept::run(int argc, char* argsv[])
{
    // If no arguments have been passed, just display the help text.
    if (argc <= 1)
    {
	help();
	return ReturnCode::ok;
    }

    // At least one argument. argsv[0] is just the command, which we don't care about.
    argc--;
    argsv++;

    // Begin by converting the arguments array into a list.
    std::list<std::string> argsList = std::list<std::string>();
    for (int i = 0; i < argc; i++)
    {
	argsList.emplace_back(argsv[i]);
    }

    // Now parse the command line arguments to find any options
    for (auto opt : opts)
    {
	opt->parse(argsList);
    }

    // If no arguments were passed or -h was passed (anywhere), display the help text
    if (optHelp.isSet())
    {
	help();
	return ReturnCode::ok;
    }

    // Any remaining arguments are the packages to download
    for (auto arg : argsList)
    {
	std::cmatch matches;
	if (!std::regex_search(arg.c_str(), matches, packageNameVersionRegex))
	{
	    // Invalid package name passed
	    std::cout << Strings::invalidPackageName << arg << std::endl << Strings::packageFormat << std::endl;
	    return ReturnCode::invalidPackage;
	}
	packages.emplace_back(std::make_pair(std::string(matches[packageNameIndex].first, matches[packageNameIndex].second),
					     std::string(matches[packageVersionIndex].first, matches[packageVersionIndex].second)));
    }

    // Begin by downloading the master package list from the repo and check
    fetchFile(optRepo[0] + "/" + optIndex[0]);
    tinyxml2::XMLDocument index;
    index.Parse(fileBuffer.data(), fileBuffer.size());
    index.Print();
    return 0;
}

/*
 * Outputs the help text to stdout, then returns.
 */
void adept::help(void) const
{
    std::cout << Strings::helpIntro << std::endl;
    for (auto opt : opts)
    {
	std::cout << opt->getFormattedHelpText(0, 25, 100) << std::endl;
    }
    return;
}
