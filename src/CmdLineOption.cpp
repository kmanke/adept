/*
 * Simple class for processing command line flags / options.
 */

#include "CmdLineOption.h"

// Static member initialization
const char CmdLineOption::helpTextPlaceholder = '%';

/*
 * Parses the passed list of command line arguments. If it finds a match, then set will be set to true and the
 * next nArgs arguments will be extracted from argsList and stored in args.
 *
 * Returns:
 *    value of set after the function call.
 */
bool CmdLineOption::parse(std::list<std::string>& argsList)
{
    for (auto it = argsList.begin(); it != argsList.end(); ++it)
    {
	// Check for a match
	if (*it == shortLabel || *it == longLabel)
	{
	    it = argsList.erase(it);
	    set = true;
	    args.clear(); // Erase the defaults
	    for (int i = 0; i < nArgs; ++i)
	    {
		if (it == argsList.end())
		{
		    throw std::invalid_argument("Unexpected end of arguments list.");
		}
		args.push_back(std::move(*it));
		it = argsList.erase(it);
	    }
	}
    }
    return set;
}

/*
 * Overload of the arry subscript operator.
 *
 * Returns the ith element in argsList. If i > argsList.size(), throws std::out_of_range.
 */
std::string& CmdLineOption::operator[](size_t i)
{
    if (i > args.size())
    {
	throw std::out_of_range("Attempt to access element " + std::to_string(i) + " of option " + shortLabel + ", args.size() = " + std::to_string(args.size()));
    }

    return args[i];    
}

/*
 * Prints formatted help text for the option.
 *
 * Parameters:
 *   startindent - The number of ' ' characters prepended to the front of the string.
 *   helpTextIndent - The indentation, relative to startIndent, of the actual help text.
 *   wrapLength - The length at which the text will be wrapped. Indentation will be preserved.
 *
 * The wrap will occur on a whitespace character only, unless the word is too long to fit on a single line.
 */
std::string CmdLineOption::getFormattedHelpText(int startIndent, int helpTextIndent, int wrapLength) const
{
    std::string formatted = std::string(startIndent, ' ');

    // Tokenize the help text and then replace any placeholders
    std::list<std::string> words = preprocessHelpText();
    if (shortLabel.length())
    {
	formatted += shortLabel;
	if (longLabel.length())
	{
	    formatted += ", " + longLabel;
	}
    }
    else if (longLabel.length())
    {
	formatted += longLabel;
    }

    for (auto label : argLabels)
    {
	formatted += " <" + label + ">";
    }

    // Append one more space to make sure there's separation
    formatted.append(1, ' ');

    // Now print the help text, taking care to properly indent
    // Initial indentation
    int currentCol;
    if (formatted.length() < startIndent + helpTextIndent)
    {
	formatted.append(startIndent + helpTextIndent - formatted.length(), ' ');
    } 
    currentCol = formatted.length();
    while(!words.empty())
    {
	std::string& word = words.front();
	if (currentCol >= wrapLength)
	{
	    currentCol = indent(formatted, startIndent + helpTextIndent);
	}
	else if (word.length() > wrapLength - startIndent - helpTextIndent) // Long word, print as much as possible
	{
	    formatted.append(word.substr(0, wrapLength - currentCol));
	    word.erase(0, wrapLength - currentCol);
	    currentCol = indent(formatted, startIndent + helpTextIndent);
	}
	else if (word.length() < wrapLength - currentCol)
	{
	    currentCol += word.length() + 1;
	    formatted.append(word);
	    formatted.append(1, ' ');
	    words.pop_front();
	}
	else
	{
	    currentCol = indent(formatted, startIndent + helpTextIndent);
	}
    }

    return formatted;
}

/*
 * Preprocesses the helpText string.
 *
 * First, any "%i" will be replaced with the ith argument.
 * Then, it breaks the string into single words up to the specified max.
 */
std::list<std::string> CmdLineOption::preprocessHelpText(void) const
{
    std::list<std::string> words;
    findWords(helpText, words);
    replacePlaceholders(words, args);
    return words;
}

/*
 * Finds all words (whitespace delimited strings) in the specified string.
 *
 * Arguments:
 *    input - The input string.
 *    list - List in which to store the words
 *
 * Returns:
 *    the number of words found
 */
int CmdLineOption::findWords(const std::string& input, std::list<std::string>& words) const
{
    auto startIt = input.begin();
    auto endIt = startIt;
    int nWords = 0;

    while (startIt != input.end())
    {
	// Advance to the next non-whitespace character
	while (std::isspace(*startIt))
	{
	    if (++startIt == input.end())
	    {
	        return nWords;
	    }
	}
	// Advance the end iterator to the next whitespace character
	endIt = startIt;
	do
	{
	    if (++endIt == input.end())
	    {
		words.emplace_back(startIt, endIt);
		return ++nWords;
	    }
	} while (!std::isspace(*endIt));

	// Add the word to the list
	words.emplace_back(startIt, endIt);
	nWords++;
	startIt = endIt;
    }
    
    return nWords;
}

/*
 * For each placeholder %i in the list, replace it with the ith entry from the specified list.
 *
 * The modified item will be treated as a single word for wrapping purposes.
 */
void CmdLineOption::replacePlaceholders(std::list<std::string>& words, const std::vector<std::string>& replacements) const
{
    // Input validation
    if (replacements.empty() || words.empty())
	return;
    
    // Determine the maximum possible number of digits we should search for
    int nDigits = 0;
    for (int size = replacements.size(); size > 0; size /= 10, nDigits++);
    
    for (auto it = words.begin(); it != words.end();)
    {
	std::string& word = *it;
	auto index = word.find(helpTextPlaceholder);
	if (index == std::string::npos)
	{
	    ++it;
	    continue; // No more placeholders in this word
	}
	auto placeholderStart = word.begin() + index;
	auto placeholderEnd = placeholderStart + 1;
	
	char digits[nDigits + 1];
	int n = 0;
	
	while (n < nDigits)
	{
	    if (placeholderEnd == word.end())
	    {
		break;
	    }
	    else if (std::isdigit(*placeholderEnd))
	    {
		digits[n] = *placeholderEnd;
		++n;
		++placeholderEnd;
	    }
	    else
	    {
		break; // Not a digit
	    }
	}
	
	digits[n] = 0; // Null terminate
	if (n == 0)
	{
	    // Replace with nothing
	    word.replace(placeholderStart, placeholderEnd, std::string());
	    continue;
	}
	
	unsigned int replacementIndex = std::atoi(digits);
	if (replacementIndex > replacements.size())
	{
	    // Replace with nothing
	    word.replace(placeholderStart, placeholderEnd, std::string());
	    continue;
	}

	// Replace the placeholder with the replacement string
	word.replace(placeholderStart, placeholderEnd, replacements[replacementIndex]);
    }

    return;
}

/*
 * Helper function to perform indentation on the formatted help text string.
 */
inline int CmdLineOption::indent(std::string& formatted, int indentAmount) const
{
    formatted.append(1, '\n');
    formatted.append(indentAmount, ' ');

    return indentAmount;
}
