#ifndef TASKCONVERTER_H
#define TASKCONVERTER_H

#include <vector>
#include <unordered_map>
#include <utility>
#include "HEFTPlanningAlgorithm.hpp"

class TaskConverter {
public:
    static std::pair<std::vector<Task>, std::unordered_map<std::string, int>> convertToTasks(const std::vector<inputTask>& inputTasks);
};

#endif // TASKCONVERTER_H