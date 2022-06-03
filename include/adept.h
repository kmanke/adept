/*
 * abuild.h
 *
 * A simple cmdline utility for building Android APKs using the Android build-tools.
 *
 * Copyright (C) 2022 Kyle Manke
 */

#include <string>
#include <cstring>
#include <iostream>
#include <vector>
#include "CmdLineOption.h"
#include <curl/curl.h>
#include <regex>
#include <tinyxml2.h>

typedef std::pair<std::string, std::string> PackageDescriptor;
typedef std::vector<PackageDescriptor> PackageList;

class adept
{
public:
    static adept* init(void);
    static void deinit(void);

    int run(int argc, char* argsv[]);

    enum ReturnCode : int
    {
	ok = 0,
	invalidPackage = -1,
	packageNotFound = -2    
    };

private:
    adept(void);
    ~adept(void);
    static adept* singleton;

    // Internal functions
    void help(void) const;
    CURLcode curlPerform(const std::string& url);
    bool fetchFile(std::string url);
    int getHttpCode(void) const;
    std::string getRedirectUrl(void) const;
    
    // Repository settings
    const std::string defaultRepo = "maven.google.com";
    const std::string repoMasterIndex = "master-index.xml";

    // Command line options
    const std::vector<CmdLineOption*> opts{&optHelp, &optOutDir, &optFetchDeps, &optRepo, &optForce, &optIndex};

    CmdLineOption optHelp;
    CmdLineOption optOutDir;
    CmdLineOption optFetchDeps;
    CmdLineOption optRepo;
    CmdLineOption optForce;
    CmdLineOption optIndex;

    // A list of packages to download
    PackageList packages;

    // cURL stuff
    CURL* curlHandle;
    std::vector<char> fileBuffer;
    std::vector<char> headerBuffer;
    static size_t curlWriteData(void* buffer, size_t size, size_t n, void* userp);

    // Regexes for processing HTTP headers, extracting version numbers, etc.
    const std::regex httpCodeRegex = std::regex("HTTP\\/(\\d+\\.*\\d*)\\s*(\\d{3})");
    const int httpCodeIndex = 2;
    const std::regex httpLocationRegex = std::regex("Location:\\s*(\\S*)");
    const int httpLocationIndex = 1;
    const std::regex packageNameVersionRegex = std::regex("(\\S*):(\\S*)");
    const int packageNameIndex = 1;
    const int packageVersionIndex = 2;

    struct Strings
    {
	static const std::string invalidPackageName;
	static const std::string packageFormat;
	static const std::string helpIntro;
	static const std::string msgFetch;
	static const std::string msgFetchFail;
	static const std::string msgRedirect;
	static const std::string msgSuccess;
	static const std::string msgHttpCode;
    };
};
