// This is a simplified version of nlohmann/json.
// For a real project, download the full library from: https://github.com/nlohmann/json

#pragma once
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace nlohmann {
    class json {
    private:
        enum class value_type {
            null,
            object,
            array,
            string,
            boolean,
            number
        };

        value_type type = value_type::null;
        std::map<std::string, json> object_value;
        std::vector<json> array_value;
        std::string string_value;
        bool bool_value = false;
        double number_value = 0.0;

    public:
        // Constructors
        json() : type(value_type::null) {}
        json(bool val) : type(value_type::boolean), bool_value(val) {}
        json(int val) : type(value_type::number), number_value(val) {}
        json(double val) : type(value_type::number), number_value(val) {}
        json(const std::string& val) : type(value_type::string), string_value(val) {}
        json(const char* val) : type(value_type::string), string_value(val) {}
        
        // Map constructor
        json(const std::map<std::string, bool>& map) : type(value_type::object) {
            for (const auto& pair : map) {
                object_value[pair.first] = json(pair.second);
            }
        }
        
        // Assignment operators
        json& operator[](const std::string& key) {
            type = value_type::object;
            return object_value[key];
        }
        
        // Value access
        std::string dump(int indent = 0) const {
            switch(type) {
                case value_type::null:
                    return "null";
                case value_type::boolean:
                    return bool_value ? "true" : "false";
                case value_type::number:
                    return std::to_string(number_value);
                case value_type::string:
                    return "\"" + string_value + "\"";
                case value_type::array: {
                    std::string result = "[";
                    for (size_t i = 0; i < array_value.size(); ++i) {
                        if (i > 0) result += ",";
                        result += array_value[i].dump(indent);
                    }
                    result += "]";
                    return result;
                }
                case value_type::object: {
                    if (object_value.empty()) return "{}";
                    
                    std::string spacing = indent > 0 ? std::string(indent, ' ') : "";
                    std::string inner_spacing = indent > 0 ? std::string(indent, ' ') : "";
                    std::string result = "{";
                    
                    if (indent > 0) result += "\n";
                    
                    bool first = true;
                    for (const auto& pair : object_value) {
                        if (!first) {
                            result += ",";
                            if (indent > 0) result += "\n";
                        }
                        first = false;
                        
                        if (indent > 0) result += spacing;
                        result += "\"" + pair.first + "\":" + (indent > 0 ? " " : "") + pair.second.dump(indent);
                    }
                    
                    if (indent > 0) result += "\n" + (indent > 4 ? std::string(indent - 4, ' ') : "");
                    result += "}";
                    return result;
                }
            }
            return "null"; // default
        }
        
        // Make specific bool_value public for our simple implementation
        bool bool_value_public() const {
            return bool_value;
        }
        
        // Iterators for object type - with explicit types for C++11
        typedef std::map<std::string, json>::iterator iterator;
        typedef std::map<std::string, json>::const_iterator const_iterator;
        
        iterator begin() { return object_value.begin(); }
        iterator end() { return object_value.end(); }
        const_iterator begin() const { return object_value.begin(); }
        const_iterator end() const { return object_value.end(); }
        
        // Friend functions for serialization
        friend std::istream& operator>>(std::istream& is, json& j);
    };
    
    // Basic parser - only handles simple cases
    std::istream& operator>>(std::istream& is, json& j) {
        // Very simplified parser - only supports objects with string keys and boolean values
        std::string content((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        
        if (content.empty() || content == "{}") {
            j = json();
            return is;
        }
        
        size_t pos = 0;
        while (pos < content.length()) {
            // Find key
            pos = content.find("\"", pos);
            if (pos == std::string::npos) break;
            pos++;
            size_t end_pos = content.find("\"", pos);
            if (end_pos == std::string::npos) break;
            
            std::string key = content.substr(pos, end_pos - pos);
            pos = content.find(":", end_pos);
            if (pos == std::string::npos) break;
            pos++;
            
            // Skip whitespace
            while (pos < content.length() && std::isspace(content[pos])) pos++;
            
            // Parse value
            if (content.substr(pos, 4) == "true") {
                j[key] = true;
                pos += 4;
            } else if (content.substr(pos, 5) == "false") {
                j[key] = false;
                pos += 5;
            } else if (content[pos] == '\"') {
                pos++;
                size_t value_end = content.find("\"", pos);
                if (value_end == std::string::npos) break;
                j[key] = content.substr(pos, value_end - pos);
                pos = value_end + 1;
            } else if (std::isdigit(content[pos]) || content[pos] == '-') {
                size_t value_end = pos;
                while (value_end < content.length() && 
                      (std::isdigit(content[value_end]) || content[value_end] == '.' || 
                       content[value_end] == '-' || content[value_end] == 'e' || 
                       content[value_end] == 'E' || content[value_end] == '+')) {
                    value_end++;
                }
                std::string num_str = content.substr(pos, value_end - pos);
                j[key] = std::stod(num_str);
                pos = value_end;
            }
            
            // Find next key
            pos = content.find(",", pos);
            if (pos == std::string::npos) break;
            pos++;
        }
        
        return is;
    }
}