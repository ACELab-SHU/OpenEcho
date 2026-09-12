#include "TaskConverter.hpp"

std::pair<std::vector<Task>, std::unordered_map<std::string, int>> TaskConverter::convertToTasks(const std::vector<inputTask> &inputTasks)
{
    std::unordered_map<std::string, int> idMapping;
    std::vector<Task> tasks;

    // Create a mapping between string IDs and integer IDs
    for (const auto &inputTask : inputTasks)
    {
        int newId = idMapping.size(); // Assign a new integer ID
        idMapping[inputTask.taskId] = newId;
    }

    // Convert inputTask objects to Task objects using integer IDs
    for (const auto &inputTask : inputTasks)
    {
        Task newTask;
        newTask.taskId          = idMapping[inputTask.taskId];
        newTask.computationCost = inputTask.computationCost;
        newTask.spm_size        = inputTask.spm_size;
        newTask.num_lane        = inputTask.num_lane;
        newTask.has_bitalu      = inputTask.has_bitalu;
        newTask.has_serdiv      = inputTask.has_serdiv;
        newTask.has_complexunit = inputTask.has_complexunit;
        newTask.global_Input    = inputTask.global_Input;
        newTask.para_Input      = inputTask.para_Input;
        newTask.return_output   = inputTask.return_output;
        newTask.temp_size_map   = inputTask.temp_size_map;
        newTask.address         = inputTask.address;
        newTask.length          = inputTask.length;
        newTask.output_num      = inputTask.output_num;
        newTask.is_spmd         = inputTask.is_spmd;
        newTask.min_core_num    = inputTask.min_core_num;

        // Convert parentTasks and childTasks to integer IDs
        for (const auto &parent : inputTask.parentTasks)
        {
            auto parent_id = std::get<0>(parent);
            int portIndex = std::get<1>(parent);
            auto dest_address = std::get<2>(parent);
            int concat_value = std::get<3>(parent);
            auto slice_length = std::get<4>(parent);
            auto slice_data_type = std::get<5>(parent);
            auto varname = std::get<6>(parent);
            bool is_pointer = std::get<7>(parent);
            auto type_hint = std::get<8>(parent);
            if (idMapping.find(parent_id) != idMapping.end())
            {
                newTask.parentTasks.push_back({idMapping[parent_id], portIndex, dest_address, concat_value, slice_length, slice_data_type, varname, is_pointer, type_hint});
            }
            else
            {
                newTask.parentTasks.push_back({-1, 0, "0", 0, "0", "0", "", false, ""});
            }
        }

        for (const auto &child : inputTask.childTasks)
        {   
            std::string child_id = std::get<0>(child);
            int portIndex = std::get<1>(child);
            std::string dest_address = std::get<2>(child);
            int concat_value = std::get<3>(child);
            std::string slice_length = std::get<4>(child);
            std::string slice_data_type = std::get<5>(child);
            std::string varname = std::get<6>(child);
            if (idMapping.find(child_id) != idMapping.end())
            {
                newTask.childTasks.push_back({idMapping[child_id], portIndex ,dest_address, concat_value, slice_length, slice_data_type, varname}); // Port Index set to 0
            }
            else
            {
                newTask.childTasks.push_back({-1, 0, 0, 0, 0, 0, 0}); // Port Index set to 0
            }
        }

        tasks.push_back(newTask);
    }

    return {tasks, idMapping};
}
