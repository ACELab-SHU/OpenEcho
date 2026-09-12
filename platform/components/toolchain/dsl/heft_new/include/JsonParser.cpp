#include "JsonParser.hpp"
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<inputTask> JsonParser::parseJson(const std::string& filename) {
    std::ifstream file(filename);
    json jsonData;
    file >> jsonData;

    // 推导 temp_alloc.json 路径：filename 形如 ./IJ/dag1/slice_updated_tasks.json
    // temp_alloc 在 ./variable/map/dag1_temp_alloc.json
    std::unordered_map<std::string, int> temp_alloc_size_map;
    {
        // 从 filename 中提取 dag 名称
        std::string dag_name;
        auto ij_pos = filename.find("/IJ/");
        if (ij_pos == std::string::npos) ij_pos = filename.find("\\IJ\\");
        if (ij_pos != std::string::npos) {
            auto after_ij = ij_pos + 4;
            auto slash_pos = filename.find_first_of("/\\", after_ij);
            if (slash_pos != std::string::npos)
                dag_name = filename.substr(after_ij, slash_pos - after_ij);
        }
        if (!dag_name.empty()) {
            // main 从 heft_new/ 目录运行，temp_alloc 在上一级的 variable/map/ 下
            std::string temp_alloc_path = "../variable/map/" + dag_name + "_temp_alloc.json";
            std::ifstream ta_file(temp_alloc_path);
            if (ta_file.is_open()) {
                json ta_data;
                ta_file >> ta_data;
                for (const auto& entry : ta_data) {
                    std::string name = entry["name"];
                    int size_bytes   = entry["size_bytes"];
                    temp_alloc_size_map[name] = size_bytes;
                }
            }
        }
    }

    std::vector<inputTask> inputTasks;

    for (const auto& taskData : jsonData) {
        inputTask task;
        task.taskId          = taskData["taskId"];
        task.computationCost = taskData["computationCost"];
        task.spm_size        = taskData["spm_size"];
        task.num_lane        = taskData["num_lane"];
        task.has_bitalu      = taskData["has_bitalu"];
        task.has_serdiv      = taskData["has_serdiv"];
        task.has_complexunit = taskData["has_complexunit"];
        task.text_offset     = taskData.value("text_offset", 0);
        task.data_offset     = taskData.value("data_offset", 0);
        task.total_length    = taskData.value("total_length", 0);
        task.text_length     = taskData.value("text_length", 0);
        task.data_length     = taskData.value("data_length", 0);
        task.output_num      = taskData["output_num"];
        task.hardwareinfo    = taskData["hardwareinfo"];
        task.hash            = taskData.value("hash", "0x0");

        if (taskData.contains("is_spmd")) {
            task.is_spmd = taskData["is_spmd"];
        } else {
            task.is_spmd = 0;
        }

        if (taskData.contains("min_core_num")) {
            task.min_core_num = taskData["min_core_num"];
        } else {
            task.min_core_num = 1;
        }

        // parent
        for (const auto& parentTask : taskData["parentTasks"]) {
            std::string parentId = parentTask["taskId"];
            int outputPort = parentTask["outputIndex"];
            std::cout << "parentTask_dest: " << parentTask["dest_address"] << std::endl;
            int concat_value = parentTask["concat_value"];
            std::string parentTask_slice_length   ;
            std::string parentTask_slice_data_type;
            std::string varname = parentTask["outputVar"];
            bool is_pointer = parentTask.value("is_pointer", false);
            std::string parent_type_hint = parentTask.value("type", "");

            if (parentTask["dest_address"] != "null"){
                std::cout << "if parentTask_dest: " << parentTask["dest_address"] << std::endl;
                std::string dest_addr = parentTask["dest_address"];
                if (parentTask.contains("slice_length")) {
                    parentTask_slice_length = parentTask["slice_length"];
                    parentTask_slice_data_type = parentTask["slice_data_type"];
                }
                else {
                    parentTask_slice_length = "0";
                    parentTask_slice_data_type = "0";
                }
                task.parentTasks.push_back({parentId, outputPort, dest_addr, concat_value, parentTask_slice_length, parentTask_slice_data_type, varname, is_pointer, parent_type_hint});
            }
        }

        // child
        for (const auto& childTask : taskData["childTasks"]) {
            std::string childId = childTask["taskId"];
            int inputPort = childTask["inputIndex"];
            std::string dest_addr = "null";
            int concat_value = childTask["concat_value"];
            std::string childTask_slice_length ;
            std::string childTask_slice_data_type;
            std::string varname = childTask["inputVar"];

            if (childTask.contains("slice_length")) {
                childTask_slice_length    = childTask["slice_length"];
                childTask_slice_data_type = childTask["slice_data_type"];
            }
            else {
                childTask_slice_length    = "0";
                childTask_slice_data_type = "0";
            }
            task.childTasks.push_back({childId, inputPort, dest_addr, concat_value, childTask_slice_length, childTask_slice_data_type, varname});
        }

        //data
        for (const auto& global_Input : taskData["global_Input"]) {
            std::string globalId = global_Input["name"];
            std::cout << "global_Input: " << global_Input["dest_address"] << std::endl;
            if (global_Input["dest_address"] != "null")
            {   std::cout << "in global_Input: " << global_Input["dest_address"] << std::endl;
                std::string global_addr = global_Input["dest_address"];
                task.global_Input.push_back({globalId, global_addr});
            }
        }
        for (const auto& para_Input : taskData["para_Input"]) {
            std::string paraId = para_Input["name"];
            if( para_Input["dest_address"] != "null" ){
                std::string paraId_addr = para_Input["dest_address"];
                std::string para_slice_length;
                std::string para_slice_data_type;
                bool is_pointer = para_Input.value("is_pointer", false);
                std::string para_type = para_Input.value("type", "");
                if (para_Input.contains("slice_length")) {
                    para_slice_length = para_Input["slice_length"];
                    para_slice_data_type = para_Input["slice_data_type"];
                }
                else {
                    para_slice_length = "0";
                    para_slice_data_type = "0";
                }
                task.para_Input.push_back({paraId, paraId_addr, para_slice_length, para_slice_data_type, is_pointer, para_type});
            }
        }
        for (const auto& return_output : taskData["return_output"]) {
            std::string returnId = return_output["name"];
            int return_length = 0;
            if (return_output.contains("length")) {
                const auto& len_val = return_output["length"];
                if (len_val.is_number()) {
                    return_length = len_val.get<int>();
                } else if (len_val.is_string()) {
                    std::string length_str = len_val.get<std::string>();
                    if (!length_str.empty()) {
                        return_length = std::stoi(length_str, nullptr, 0);
                    }
                }
            }
            task.return_output.push_back({returnId, return_length});
        }

        if (taskData.contains("all_output")) {
            for (const auto& all_out : taskData["all_output"]) {
                std::string name = all_out["name"];
                std::string parentTasksPort = all_out.value("parentTasksPort", "");
                int temp_offset = all_out.value("temp_offset", 0);
                int length = 0;
                if (all_out.contains("length")) {
                    const auto& len_val = all_out["length"];
                    if (len_val.is_number()) {
                        length = len_val.get<int>();
                    } else if (len_val.is_string()) {
                        std::string length_str = len_val.get<std::string>();
                        if (!length_str.empty()) {
                            length = std::stoi(length_str, nullptr, 0);
                        }
                    }
                }
                task.all_output.push_back({name, parentTasksPort, temp_offset, length});
            }
        }

        inputTasks.push_back(task);
    }

    // 将临时变量的 size_bytes 存入当前消费 task 自身的 temp_size_map，
    // key = varname，供 main.cpp 在处理 parentTasks 时用 varname 查找。
    // 注意：main.cpp 处理 task.parentTasks[i] 时，parentTaskId 已被重映射为数字，
    // 但 temp_size_map 挂在"当前 task"（消费者）上，用 varname 直接查，不依赖 parentTaskId 匹配。
    for (auto& consumer : inputTasks) {
        for (const auto& parent : consumer.parentTasks) {
            std::string varname = std::get<6>(parent);
            bool is_pointer     = std::get<7>(parent);
            if (!is_pointer) continue;   // 只有指针输入才需要 pointer_length
            auto it = temp_alloc_size_map.find(varname);
            if (it == temp_alloc_size_map.end()) continue;
            consumer.temp_size_map[varname] = it->second;
        }
    }

    return inputTasks;
}