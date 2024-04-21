#include <iostream>

#include "simdjson.h"

using namespace simdjson;

int main(void) {
    ondemand::parser parser;
    padded_string json        = padded_string::load("twitter.json");
    ondemand::document tweets = parser.iterate(json);
    std::cout << std::string_view(tweets["k"]["o"]);
}