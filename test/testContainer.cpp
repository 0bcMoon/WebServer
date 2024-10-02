#include <iostream>
#include <cctype>
#include <iomanip>
#include <sstream>

std::string urlEncode(const std::string& str) {
    std::ostringstream encodedStream;
    encodedStream << std::hex << std::uppercase << std::setfill('0');

    for (char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encodedStream << c;
        } else {
            encodedStream << '%' << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(c));
        }
    }

    return encodedStream.str();
}


int main() {

    std::string originalString = "";
	auto str = originalString.substr(1);
	if ( str.empty())
		std::cout << "str is empty" << std::endl;
	else
		std::cout << "str is not empty" << std::endl;
			
    // std::string encodedString = urlEncode(originalString);

    // std::cout << "Original string: " << originalString << std::endl;
    // std::cout << "Encoded string: " << encodedString << std::endl;

    return 0;
}


