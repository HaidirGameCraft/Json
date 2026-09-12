#include "Json.hpp"

#include <cstdlib>
#include <fstream>
#include <cstdio>
#include <cstring>

using namespace Json;
enum JsonState {
    JsonState_Object,
    JsonState_Array,
    JsonState_NameIdentify,
    JsonState_Value
};

enum JsonTokenType {
    JsonTokenType_None,
    JsonTokenType_Int,
    JsonTokenType_Float,
    JsonTokenType_String,
    JsonTokenType_DoubleDot,
    JsonTokenType_LParen,
    JsonTokenType_RParen,
    JsonTokenType_LBrac,
    JsonTokenType_RBrac
};


struct JsonDecode_Object {
    JsonState   state;
    struct JsonObject* object;  // Either Object or Array
};

JsonObject Json::JsonCreate() {
    JsonObject json_object;
    json_object.type = JsonType_Begin;
    json_object.name = "<Begin>";
    json_object.objects = std::vector<std::pair<std::string, struct JsonObject*>>();
    return json_object;
}

void    Json::JsonLoadFromFile( const std::string& file_path, JsonObject* json_object ) {
    std::ifstream file( file_path, std::ios_base::binary );
    if( !file.is_open() )
    {
        printf("Error: Could not open the file '%s': No such or an directory\n", file_path.c_str() );
        exit(69);
    }

    file.seekg( 0, std::ios_base::end );
    size_t file_size = file.tellg();
    file.seekg( 0, std::ios_base::beg );

    char* content = (char*) std::malloc( file_size + 1 );
    size_t index_content = 0;
    std::string line;
    while( std::getline( file, line ) ) {
        std::memcpy( &content[index_content], line.c_str(), line.size() );
        index_content += line.size();
    }


    content[file_size] = 0; // NULL Terminate
    printf("Content: %s\n", content);
    Json::JsonLoadFromContent( content, file_size, json_object );
    std::free( content );
}

void Json::JsonLoadFromContent( const char* content, size_t size, JsonObject* json_object ) {
    std::vector<struct JsonObject*> json_tracker;
    std::string name_identify;
    JsonState state = JsonState_NameIdentify;
    JsonState obj_state = JsonState_Object;
    
    for( size_t i = 0; i < size; i++ ) {
        if( content[i] == ' ' || content[i] == '\n' || content[i] == '\t' ) {
            continue;
        }
        else if( content[i] == '{' ) // Create new Body but not the Main Body
        {
            if( json_tracker.size() == 0 )
                json_tracker.push_back( json_object );
            else {
               JsonObject* object = JsonCreateObject( name_identify, json_tracker[json_tracker.size() - 1] );
               json_tracker.push_back( object );
            }
            obj_state = JsonState_Object;
            state = JsonState_NameIdentify;
        }
        else if ( content[i] == '[' ) {
            JsonObject* last = json_tracker[json_tracker.size() - 1];
            JsonArray* array = JsonCreateArray( name_identify, last );
            json_tracker.push_back( (JsonObject*) array );
            obj_state = JsonState_Array;
            state = JsonState_Value;
        }
        else if ( content[i] == ':' ) {
            if( state != JsonState_NameIdentify ) {
                printf("Error: No command of the Identify Name\n");
                exit(69);
            }

            state = JsonState_Value;
        }
        else if ( content[i] == ',' ) {
            JsonObject* last = json_tracker[json_tracker.size() - 1];
            if( last->type == JsonType_Object || last->type == JsonType_Begin ) {
                obj_state = JsonState_Object;
                state = JsonState_NameIdentify;
            }
            else if( last->type == JsonType_Array ) {
                obj_state = JsonState_Array;
                state = JsonState_Value;
            }
        }
        else if ( content[i] == ']' ) {
            JsonArray* array = (JsonArray*) json_tracker[json_tracker.size() - 1];
            json_tracker.pop_back();
            if( array->type != JsonType_Array ) {
                printf("Error: Unexpected Object to be an Array\n");
                exit(69);
            }

            if( json_tracker[json_tracker.size() - 1]->type == JsonType_Object ||
                json_tracker[json_tracker.size() - 1]->type == JsonType_Begin ) {
                obj_state = JsonState_Object;
            }
            else if ( json_tracker[ json_tracker.size() - 1 ]->type == JsonType_Array )
                obj_state = JsonState_Array;
            state = JsonState_Value;
        }
        else if ( content[i] == '}' ) {
            if( json_tracker.size() == 0 ) {
                printf("Error: Unexcepted Out of Body\n");
                exit(69);
            }

            JsonObject* object = (JsonObject*) json_tracker[json_tracker.size() - 1];
            json_tracker.pop_back();
            if( json_tracker.size() == 0 )
                return;
            
            if( json_tracker[json_tracker.size() - 1]->type == JsonType_Object ||
                    json_tracker[json_tracker.size() - 1]->type == JsonType_Begin )
                obj_state = JsonState_Object;
            else if ( json_tracker[ json_tracker.size() - 1 ]->type == JsonType_Array )
                obj_state = JsonState_Array;
            state = JsonState_Value;
        }
        else if ( content[i] == '"' ) {
            // Opening String
            std::string tmp;
            i++;
            while( content[i] != '\"' )
            {
                if( i >= size )
                {
                    printf("Error: Out of Content\n");
                    exit(69);
                }
                tmp += content[i];
                i++;
            }

            if( state == JsonState_NameIdentify ) {
                // Save the name identification
                name_identify = tmp;
            }
            else if ( state == JsonState_Value ){
                // Identify the type of object it Is String type
                JsonObject* object = json_tracker[json_tracker.size() - 1];
                JsonCreateString( name_identify, tmp, object );
                name_identify.clear();
            }
        }
        else if ( state == JsonState_Value ) {
            std::string tmp;
            bool is_float = false;
            int index_dot = 0;
            int prev_i = i;
            while( content[i] != ',' ) {
                if( content[i] == ' ' || content[i] == '\n' || content[i] == '\t' )
                {
                    i++;
                    continue;
                }

                if( content[i] == ']' || content[i] == '}' )
                    break;

                if( content[i] == '.' )
                {
                    is_float = true;
                    index_dot = i - prev_i;
                }

                tmp += content[i];
                i++;
            }
            i--;

            double value_float = 0;
            int value_int = 0;
            JsonObject* last = json_tracker[json_tracker.size() - 1];

            if( is_float == false ) {
                int j = 0;
                while( j < tmp.size() )
                    value_int = value_int * 10 + (tmp[j++] - '0');
                
                JsonCreateInt( name_identify, value_int, last );
            } else {
                int j = 0;
                while( j < tmp.size() ) {
                    if( tmp[j] == '.' )
                    {
                        j++;
                        continue;
                    }
                    value_float = value_float * 10 + (tmp[j] - '0');
                    j++;
                }
                
                j = tmp.size() - 1;
                while( j > index_dot ) {
                    value_float /= 10;
                    j--;
                }
                JsonCreateFloat( name_identify, value_float, last );
            }

            name_identify.clear();
        }
    }
}

size_t Json::JsonGetSizeArray( JsonArray* array ) {
    return array->array.size();
}

void    Json::JsonCreateInt( const std::string& name, int value, JsonObject* json_object ) {
    if( json_object == NULL )
        return;

    JsonValue* json_value = new JsonValue;
    json_value->name = name;
    json_value->type = JsonType_Int;
    json_value->value = value;

    if( json_object->type == JsonType_Array )
    {
        ((JsonArray*) json_object)->array.push_back( (JsonObject*) json_value );
        return;
    }
    json_object->objects.push_back( { name, (JsonObject*) json_value } );
}

void    Json::JsonCreateFloat( const std::string& name, float value, JsonObject* json_object ) {
    if( json_object == NULL )
        return;

    JsonValue* json_value = new JsonValue;
    json_value->name = name;
    json_value->type = JsonType_Float;
    memcpy( &json_value->value, &value, sizeof( float ) );
    if( json_object->type == JsonType_Array )
    {
        ((JsonArray*) json_object)->array.push_back( (JsonObject*) json_value );
        return;
    }
    json_object->objects.push_back( { name, (JsonObject*) json_value } );
}

void    Json::JsonCreateString( const std::string& name, const std::string& str, JsonObject* json_object ) {
    if( json_object == NULL )
        return;

    JsonString* json_string = new JsonString;
    json_string->name = name;
    json_string->type = JsonType_String;
    json_string->value = str;
    if( json_object->type == JsonType_Array )
    {
        ((JsonArray*) json_object)->array.push_back( (JsonObject*) json_string );
        return;
    }
    json_object->objects.push_back( { name, (JsonObject*) json_string } );
}

struct JsonArray*    Json::JsonCreateArray( const std::string& name, JsonObject* json_object ) {
    if( json_object == NULL )
        return NULL;

    JsonArray* array = new JsonArray;
    array->type = JsonType_Array;
    array->name = name;
    array->array = std::vector<struct JsonObject*>();

    if( json_object->type == JsonType_Array )
        ((JsonArray*) json_object)->array.push_back( (JsonObject*) array );
    else
        json_object->objects.push_back( {name, (JsonObject*) array} );
    return array;
}

int Json::JsonGetInt( JsonObject* json_object, const std::string& name ) {
    for( int i = 0; i < json_object->objects.size(); i++ )
    {
        auto& item = json_object->objects[i];
        if( item.first == name ) {
            if( item.second->type != JsonType_Int )
            {
                printf("Error: value of key '%s' is not an Integer\n", name.c_str() );
                exit(69);
            }
            return (int) ((JsonValue*) item.second)->value;
        }
    }

    printf("Error: Could not found any key call '%s'\n", name.c_str() );
    exit(69);
}

float Json::JsonGetFloat( JsonObject* json_object, const std::string& name ) {
    for( int i = 0; i < json_object->objects.size(); i++ )
    {
        auto& item = json_object->objects[i];
        if( item.first == name ) {
            if( item.second->type != JsonType_Float ) {
                printf("Erorr: value of key '%s' is not an Float\n", name.c_str() );
                exit( 69 );
            }
            float f;
            memcpy( &f, &((JsonValue*) item.second)->value, sizeof( float ) );
            return f;
        }
    }

    printf("Error: Could not found any key call '%s'\n", name.c_str() );
    exit(69);
    
}

std::string Json::JsonGetString( JsonObject* json_object, const std::string& name ) {
    for( int i = 0; i < json_object->objects.size(); i++ )
    {
        auto& item = json_object->objects[i];
        if( item.first == name ) {
            if( item.second->type != JsonType_String ) {
                printf("Erorr: value of key '%s' is not an String\n", name.c_str() );
                exit( 69 );
            }
            return ((JsonString*) item.second)->value;
        }
    }

    printf("Error: Could not found any key call '%s'\n", name.c_str() );
    exit(69); 
}

JsonArray* Json::JsonGetArray( JsonObject* json_object, const std::string& name ) {
    for( int i = 0; i < json_object->objects.size(); i++ )
    {
        auto& item = json_object->objects[i];
        if( item.first == name ) {
            if( item.second->type != JsonType_Array ) {
                printf("Erorr: value of key '%s' is not an Array\n", name.c_str() );
                exit( 69 );
            }
            return ((JsonArray*) item.second);
        }
    }

    printf("Error: Could not found any key call '%s'\n", name.c_str() );
    exit(69); 
    return NULL;
}

JsonObject* Json::JsonGetObject( JsonObject* json_object, const std::string& name ) {
    for( int i = 0; i < json_object->objects.size(); i++ )
    {
        auto& item = json_object->objects[i];
        if( item.first == name ) {
            if( item.second->type != JsonType_Object ) {
                printf("Erorr: value of key '%s' is not an Object\n", name.c_str() );
                exit( 69 );
            }
            return item.second;
        }
    }

    printf("Error: Could not found any key call '%s'\n", name.c_str() );
    exit(69); 
    return NULL;
}

int     Json::JsonGetInt( JsonArray* array, int i ) {
    if( i < 0 || i >= array->array.size() )
    {
        printf("Error: accessing json array with index is out of range");
        exit(69);
    }

    JsonValue* value = (JsonValue*) array->array[i];
    if( value->type != JsonType_Int )
    {
        printf("Error: the values of index '%i' is not an Int\n", i );
        exit(69);
    }

    return (int) value->value;
}

float Json::JsonGetFloat( JsonArray* array, int i ) {
    if( i < 0 || i >= array->array.size() )
    {
        printf("Error: accessing json array with index is out of range");
        exit(69);
    }

    JsonValue* value = (JsonValue*) array->array[i];
    if( value->type != JsonType_Float )
    {
        printf("Error: the values of index '%i' is not an Float\n", i );
        exit(69);
    }

    float f;
    memcpy( &f, &value->value, sizeof( float ) );
    return f;
}

std::string Json::JsonGetString( JsonArray* array, int i ) {
    if( i < 0 || i >= array->array.size() )
    {
        printf("Error: accessing json array with index is out of range");
        exit(69);
    }

    JsonString* value = (JsonString*) array->array[i];
    if( value->type != JsonType_String )
    {
        printf("Error: the values of index '%i' is not an String\n", i );
        exit(69);
    }

    return value->value;
}

JsonArray* Json::JsonGetArray( JsonArray* array, int i ) {
    if( i < 0 || i >= array->array.size() )
    {
        printf("Error: accessing json array with index is out of range");
        exit(69);
    }

    JsonArray* value = (JsonArray*) array->array[i];
    if( value->type != JsonType_Array )
    {
        printf("Error: the values of index '%i' is not an Array\n", i );
        exit(69);
    }

    return value;

}

JsonObject* Json::JsonGetObject( JsonArray* array, int i ) {
    if( i < 0 || i >= array->array.size() )
    {
        printf("Error: accessing json array with index is out of range");
        exit(69);
    }

    JsonObject* value = (JsonObject*) array->array[i];
    if( value->type != JsonType_Object )
    {
        printf("Error: the values of index '%i' is not an Object\n", i );
        exit(69);
    }

    return value;
}

std::string Json::JsonObjectToString( JsonObject* object ) {
    std::string to_string = "{ ";
    std::vector<std::pair<std::string, struct JsonObject*>>& objects = object->objects;
    int i = 0;
    for( const auto& object : objects ) {
        if( i > 0 )
            to_string += ", ";
        std::string name = object.first;
        struct JsonObject* value = object.second;
        
        to_string += "\"" + name + "\": ";
        if( value->type == JsonType_String ) {
            to_string += "\"" + ((JsonString*) value)->value + "\"";
        }
        else if ( value->type == JsonType_Int ) {
            char buffer[20];
            sprintf( buffer, "%i", (int) ((JsonValue*) value)->value );
            to_string += buffer;
        }
        else if ( value->type == JsonType_Float ) {
            float v;
            memcpy( &v, &((JsonValue*) value)->value, sizeof( float ) );
            char buffer[20];
            sprintf( buffer, "%f", v );
            to_string += buffer;
        }
        else if ( value->type == JsonType_Object )
            to_string += Json::JsonObjectToString( value );
        else if ( value->type == JsonType_Array )
            to_string += Json::JsonArrayToString( (JsonArray*) value );
        i++;
    }

    to_string += " }";
    return to_string;
}
std::string Json::JsonArrayToString( JsonArray* json_array ) {
    std::string to_string = "[";
    std::vector<struct JsonObject*>& array = json_array->array;
    int i = 0;
    for( auto& item : array ) {
        if( i > 0 )
            to_string += ", ";
        if( item->type == JsonType_Int ) {
           char buffer[20];
           std::sprintf( buffer, "%i", (int) ((JsonValue*) item)->value );
           to_string += buffer;
        }
        else if ( item->type == JsonType_Float ) {
            float v;
            memcpy( &v, &((JsonValue*) item)->value, sizeof( float ) );
            char buffer[20];
            sprintf( buffer, "%f", v );
            to_string += buffer;
        }
        else if( item->type == JsonType_String ) {
            to_string += "\"" + ((JsonString*) item)->value + "\"";
        }
        else if ( item->type == JsonType_Object )
            to_string += Json::JsonObjectToString( item );
        else if ( item->type == JsonType_Array )
            to_string += Json::JsonArrayToString( (JsonArray*) item );
        i++;
    }

    to_string += "]";
    return to_string;
}

std::string Json::JsonToString( JsonObject* json_object ) {
    return JsonObjectToString( json_object );    
}

void JsonPrintTab( JsonObject* json_object, int tab ) {

    for( int i = 0; i < tab; i++ )
        printf("  ");

    if( json_object->type == JsonType_Array ) {
        printf("%s: ", json_object->name.c_str() );
        JsonArray* array = (JsonArray*) json_object;
        for( int i = 0; i < array->array.size(); i++ ) {
            JsonObject* item = array->array[i];
            if( item->type == JsonType_Array || item->type == JsonType_Object ) {
                JsonPrintTab( item, tab + 1 );
            } else {
                JsonPrintTab( item, tab );
            }
        }

        return;
    }
    else if ( json_object->type == JsonType_Object || json_object->type == JsonType_Begin ) {
        printf("%s: ", json_object->name.c_str() );
        for( int i = 0; i < json_object->objects.size(); i++ ) {
            auto& item = json_object->objects[i].second;
            if( item->type == JsonType_Array || item->type == JsonType_Object )
                JsonPrintTab( item, tab + 1 );
            else 
                JsonPrintTab( item, tab );
        }

        return;
    }
    else if ( json_object->type == JsonType_String ) {
        JsonString* str = (JsonString*) json_object;
        printf("%s: %s\n", str->name.c_str(), str->value.c_str() );
    }
    else if ( json_object->type == JsonType_Int ) {
        JsonValue* val = (JsonValue*) json_object;
        printf("%s: %li\n", val->name.c_str(), val->value );
    }
    else if ( json_object->type == JsonType_Float ) {
        JsonValue* val = (JsonValue*) json_object;
        float f;
        memcpy( &f, &val->value, sizeof( float ) );
        printf("%s: %f\n", val->name.c_str(), f );
    }
}

void Json::JsonPrint( JsonObject* json_object ) {
    JsonPrintTab( json_object, 0 );
}

struct JsonObject* Json::JsonCreateObject( const std::string& name, JsonObject* json_object ) {
    if( json_object == NULL )
        return NULL;

    JsonObject* object = new JsonObject;
    object->name = name;
    object->type = JsonType_Object;
    object->objects = std::vector<std::pair<std::string, struct JsonObject*>>();
    if( json_object->type == JsonType_Array )
    {
        ((JsonArray*) json_object)->array.push_back( (JsonObject*) object );
    }
    else {
        json_object->objects.push_back( {name, object});
    }
    return object;
}
void    Json::JsonDestroy( JsonObject* json_object ) {
    if( json_object == NULL )
        return;

    if( json_object->type == JsonType_Array ) {
        JsonArray* array = (JsonArray*) json_object;
        for( int i = 0; i < array->array.size(); i++ ) {
            Json::JsonDestroy( array->array[i] );
        }
        array->array.clear();
    }
    else if ( json_object->type == JsonType_Object || json_object->type == JsonType_Begin ) {
        std::vector<std::pair<std::string, struct JsonObject*>>& objects = json_object->objects;
        for( const auto& object : objects ) {
            Json::JsonDestroy( object.second );
        }
        objects.clear();
    }

    if( json_object->type != JsonType_Begin ) {
        if( json_object->type == JsonType_String )
            delete (JsonString*) json_object;
        else if ( json_object->type == JsonType_Int || json_object->type == JsonType_Float )
            delete (JsonValue*) json_object;
        else if ( json_object->type == JsonType_Object )
            delete json_object;
        else if ( json_object->type == JsonType_Array )
            delete (JsonArray*) json_object;
    }
}
