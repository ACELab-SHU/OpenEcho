#ifndef HEFT_PLANNING_ALGORITHM_H
#define HEFT_PLANNING_ALGORITHM_H

#include <iostream>
#include <vector>
#include <map>
#include <unordered_set>
#include <queue>
#include <functional> 
#include <unordered_map>
struct Task {
    int taskId;
    double computationCost;
    std::vector<std::tuple<int, int, std::string, int, std::string, std::string, std::string, bool, std::string>> parentTasks; // ... is_pointer, dsl_type_hint from JSON (may be empty)
    std::vector<std::tuple<int, int, std::string, int, std::string, std::string, std::string>> childTasks;
    int spm_size;
    int num_lane;
    bool has_bitalu;
    bool has_serdiv;
    bool has_complexunit;
    std::string address;
    int length;
    std::vector<std::pair<std::string, std::string>> global_Input;
    std::vector<std::tuple<std::string, std::string, std::string, std::string, bool, std::string>> para_Input;
    std::vector<std::pair<std::string, int>> return_output;
    std::vector<std::tuple<std::string, std::string, int, int>> all_output; // Name, parentTasksPort, temp_offset, length
    std::unordered_map<std::string, int> temp_size_map; // 临时变量名 -> size_bytes，仅用于 pointer_length 查找
    int text_offset;
    int data_offset;
    int total_length;
    int text_length;
    int data_length;
    int output_num;
    std::string hardwareinfo;
    std::string hash;
    int is_spmd;
    int min_core_num;

};

struct Tile {
    int tileId;
    double computationCapacity;
    int spm_size;
    int num_lane;
    bool has_bitalu;
    bool has_serdiv;
    bool has_complexunit;
};

struct inputTask {
    std::string taskId;
    double computationCost;
    std::vector<std::tuple<std::string, int, std::string, int, std::string, std::string, std::string, bool, std::string>> parentTasks;
    std::vector<std::tuple<std::string, int, std::string, int, std::string, std::string, std::string>> childTasks;
    int spm_size;
    int num_lane;
    bool has_bitalu;
    bool has_serdiv;
    bool has_complexunit;
    std::string address;
    int length;
    std::vector<std::pair<std::string, std::string>> global_Input;
    std::vector<std::tuple<std::string, std::string, std::string, std::string, bool, std::string>> para_Input;
    std::vector<std::pair<std::string, int>> return_output;
    std::vector<std::tuple<std::string, std::string, int, int>> all_output; // Name, parentTasksPort, temp_offset, length
    std::unordered_map<std::string, int> temp_size_map; // 临时变量名 -> size_bytes，仅用于 pointer_length 查找，不写入输出 JSON
    int text_offset;
    int data_offset;
    int total_length;
    int text_length;
    int data_length;
    int output_num;
    std::string hardwareinfo;
    std::string hash;
    int is_spmd;
    int min_core_num;
};

struct Event {
    int taskId;
    int tileId;
    double start;
    double finish;
    int spmd_master_id;     // 非SPMD任务为-1，SPMD任务组共享一个master ID
};

class HEFTPlanningAlgorithm {
private:
    std::vector<Task> tasks;
    std::vector<Tile> tiles;
    std::vector<std::pair<int, double>> rankVector;
    std::unordered_set<int> currentlyCalculating;
    std::map<int, std::map<int, double>> computationCosts;
    std::map<int, std::map<int, double>> transferCosts;
    std::map<int, double> rank;
    std::map<int, double> earliestFinishTimes;
    std::map<int, std::vector<Event>> schedules;
    double epsilon = 1e-6;
    double averageBandwidth;

    double calculateAverageBandwidth();

    bool isChildTask(const Task& taskA, const Task& taskB) ;   

    static bool compareRank(const std::pair<int, double>& a, const std::pair<int, double>& b);

    void calculateComputationCosts(const std::vector<Task>& tasks, const std::vector<Tile>& tiles);

    void calculateTransferCosts(const std::vector<Task>& tasks);

    double calculateTransferCost(const Task& parent, const Task& child);

    void calculateRanks(const std::vector<Task>& tasks);

    double calculateRank(const Task& task);
    
    bool checkTaskTileMatch(const Task& task, const Tile& tile);

    void allocateTasks(const std::vector<Task>& tasks, const std::vector<Tile>& tiles);

    bool compareTasks(const Task& a, const Task& b);

    void allocateTask(const Task& task);

    void allocateSpmdTask(const Task& task);

    double findFinishTime(const Task& task, const Tile& tile, double readyTime, bool occupySlot);  

public:
    HEFTPlanningAlgorithm(const std::vector<Task>& taskList, const std::vector<Tile>& tileList);

    void run();

    const std::vector<Task>& getTasks() const;

    const std::vector<Tile>& getTILEs() const;

    const std::vector<std::pair<int, double>>& getRanks() const ;

    const std::map<int, std::vector<Event>>& getSchedules() const;
};

#endif // HEFT_PLANNING_ALGORITHM_H
