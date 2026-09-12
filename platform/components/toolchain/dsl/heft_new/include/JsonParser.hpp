#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <string>
#include <vector>
#include "HEFTPlanningAlgorithm.hpp"

class JsonParser {
public:
    static std::vector<inputTask> parseJson(const std::string& filename);
};

#endif // JSONPARSER_H