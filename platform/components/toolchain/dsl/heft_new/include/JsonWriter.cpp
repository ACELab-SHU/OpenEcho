#include "JsonWriter.hpp"
#include <bitset>
#include <iostream>

void JsonWriter::writeBinaryToJson(json &jsonData, const std::string& taskId , int port_num, std::string dest_addr, int concat_value, std::string slice_length, std::string slice_data_type, std::string hex_slice_data_dest_str, std::string varname)
{
    int taskid_int = 0;
    try { taskid_int = std::stoi(taskId); } catch (...) { taskid_int = 0; }
    std::string taskid_binary = std::bitset<6>(taskid_int).to_string();
    json taskDataJson;
    taskDataJson["type"] = "0b000";  // 修改为 3bit：temp 类型
    taskDataJson["parentTasks"] = taskId;
    taskDataJson["name"] = varname;
    taskDataJson["dest_address"] = dest_addr;
    taskDataJson["parentTasksPort"] = "0b" + taskid_binary + std::bitset<4>(port_num).to_string();
    taskDataJson["concat_value"] = concat_value;
    taskDataJson["slice_length"] = slice_length;
    taskDataJson["slice_data_type"] = slice_data_type;
    taskDataJson["slice_data_dest_str"] = "0x" + hex_slice_data_dest_str;
    jsonData.push_back(taskDataJson);
}

void JsonWriter::writeBinaryToJson_data_global(json &jsonData, const std::string& Id, std::string addr, const std::string& value_type)
{
    json taskDataJson;
    taskDataJson["type"] = value_type;  // 0b001：global（与 param/global 同类静态）
    taskDataJson["name"] = Id;
    taskDataJson["dest_address"] = addr;
    taskDataJson["parentTasksPort"] = "0b0000000000";
    std::cout << "writeBinaryToJson_data_global parentTask_dest: " << addr << std::endl;
    jsonData.push_back(taskDataJson);
}

void JsonWriter::writeBinaryToJson_data_para(json &jsonData, const std::string& Id, std::string addr, std::string slice_length, std::string slice_data_type,std::string hex_slice_data_dest_str, const std::string& value_type)
{
    json taskDataJson;
    taskDataJson["type"] = value_type;  // 0b001 parameter/global；0b010 dag_input/dfedata
    taskDataJson["name"] = Id;
    taskDataJson["dest_address"] = addr;
    taskDataJson["parentTasksPort"] = "0b0000000000";
    taskDataJson["slice_length"] = slice_length;
    taskDataJson["slice_data_type"] = slice_data_type;
    std::cout << "writeBinaryToJson_data__para parentTask_dest: " << addr << std::endl;
    taskDataJson["slice_data_dest_str"] = "0x" + hex_slice_data_dest_str;
    jsonData.push_back(taskDataJson);
}

void JsonWriter::writeBinaryToJson_data(json &jsonData, const std::string& Id,const std::string& taskId, int port_num)
{
    json taskDataJson;
    taskDataJson["name"] = Id;
    int taskid_int = 0;
    try { taskid_int = std::stoi(taskId); } catch (...) { taskid_int = 0; }
    std::string taskid_binary = std::bitset<6>(taskid_int).to_string();
    taskDataJson["parentTasks"] = taskId;
    taskDataJson["parentTasksPort"] = "0b"+taskid_binary + std::bitset<4>(port_num).to_string();
    jsonData.push_back(taskDataJson);
}

//新增pointer类型数据的写入函数
void JsonWriter::writeBinaryToJson_pointer(json &jsonData, const std::string& Id, std::string addr, std::string length, std::string pointer_type, std::string parentTasksPort)
{
    json taskDataJson;
    taskDataJson["type"] = pointer_type;  // 0b100/0b101/0b110
    taskDataJson["name"] = Id;
    taskDataJson["dest_address"] = addr;
    taskDataJson["parentTasksPort"] = parentTasksPort;
    taskDataJson["slice_length"] = "0";
    taskDataJson["slice_data_type"] = "0";
    taskDataJson["slice_data_dest_str"] = addr;
    taskDataJson["length"] = length;
    std::cout << "writeBinaryToJson_pointer dest: " << addr << ", type: " << pointer_type << std::endl;
    jsonData.push_back(taskDataJson);
}