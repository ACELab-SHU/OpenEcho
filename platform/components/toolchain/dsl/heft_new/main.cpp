#include "./include/HEFTPlanningAlgorithm.hpp"
#include "./include/JsonParser.hpp"
#include "./include/JsonWriter.hpp"
#include "./include/TaskConverter.hpp"
#include "./include/InputTile.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <bitset>
#include <random>
#include <utility>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 支持 0b/0x/十进制 前缀的字符串转整数（用 unsigned long 避免地址值溢出）
static int parse_int(const std::string& s) {
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        return (int)std::stoul(s.substr(2), nullptr, 2);
    }
    return (int)std::stoul(s, nullptr, 0);
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];

    std::vector<inputTask> inputtasks = JsonParser::parseJson(inputFile);
    std::vector<Tile> tiles = InputTile::setupTiles();

    //Developer can change their own schedule algoithm
    
    auto result = TaskConverter::convertToTasks(inputtasks);

    std::vector<Task> tasks = result.first;
    std::unordered_map<std::string, int> idMapping = result.second;

    HEFTPlanningAlgorithm heftPlanner(tasks, tiles);
    heftPlanner.run();

    std::vector<std::pair<int, double>> rankkk = heftPlanner.getRanks();

    std::vector<std::pair<std::string, double>> mappedTaskData;
    std::string mappedTaskId;

    for (const auto &entry : rankkk)
    {
        for (const auto &pair : idMapping)
        {
            if (pair.second == entry.first)
            {
                mappedTaskId = pair.first;
                mappedTaskData.push_back({mappedTaskId, entry.second});
                break;
            }
        }
    }

    std::vector<inputTask> output = inputtasks;
    std::unordered_map<std::string, int> sequentialMapping;
    int sequentialCounter = 0;

    for (const auto &data : mappedTaskData) {
        sequentialMapping[data.first] = sequentialCounter++;
    }

    for (auto &task : output) {
        task.taskId = std::to_string(sequentialMapping[task.taskId]);

        for (auto &parentTaskId : task.parentTasks) {
            if (std::get<0>(parentTaskId) != "-1") {
                std::get<0>(parentTaskId) = std::to_string(sequentialMapping[std::get<0>(parentTaskId)]);
            }
        }
        for (auto &childTaskId : task.childTasks) {
            if (std::get<0>(childTaskId) != "-1") {
                std::get<0>(childTaskId) = std::to_string(sequentialMapping[std::get<0>(childTaskId)]);    
            }
        }
    }
  
    std::ofstream outputFileStream(outputFile, std::ios::out);

    if (!outputFileStream.is_open()) {
        std::cerr << "Error opening the output file: " << outputFile << std::endl;
        return 1;
    }

    json outputJson;
    json returnJson;
    json returnJson_info;
    
    for (int count=0; count < sequentialCounter ; count++)
    {   
        json taskJson;                       
        json parentTasksJson;
    
        for (const auto& task : output) {
            if (task.taskId == std::to_string(count)) {
                auto it = std::find_if(sequentialMapping.begin(), sequentialMapping.end(),
                    [count](const std::pair<const std::string, int>& pair) { return pair.second == count; });
                if (it != sequentialMapping.end()) {
                    taskJson["debug_task_name"] = it->first;
                } 
                taskJson["current_taskId"] = count;
                taskJson["text_offset"]    = task.text_offset;
                taskJson["data_offset"]    = task.data_offset;
                taskJson["total_length"]   = task.total_length;
                taskJson["text_length"]    = task.text_length;
                taskJson["data_length"]    = task.data_length;
                taskJson["hardwareinfo"]   = task.hardwareinfo; // last 5 bits :spm_size lane_num has_serdiv has_complexunit has_bitalu
                taskJson["hash"]           = task.hash;
                taskJson["is_spmd"]        = task.is_spmd;
                taskJson["min_core_num"]   = task.min_core_num;
                taskJson["Input_Num"]      = task.parentTasks.size() + task.global_Input.size() + task.para_Input.size();
                taskJson["Output_Num"]     = task.output_num;

                for (size_t i = 0; i < task.global_Input.size(); ++i) {
                    auto global_Id = task.global_Input[i].first; 
                    auto global_addr = task.global_Input[i].second; 
                    JsonWriter::writeBinaryToJson_data_global(parentTasksJson, global_Id, global_addr, "0b001");
                }

                for (size_t i = 0; i < task.para_Input.size(); ++i) {
                    auto para_Id = std::get<0>(task.para_Input[i]);
                    auto para_addr = std::get<1>(task.para_Input[i]);
                    auto para_slice_length = std::get<2>(task.para_Input[i]);
                    auto para_slice_data_type = std::get<3>(task.para_Input[i]);
                    bool is_pointer = std::get<4>(task.para_Input[i]);
                    std::string para_dsl_type = std::get<5>(task.para_Input[i]);
                    auto decimal_dest_address = (para_addr == "null" || para_addr.empty()) ? 0 : parse_int(para_addr);
                    int slice_data_addr = parse_int(para_slice_length) * parse_int(para_slice_data_type);
                    int slice_data_dest = decimal_dest_address + slice_data_addr;
                    std::stringstream ss;
                    ss << std::hex << std::uppercase << slice_data_dest;
                    std::string hex_slice_data_dest_str = ss.str();
                    if (is_pointer) {
                        std::string ptr_t = para_dsl_type.empty() ? std::string("0b101") : para_dsl_type;
                        JsonWriter::writeBinaryToJson_pointer(parentTasksJson, para_Id, para_addr, para_slice_length, ptr_t, "0b0000000000");
                    } else {
                        std::string val_t = para_dsl_type.empty() ? std::string("0b010") : para_dsl_type;
                        JsonWriter::writeBinaryToJson_data_para(parentTasksJson, para_Id, para_addr, para_slice_length, para_slice_data_type, hex_slice_data_dest_str, val_t);
                    }
                }

                for (size_t i = 0; i < task.return_output.size(); ++i) {
                    auto return_Id = task.return_output[i].first;
                    auto parentTaskId = task.taskId;
                    // Port low 4 bits = index in all_output (same as allOutputJson); return_output order may differ
                    int output_port = static_cast<int>(i);
                    for (size_t j = 0; j < task.all_output.size(); ++j) {
                        if (std::get<0>(task.all_output[j]) == return_Id) {
                            output_port = static_cast<int>(j);
                            break;
                        }
                    }
                    JsonWriter::writeBinaryToJson_data(
                        returnJson_info, return_Id, parentTaskId, output_port);
                }

                for (size_t i = 0; i < task.parentTasks.size(); ++i) {
                    auto parentTaskId = std::get<0>(task.parentTasks[i]);
                    int childTaskSize;
                    int port_num = std::get<1>(task.parentTasks[i]);
                    std::string dest_address = std::get<2>(task.parentTasks[i]);
                    int concat_value = std::get<3>(task.parentTasks[i]);
                    std::string slice_length = std::get<4>(task.parentTasks[i]);
                    std::string slice_data_type = std::get<5>(task.parentTasks[i]);
                    auto varname = std::get<6>(task.parentTasks[i]);
                    bool is_pointer = std::get<7>(task.parentTasks[i]);
                    std::string parent_type_hint = std::get<8>(task.parentTasks[i]);
                    auto decimal_dest_address = parse_int(dest_address);
                    int slice_data_addr = parse_int(slice_length) * parse_int(slice_data_type);
                    int slice_data_dest = decimal_dest_address + slice_data_addr;
                    std::stringstream ss;
                    ss << std::hex << std::uppercase << slice_data_dest;
                    std::string hex_slice_data_dest_str = ss.str();
                    if (is_pointer) {
                        std::string pointer_length = "0";
                        int taskid_int = 0;
                        try { taskid_int = std::stoi(parentTaskId); } catch (...) { taskid_int = 0; }
                        std::string taskid_binary = std::bitset<6>(taskid_int).to_string();
                        std::string parentTasksPort = "0b" + taskid_binary + std::bitset<4>(port_num).to_string();
                        auto tmp_it = task.temp_size_map.find(varname);
                        if (tmp_it != task.temp_size_map.end()) {
                            pointer_length = std::to_string(tmp_it->second);
                        } else {
                            for (const auto& parent_task : output) {
                                if (parent_task.taskId == parentTaskId) {
                                    for (const auto& ret_output : parent_task.return_output) {
                                        if (ret_output.first == varname) {
                                            pointer_length = std::to_string(ret_output.second);
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                        std::string pointer_type;
                        if (!parent_type_hint.empty()) {
                            pointer_type = parent_type_hint;
                        } else if (tmp_it != task.temp_size_map.end()) {
                            pointer_type = "0b100";
                        } else {
                            pointer_type = "0b101";
                        }
                        JsonWriter::writeBinaryToJson_pointer(parentTasksJson, varname, dest_address, pointer_length, pointer_type, parentTasksPort);
                    } else {
                        JsonWriter::writeBinaryToJson(parentTasksJson, parentTaskId, port_num, dest_address, concat_value, slice_length, slice_data_type, hex_slice_data_dest_str, varname);
                    }
                }

                if (returnJson_info.empty())
                    returnJson["return_output"] = "None";
                else
                    returnJson["return_output"] = returnJson_info;
                if (parentTasksJson.empty())
                    taskJson["all_input"] = "None";
                else
                    taskJson["all_input"] = parentTasksJson;

                json allOutputJson;
                for (size_t i = 0; i < task.all_output.size(); ++i) {
                    json outputItem;
                    outputItem["name"] = std::get<0>(task.all_output[i]);
                    int taskid_int = std::stoi(task.taskId);
                    std::string taskid_binary = std::bitset<6>(taskid_int).to_string();
                    std::string computed_port = "0b" + taskid_binary + std::bitset<4>(i).to_string();
                    outputItem["parentTasksPort"] = computed_port;
                    outputItem["temp_offset"] = std::get<2>(task.all_output[i]);
                    outputItem["length"] = std::get<3>(task.all_output[i]);
                    allOutputJson.push_back(outputItem);
                }
                if (allOutputJson.empty())
                    taskJson["all_output"] = "None";
                else
                    taskJson["all_output"] = allOutputJson;
                break;
            
        }
        
        }
        
        outputJson.push_back(taskJson);
    }
    outputJson.push_back(returnJson);
    outputFileStream << std::setw(4) << outputJson;

    outputFileStream.close();

    return 0;
}