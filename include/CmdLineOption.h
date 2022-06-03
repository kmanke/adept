/*
 * Simple class for processing command line flags / options.
 */

#include <vector>
#include <list>
#include <string>
#include <stdexcept>
#include <iostream>

class CmdLineOption
{
public:
    CmdLineOption(const std::string& shortLabel, const std::string& longLabel, int nArgs, const std::string& helpText,
		  std::vector<std::string> defaults= std::vector<std::string>(), std::vector<std::string> argLabels = std::vector<std::string>())
	: shortLabel(shortLabel), longLabel(longLabel), nArgs(nArgs), args(defaults), argLabels(argLabels), helpText(helpText), set(false) { }

    bool parse(std::list<std::string>& argsList);
    inline bool isSet(void) const { return set; }
    inline const std::vector<std::string>& getArgs(void) const { return args; }
    std::string getFormattedHelpText(int startIndent, int helpTextIndent, int wrapCol) const;

    // Operator overloads
    std::string& operator[](size_t i);

    // Constants
    static const char helpTextPlaceholder;
    
private:
    std::string shortLabel;
    std::string longLabel;
    int nArgs;
    std::vector<std::string> args;
    std::vector<std::string> argLabels; // labels used in helptext, ie -d <argLabel>
    std::string helpText;
    bool set;

    std::list<std::string> preprocessHelpText(void) const;
    int findWords(const std::string& input, std::list<std::string>& words) const;
    void replacePlaceholders(std::list<std::string>& words, const std::vector<std::string>& replacements) const;
    inline int indent(std::string& formatted, int indentAmount) const;
};
