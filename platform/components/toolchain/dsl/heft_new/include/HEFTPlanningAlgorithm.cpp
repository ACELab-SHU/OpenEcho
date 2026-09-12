#include "HEFTPlanningAlgorithm.hpp"
#include <algorithm>
#include <limits>


double HEFTPlanningAlgorithm::calculateAverageBandwidth() {
    std::cout << "HEFT calculateAverageBandwidth\n";
    double avg = 0.0;
    for (const auto& tile : tiles) {
        avg += tile.computationCapacity;
    }
    return avg / tiles.size();
}
void HEFTPlanningAlgorithm::calculateComputationCosts(const std::vector<Task>& tasks, const std::vector<Tile>& tiles) {
    std::cout << "HEFT calculateComputationCosts\n";
    for (const auto& task : tasks) {
        std::map<int, double> costsTile;
        for (const auto& tile : tiles) {
            if (tile.computationCapacity < task.computationCost) {
                costsTile[tile.tileId] = std::numeric_limits<double>::infinity();
            } else {
                costsTile[tile.tileId] = task.computationCost / tile.computationCapacity;
            }
        }
        computationCosts[task.taskId] = costsTile;
    }


    for (const auto& task : tasks) {
        std::map<int, double> costsTile;
        for (const auto& tile : tiles) {
            double computationCost = computationCosts[task.taskId][tile.tileId];
            // std::cout << "任务 " << task.taskId << " 在 TILE " << tile.tileId << " 上的用时：" << computationCost << "\n";
        }
        
    }
}
void HEFTPlanningAlgorithm::calculateTransferCosts(const std::vector<Task>& tasks) {
    // std::cout << "HEFT calculateTransferCosts\n";
    for (const auto& parent : tasks) {
        std::map<int, double> taskTransferCosts;
        for (const auto& child : tasks) {
            taskTransferCosts[child.taskId] = calculateTransferCost(parent, child);
        }
        transferCosts[parent.taskId] = taskTransferCosts;
    }
}
double HEFTPlanningAlgorithm::calculateTransferCost(const Task& parent, const Task& child) {
    // std::cout << "HEFT calculateTransferCost\n";
    double acc = 0.0;
    for (const auto& parentFile : parent.childTasks) {
        if (std::get<0>(parentFile) != -1) {
            for (const auto& childFile : child.parentTasks) {
                if (std::get<0>(childFile)!= -1 && std::get<0>(parentFile) == std::get<0>(childFile)) {
                    acc += 1.0; // 假设传输成本是常量
                    break;
                }
            }
        }
    }
    return acc;
}

bool HEFTPlanningAlgorithm::compareRank(const std::pair<int, double>& a, const std::pair<int, double>& b) {
    return a.second > b.second;
}

void HEFTPlanningAlgorithm::calculateRanks(const std::vector<Task>& tasks) {
    // std::cout << "HEFT calculateRanks\n";
    std::map<int, std::vector<int>> graph; // 父任务到子任务的关系
    std::map<int, int> inDegree;           // 任务的入度
    std::map<int, int> ranks;              // 任务的 rank

    for (const auto& task : tasks) {
        for (const auto& parentTasks : task.parentTasks) {
            int parentID = std::get<0>(parentTasks);
            int childID = task.taskId;
            graph[parentID].push_back(childID);
            inDegree[childID]++;
            if (inDegree.find(parentID) == inDegree.end()) {
                inDegree[parentID] = 0; // 父任务可能没有被子任务引用
            }
        }

    std::queue<int> q;
    for (const auto& [taskID, degree] : inDegree) {
        if (degree == 0) {
            q.push(taskID);
            ranks[taskID] = inDegree.size(); // 根节点 rank 最大
        }
    }

    // 拓扑排序处理，更新 ranks
    while (!q.empty()) {
        int current = q.front();
        q.pop();
        int currentRank = ranks[current];

        for (const auto& child : graph[current]) {
            inDegree[child]--;
            if (inDegree[child] == 0) {
                q.push(child);
                ranks[child] = currentRank - 1; // 子任务 rank 比父任务低
            }
        }
    }

    // 输出结果
    for (const auto& [taskID, rank] : ranks) {
        std::cout << "Task ID: " << taskID << ", Rank: " << rank << std::endl;
    }

    }

    for (const auto& task : tasks) {
        epsilon = ranks[task.taskId];
        
        calculateRank(task);
    }
    // std::cout << "okkk1\n";
    rankVector.assign(rank.begin(), rank.end());
    std::sort(rankVector.begin(), rankVector.end(), compareRank);
    std::map<int, double> rankMap;

    // 遍历 rankVector，并将每个元素插入到 rankMap 中
    for (const auto& pair : rankVector) {
        rankMap[pair.first] = pair.second;
    }
}


double HEFTPlanningAlgorithm::calculateRank(const Task& task) {
    if (rank.count(task.taskId) > 0) {
        return rank[task.taskId];
    }

    // 检查任务是否已经在当前计算中，如果是则返回一个合理的值，防止无限递归
    if (currentlyCalculating.count(task.taskId) > 0) {
        return 0.0;  // 或者使用其他适当的默认值
    }

    // 将任务添加到当前计算中
    currentlyCalculating.insert(task.taskId);
    // std::cout << "okkk2\n";
    double averageComputationCost = 0.0;
    for (const auto& cost : computationCosts[task.taskId]) {
        if (cost.second != std::numeric_limits<double>::infinity()) {
            averageComputationCost += cost.second;
        }
    }

    // std::cout << "-------- " << "\n";
    averageComputationCost /= computationCosts[task.taskId].size();
    // std::cout << "okkk3\n";
    double maxChildCost = 0.0;

    for (const auto& childId : task.childTasks) {
        // std::cout << "task " << task.taskId << "childId " << childId << "\n";
        if (std::get<0>(childId)!=-1) {  // 检查 tasks 是否包含 childId
            double childCost = transferCosts[task.taskId][std::get<0>(childId)] + calculateRank(tasks[std::get<0>(childId)]) + epsilon;
            maxChildCost = std::max(maxChildCost, childCost);
        }
    }
    
    // 从当前计算集合中移除任务
    currentlyCalculating.erase(task.taskId);

    rank[task.taskId] = averageComputationCost + maxChildCost;
    return rank[task.taskId];
}

void HEFTPlanningAlgorithm::allocateTasks(const std::vector<Task>& tasks, const std::vector<Tile>& tiles) {
    // std::cout << "HEFT allocateTasks\n";
    std::vector<Task> sortedTasks = tasks;
    std::sort(sortedTasks.begin(), sortedTasks.end(), [this](const Task& a, const Task& b) {
        // Custom sorting function
        return compareTasks(a, b);
    });
    for (const auto& task : sortedTasks) {
        allocateTask(task);
    }
}

bool HEFTPlanningAlgorithm::compareTasks(const Task& a, const Task& b)  {
    // Check if a has any unresolved dependencies
    for (const auto& dependency : a.parentTasks) {
        if (earliestFinishTimes.find(std::get<0>(dependency)) == earliestFinishTimes.end()) {
            // a has an unresolved dependency, so it should come after b
            return false;
        }
    }

    // Check if b has any unresolved dependencies
    for (const auto& dependency : b.parentTasks) {
        if (earliestFinishTimes.find(std::get<0>(dependency)) == earliestFinishTimes.end()) {
            // b has an unresolved dependency, so it should come after a
            return true;
        }
    }

    // Both tasks have resolved dependencies, compare their ranks
    return rank[a.taskId] > rank[b.taskId];
}


bool HEFTPlanningAlgorithm::checkTaskTileMatch(const Task& task, const Tile& tile) {
    // 获取任务和瓦片的属性
    return (task.spm_size == tile.spm_size &&
            task.num_lane == tile.num_lane &&
            task.has_bitalu == tile.has_bitalu &&
            task.has_serdiv == tile.has_serdiv &&
            task.has_complexunit == tile.has_complexunit);
}

void HEFTPlanningAlgorithm::allocateTask(const Task& task) {
    std::cout << "HEFT allocateTask\n";

    if (task.is_spmd == 1) {
        allocateSpmdTask(task);
        return;
    }

    Tile chosenTile;
    double earliestFinishTime = std::numeric_limits<double>::infinity();
    double bestReadyTime = 0.0;
    double finishTime;

    for (const auto& tile : tiles) {
        // 检查任务和瓦片的属性是否匹配
        if (checkTaskTileMatch(task, tile)) {
            double minReadyTime = 0.0;
            for (const auto& parent : task.parentTasks) {
                double readyTime = earliestFinishTimes[std::get<0>(parent)];
                if (std::get<0>(parent) != task.taskId) {
                    readyTime += transferCosts[std::get<0>(parent)][task.taskId];
                }
                minReadyTime = std::max(minReadyTime, readyTime);
            }
            finishTime = findFinishTime(task, tile, minReadyTime, false);
            if (finishTime < earliestFinishTime) {
                bestReadyTime = minReadyTime;
                earliestFinishTime = finishTime;
                chosenTile = tile;
            }
        }
    }

    findFinishTime(task, chosenTile, bestReadyTime, true);
    earliestFinishTimes[task.taskId] = earliestFinishTime;
    // std::cout << "任务 " << task.taskId << " 分配给 TILE " << chosenTile.tileId << "，最早完成时间：" << earliestFinishTime << "\n";
}

void HEFTPlanningAlgorithm::allocateSpmdTask(const Task& task) {
    std::cout << "HEFT allocateSpmdTask for task " << task.taskId << " requiring " << task.min_core_num << " cores\n";

    if (task.min_core_num > tiles.size()) {
        std::cerr << "ERROR: Task " << task.taskId << " requires " << task.min_core_num
                  << " cores but only " << tiles.size() << " tiles available\n";
        throw std::runtime_error("Insufficient tiles for SPMD task");
    }

    double minReadyTime = 0.0;
    for (const auto& parent : task.parentTasks) {
        double readyTime = earliestFinishTimes[std::get<0>(parent)];
        if (std::get<0>(parent) != task.taskId) {
            readyTime += transferCosts[std::get<0>(parent)][task.taskId];
        }
        minReadyTime = std::max(minReadyTime, readyTime);
    }

    earliestFinishTimes[task.taskId] = minReadyTime + computationCosts[task.taskId][tiles[0].tileId];
}

double HEFTPlanningAlgorithm::findFinishTime(const Task& task, const Tile& tile, double readyTime, bool occupySlot) {
    std::cout << "HEFT findFinishTime\n";
    std::vector<Event>& sched = schedules[tile.tileId];
    double computationCost = computationCosts[task.taskId][tile.tileId];
    double start, finish;
    int pos;
    if (sched.empty()) {
        if (occupySlot) {
            sched.push_back({task.taskId, tile.tileId, readyTime, readyTime + computationCost, -1});
        }
        return readyTime + computationCost;
    }
    if (sched.size() == 1) {
        if (readyTime >= sched[0].finish) {
            pos = 1;
            start = readyTime;
        } else if (readyTime + computationCost <= sched[0].start) {
            pos = 0;
            start = readyTime;
        } else {
            pos = 1;
            start = sched[0].finish;
        }
        if (occupySlot) {
            sched.insert(sched.begin() + pos, {task.taskId, tile.tileId, start, start + computationCost, -1});
        }
        return start + computationCost;
    }
    start = std::max(readyTime, sched[sched.size() - 1].finish);
    finish = start + computationCost;
    int i = sched.size() - 1;
    int j = sched.size() - 2;
    pos = i + 1;
    while (j >= 0) {
        Event current = sched[i];
        Event previous = sched[j];
        if (readyTime > previous.finish) {
            if (readyTime + computationCost <= current.start) {
                start = readyTime;
                finish = readyTime + computationCost;
                pos = i;
            }
            break;
        }
        if (previous.finish + computationCost <= current.start) {
            start = previous.finish;
            finish = previous.finish + computationCost;
            pos = i;
        }
        i--;
        j--;
    }
    if (readyTime + computationCost <= sched[0].start) {
        pos = 0;
        start = readyTime;
        if (occupySlot) {
            sched.insert(sched.begin() + pos, {task.taskId, tile.tileId, start, start + computationCost, -1});
        }
        return start + computationCost;
    }
    if (occupySlot) {
        sched.insert(sched.begin() + pos, {task.taskId, tile.tileId, start, finish, -1});
    }
    return finish;
}


HEFTPlanningAlgorithm::HEFTPlanningAlgorithm(const std::vector<Task>& taskList, const std::vector<Tile>& tileList)
    : tasks(taskList), tiles(tileList), averageBandwidth(calculateAverageBandwidth()) {
    // std::cout << "HEFT HEFTPlanningAlgorithm\n";
    for (const auto& tile : tiles) {
        schedules[tile.tileId] = std::vector<Event>();
    }
}

void HEFTPlanningAlgorithm::run() {
    // std::cout << "HEFT planner running\n";
    calculateComputationCosts(tasks, tiles);
    calculateTransferCosts(tasks);
    calculateRanks(tasks);
}
const std::vector<Task>& HEFTPlanningAlgorithm::getTasks() const {
    return tasks;
}

const std::vector<std::pair<int, double>>& HEFTPlanningAlgorithm::getRanks() const {
    return rankVector;
}

const std::vector<Tile>& HEFTPlanningAlgorithm::getTILEs() const {
    return tiles;
}
const std::map<int, std::vector<Event>>& HEFTPlanningAlgorithm::getSchedules() const {
    return schedules;
}
