#ifndef JSONWRITER_H
#define JSONWRITER_H

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonWriter {
public:
    static void writeBinaryToJson(json& jsonData, const std::string& taskId , int port_num, std::string dest_addr, int concat_value, std::string slice_length, std::string slice_data_type, std::string hex_slice_data_dest_str, std::string varname);
    static void writeBinaryToJson_data_global(json &jsonData, const std::string& Id, std::string addr, const std::string& value_type);
    static void writeBinaryToJson_data_para(json &jsonData, const std::string& Id, std::string addr, std::string slice_length, std::string slice_data_type, std::string hex_slice_data_dest_str, const std::string& value_type);
    static void writeBinaryToJson_data(json &jsonData, const std::string& Id,const std::string& taskId, int port_num);
    static void writeBinaryToJson_pointer(json &jsonData, const std::string& Id, std::string addr, std::string length, std::string pointer_type, std::string parentTasksPort);
};

#endif // JSONWRITER_H