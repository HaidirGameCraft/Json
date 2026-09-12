#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>

namespace Json {
    enum JsonType {
        JsonType_Begin,
        JsonType_Float,
        JsonType_Int,
        JsonType_String,
        JsonType_Array,
        JsonType_Object
    };

    struct JsonValue {
        std::string     name;
        JsonType        type;
        long            value;
    };

    struct JsonString {
        std::string     name;
        JsonType        type;
        std::string     value;
    };
    
    struct JsonObject {
        std::string     name;
        JsonType        type;
        std::vector<std::pair<std::string, struct JsonObject*>> objects;
    };
    
    struct JsonArray {
        std::string     name;
        JsonType        type;
        std::vector<struct JsonObject*> array;
    };

    JsonObject JsonCreate();
    void    JsonLoadFromFile( const std::string& file_path, JsonObject* json_object );
    void    JsonLoadFromContent( const char* content, size_t size, JsonObject* json_object );
    void    JsonCreateInt( const std::string& name, int value, JsonObject* json_object );
    void    JsonCreateFloat( const std::string& name, float value, JsonObject* json_object );
    void    JsonCreateString( const std::string& name, const std::string& str, JsonObject* json_object );

    size_t  JsonGetSizeArray( JsonArray* array );
    int     JsonGetInt( JsonObject* json_object, const std::string& name );
    float   JsonGetFloat( JsonObject* json_object, const std::string& name );
    std::string JsonGetString( JsonObject* json_object, const std::string& name );
    JsonArray* JsonGetArray( JsonObject* json_object, const std::string& name );
    JsonObject* JsonGetObject( JsonObject* json_object, const std::string& name );

    int     JsonGetInt( JsonArray* array, int i );
    float   JsonGetFloat( JsonArray* array, int i );
    std::string JsonGetString( JsonArray* array, int i );
    JsonArray* JsonGetArray( JsonArray* array, int i );
    JsonObject* JsonGetObject( JsonArray* array, int i );

    void    JsonPrint( JsonObject* json_object );
    struct JsonObject* JsonCreateObject( const std::string& name, JsonObject* json_object );
    struct JsonArray*    JsonCreateArray( const std::string& name, JsonObject* json_object );
    std::string     JsonObjectToString( JsonObject* object );
    std::string     JsonArrayToString( JsonArray* array );
    std::string    JsonToString( JsonObject* json_object );
    void    JsonDestroy( JsonObject* json_object );
}

