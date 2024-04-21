#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

///////
typedef
struct {
    FILE *finfo;       // default stdout
    FILE *fwarning;    // default stdout
    FILE *fdebug;      // default stdout
    FILE *ferror;      // default stderr
    long int max_size; // default 256
    bool debug;        // default false
} Log_Config;

Log_Config log_config;

// call `log_init(NULL);` to use default values.
// You dont have to provide all values if you want to modify something,
// fallback is the defaults
void log_init(Log_Config *config);

void log_info(char *fmt, ...);
void log_error(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_debug(const char *fmt, ...);
void panic(const char *fmt, ...);
//////

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef
enum {
    TOKEN_OPCBRAKT,
    TOKEN_CLCBRAKT,
    TOKEN_OPBRAKT ,
    TOKEN_CLBRAKT ,
    TOKEN_STRING  ,
    TOKEN_COLON   ,
    TOKEN_COMMA   ,
    TOKEN_NUMBER  ,
    TOKEN_BOOLEAN ,
    TOKEN_NULL    ,
    _TOTAL_TOKENS ,
} Token_Type;

char* TOKEN_DESCRIPTION[] = {
    [TOKEN_OPCBRAKT]  = "TOKEN_OPCBRAKT",
    [TOKEN_CLCBRAKT]  = "TOKEN_CLCBRAKT",
    [TOKEN_OPBRAKT]   = "TOKEN_OPBRAKT" ,
    [TOKEN_CLBRAKT]   = "TOKEN_CLBRAKT" ,
    [TOKEN_STRING]    = "TOKEN_STRING"  ,
    [TOKEN_COLON]     = "TOKEN_COLON"   ,
    [TOKEN_COMMA]     = "TOKEN_COMMA"   ,
    [TOKEN_NUMBER]    = "TOKEN_NUMBER"  ,
    [TOKEN_BOOLEAN]   = "TOKEN_BOOLEAN" ,
    [TOKEN_NULL]      = "TOKEN_NULL"    ,
};

_Static_assert(
    ARRAY_SIZE(TOKEN_DESCRIPTION) == _TOTAL_TOKENS,
    "assert that you have implemented the description of all tokens"
);

// TODO: check max string len and max key len
#define MAX_STR_LEN 1024

typedef
struct {
    char value[MAX_STR_LEN];
    Token_Type type;
} Json_Token;

#define FMT_TOKEN "( %s ) => %s"
#define ARG_TOKEN(t) (t)->value, token_desc(((t)->type))

char *token_desc(Token_Type token_type) {
    return TOKEN_DESCRIPTION[token_type];
}

typedef
struct {
    char *json_str;
    size_t cursor;
    Json_Token *token;
} Json_Tokenizer;

#define EOJ '\0'
static char next_char(Json_Tokenizer *tokenizer) {
    if (tokenizer->json_str[tokenizer->cursor] == '\0') return EOJ;
    return tokenizer->json_str[tokenizer->cursor++];
}

int next_token(Json_Tokenizer *tokenizer) {
    double powerd(double x, int y);

    char next_char (Json_Tokenizer *tokenizer);
    char c = next_char(tokenizer);


    while (c != EOJ && isspace(c)) c = next_char(tokenizer);
    if (c == EOJ) return -1;

    /* is string */
    unsigned int i = 0;
    if (c == '"') {
        while (c != EOJ && (c = next_char(tokenizer)) != '"') {
            tokenizer->token->value[i++] = c;
        }

        tokenizer->token->value[i] = '\0';
        tokenizer->token->type = TOKEN_STRING;
        return 0; /* token is string, nothing to do anymore */
    }

    /* is number */
    if (c == '-' || c == '+' || isdigit(c)) {
        tokenizer->token->value[i++] = c;
        c = next_char(tokenizer);
        while(c != EOJ && isdigit(c)) {
            tokenizer->token->value[i++] = c;
            c = next_char(tokenizer);
        }

        if (c == '.') {
            tokenizer->token->value[i++] = c;
            c = next_char(tokenizer);
            while(c != EOJ && isdigit(c)) {
                tokenizer->token->value[i++] = c;
                c = next_char(tokenizer);
            }
        }

        if (c == 'e') {
            tokenizer->token->value[i++] = c;
            c = next_char(tokenizer);
            if (c == '-' || c == '+') {
                tokenizer->token->value[i++] = c;
                c = next_char(tokenizer);
            }

            while(c != EOJ && isdigit(c)) {
                tokenizer->token->value[i++] = c;
                c = next_char(tokenizer);
            }
        }

        tokenizer->token->value[i] = '\0';
        tokenizer->token->type = TOKEN_NUMBER;
        tokenizer->cursor--;
        return 0;
    }

    /* boolean */
    if (c == 'f') {
        if (next_char(tokenizer) != 'a' ||
            next_char(tokenizer) != 'l' ||
            next_char(tokenizer) != 's' ||
            next_char(tokenizer) != 'e')
        {
            panic("Unexpected character %s", c);
        }

        *tokenizer->token->value = '\0';
        strncat(tokenizer->token->value, "false", 6);
        tokenizer->token->type = TOKEN_BOOLEAN;
        return 0;
    }

    if (c == 't') {
        if (next_char(tokenizer) != 'r' ||
            next_char(tokenizer) != 'u' ||
            next_char(tokenizer) != 'e')
        {
            panic("Unexpected character %s", c);
        }

        *tokenizer->token->value = '\0';
        strncat(tokenizer->token->value, "true", 5);
        tokenizer->token->type = TOKEN_BOOLEAN;
        return 0;
    }

    if (c == 'n') {
        if (next_char(tokenizer) != 'u' ||
            next_char(tokenizer) != 'l' ||
            next_char(tokenizer) != 'l')
        {
            panic("Unexpected character %s", c);
        }

        *tokenizer->token->value = '\0';
        tokenizer->token->type = TOKEN_NULL;
        return 0;
    }

    /* rest of the 1 char tokens are readed in the string buffer */
    switch (c) {
        case '{': tokenizer->token->type = TOKEN_OPCBRAKT; break;
        case '}': tokenizer->token->type = TOKEN_CLCBRAKT; break;
        case '[': tokenizer->token->type = TOKEN_OPBRAKT;  break;
        case ']': tokenizer->token->type = TOKEN_CLBRAKT;  break;
        case ':': tokenizer->token->type = TOKEN_COLON;    break;
        case ',': tokenizer->token->type = TOKEN_COMMA;    break;
        default: {
            fprintf(stderr, "Invalid token => %c\n", c);
            exit(1);
        }
    }

    tokenizer->token->value[i++] = c;
    tokenizer->token->value[i] = '\0';

    return 0;
}

#define THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(t) if (next_token(t) == -1) panic("unexpected end of input");

//// parser
typedef void* Null;

typedef
enum {
    JSON_ARRAY       ,
    JSON_OBJECT      ,
    JSON_STRING      ,
    JSON_NUMBER      ,
    JSON_BOOLEAN     ,
    JSON_NULL        ,
    _TOTAL_JSON_TYPES,
} Json_Type;

char* JSON_TYPE_DESCRIPTION[] = {
    [JSON_ARRAY]   = "JSON_ARRAY"  ,
    [JSON_OBJECT]  = "JSON_OBJECT" ,
    [JSON_STRING]  = "JSON_STRING" ,
    [JSON_NUMBER]  = "JSON_NUMBER" ,
    [JSON_BOOLEAN] = "JSON_BOOLEAN",
    [JSON_NULL]    = "JSON_NULL"   ,
};

_Static_assert(
    ARRAY_SIZE(JSON_TYPE_DESCRIPTION) == _TOTAL_JSON_TYPES,
    "assert that you have implemented the description of all the `Json_Type`'s"
);

char *json_type_desc(Json_Type json_type) {
    return JSON_TYPE_DESCRIPTION[json_type];
}

typedef
struct {
    char *content;
    unsigned int len;
} Json_String;

typedef struct Json_Object Json_Object;
typedef struct Json_Array Json_Array;

typedef
union {
    Json_Object *object;
    Json_Array *array;
    Json_String string;
    double number;
    bool boolean;
    Null null;
} Json_Value;

typedef
struct {
    char *key;
    Json_Value value_as;
    Json_Type type;
} Key_Value;

typedef
struct {
    Json_Value item_as;
    Json_Type type;
} Item;

#define MAX_OBJECT_ENTRIES 256
#define HASHMAP_INDEX(h) (h & (MAX_OBJECT_ENTRIES - 1))

struct Json_Object {
    Key_Value entries[MAX_OBJECT_ENTRIES];
    unsigned int table[MAX_OBJECT_ENTRIES];
    unsigned int len;
};

struct Json_Array {
    Item *items;
    unsigned int capacity;
    unsigned int len;
};

typedef
struct {
    union {
        Json_Object *object;
        Json_Array *array; /* TODO: add array type */
    } as;
    Json_Type type;
} Json;

long int hash_string(const char *str, int *op_len) {
    long int h = 0;
    int len = 0;
    char c;
    while ((c = *str) != '\0') {
        h = ((h << 5) - h) + (unsigned char)c;
        str++; len++;
    }

    if (op_len) {
        *op_len = len;
    }

    return h;
}

//trying to use a hash map //////
void hm_put(Json_Object *object, const char *key, Json_Value value, Json_Type type) {
    int len = 0;
    long int h = hash_string(key, &len);

    unsigned int *i = &object->table[HASHMAP_INDEX(h)];
    while (*i > 0 && memcmp(object->entries[*i].key, key, len) != 0) {
        h++;
        i = &object->table[HASHMAP_INDEX(h)];
    }

    if (*i == 0) {
        *i = object->len++;
        object->entries[*i].type = type;
        object->entries[*i].key = malloc(sizeof(char) * len);
        strncpy(object->entries[*i].key, key, len);
    } else {
        panic("TODO: maybe enable updating to another type");
    }

    switch(type) {
        case JSON_STRING: {
            Json_String *json_str = &object->entries[*i].value_as.string;
            object->entries[*i].type = JSON_STRING;

            unsigned int str_len = value.string.len;
            if (json_str->content == NULL) {
                json_str->content = malloc(sizeof(char) * str_len);
                if (json_str->content == NULL) {
                    panic("Out of memory");
                }

                json_str->len = str_len;
                if (str_len) {
                    *json_str->content = '\0';
                    strncat(json_str->content, value.string.content, str_len);
                }

                return; /* new entry added */
            }

            if (str_len > json_str->len) {
                json_str->content = realloc(json_str->content, str_len);
            }

            json_str->len = value.string.len;
            *json_str->content = '\0';
            strncat(json_str->content, value.string.content, str_len);
        } break;

        case JSON_NUMBER: {
            object->entries[*i].type = JSON_NUMBER;
            object->entries[*i].value_as.number = value.number;
        } break;

        case JSON_BOOLEAN: {
            object->entries[*i].type = JSON_BOOLEAN;
            object->entries[*i].value_as.boolean = value.boolean;
        } break;

        case JSON_NULL: {
            object->entries[*i].type = JSON_NULL;
            object->entries[*i].value_as.null = NULL;
        } break;

        case JSON_OBJECT: {
            object->entries[*i].type = JSON_OBJECT;
            Json_Object *old_object = object->entries[*i].value_as.object;
            if (old_object != NULL) {
                object->entries[*i].value_as.object = value.object;
                free(old_object);
            } else {
                object->entries[*i].value_as.object = value.object;
            }
        } break;

        case JSON_ARRAY: {
            object->entries[*i].type = JSON_ARRAY;
            Json_Array *old_array = object->entries[*i].value_as.array;
            if (old_array != NULL) {
                object->entries[*i].value_as.array = value.array;
                free(old_array);
            } else {
                object->entries[*i].value_as.array = value.array;
            }
        } break;

        default: {
            panic("Invalid Json Type %s", JSON_TYPE_DESCRIPTION[type]);
        }
    }
}

unsigned int json_geti(Json_Object *object, const char *key) {
    int len = 0;
    long int h = hash_string(key, &len);

    unsigned int *i = &object->table[HASHMAP_INDEX(h)];
    while (*i > 0 && memcmp(object->entries[*i].key, key, len) != 0) {
        h++;
        i = &object->table[HASHMAP_INDEX(h)];
    }

    return *i;
}

void json_append(Json_Array *array, Json_Value value, Json_Type type) {
    const int growth_factor = 2;
    if (array == NULL) {
        panic("array is a null pointer. TODO: simplify memory management");
    }

    if (array->len >= array->capacity) {
        unsigned int new_capacity = array->capacity*growth_factor;
        array->items = realloc(array->items, new_capacity);
        if (array->items == NULL) {
            panic("could not reallocate memory for the array."
                  "Array current size is %d, failed realoc for size %d",
                  array->capacity, new_capacity);
        }

        array->capacity = new_capacity;
    }

    unsigned int i = array->len++;
    switch(type) {
        case JSON_STRING: {
            array->items[i].type = JSON_STRING;
            Json_String *json_str = &array->items[i].item_as.string;
            assert(json_str->content == NULL);

            unsigned int str_len = value.string.len;
            json_str->content = malloc(sizeof(char) * str_len);
            if (json_str->content == NULL) {
                panic("Out of memory");
            }

            json_str->len = str_len;
            if (str_len) {
                *json_str->content = '\0';
                strncat(json_str->content, value.string.content, str_len);
            }
        } break;

        case JSON_NUMBER: {
            array->items[i].type = JSON_NUMBER;
            array->items[i].item_as.number = value.number;
        } break;

        case JSON_BOOLEAN: {
            array->items[i].type = JSON_BOOLEAN;
            array->items[i].item_as.boolean = value.boolean;
        } break;

        case JSON_NULL: {
            array->items[i].type = JSON_NULL;
            array->items[i].item_as.null = NULL;
        } break;

        case JSON_OBJECT: {
            array->items[i].type = JSON_OBJECT;
            array->items[i].item_as.object = value.object;
        } break;

        case JSON_ARRAY: {
            panic("TODO: json array inside json arrays is not implemented");
        } break;

        default: {
            panic("Invalid Json Type %s", JSON_TYPE_DESCRIPTION[type]);
        }
    }
}

/////////////////////////////////
Json_Array *alloc_array(void) {
    Json_Array *array = malloc(sizeof(Json_Array));
    array->items = malloc(sizeof(Json_Value) * 16);
    array->capacity = 16;
    array->len = 0;
    return array;
}

void parse_string(Json_String *str, char c_str[MAX_STR_LEN]) {
    if (str == NULL) {
        panic("Caller must provide a valid pointer to Json_String");
    }

    unsigned int len = 0;
    while(len < MAX_STR_LEN && c_str[len] != '\0') len++;
    str->content = c_str;
    str->len = len;
}

void parse_object(Json_Object *object, Json_Tokenizer *tokenizer) {
    double mstrtod(char *value);
    void parse_array(Json_Array *array, Json_Tokenizer *tokenizer);

    char buffer[MAX_STR_LEN];

    int cnt = 0;
    do {
        THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(tokenizer);

        if (tokenizer->token->type != TOKEN_STRING) {
            panic("Expected TOKEN_STRING find %s", token_desc(tokenizer->token->type));
        }

        memmove(buffer, tokenizer->token->value, MAX_STR_LEN);

        THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(tokenizer);

        if (tokenizer->token->type != TOKEN_COLON) {
            panic("Expected `:` find %s", tokenizer->token->value);
        }

        THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(tokenizer);

        switch(tokenizer->token->type) {
            case TOKEN_STRING: {
                Json_Value value = {0};
                Json_String str = {0};
                parse_string(&str, tokenizer->token->value);
                value.string.content = str.content;
                value.string.len = str.len;
                hm_put(object, buffer, value, JSON_STRING);
            } break;

            case TOKEN_NULL: {
                Json_Value value = {0};
                value.null = NULL;
                hm_put(object, buffer, value, JSON_NULL);
            } break;

            case TOKEN_NUMBER: {
                Json_Value value = {0};
                log_debug(FMT_TOKEN, ARG_TOKEN(tokenizer->token));
                value.number = mstrtod(tokenizer->token->value);
                hm_put(object, buffer, value, JSON_NUMBER);
            } break;

            case TOKEN_BOOLEAN: {
                Json_Value value = {0};
                if (tokenizer->token->value[0] == 't') {
                    value.boolean = true;
                } else {
                    value.boolean = false;
                }

                hm_put(object, buffer, value, JSON_BOOLEAN);
            } break;

            case TOKEN_OPCBRAKT: {
                Json_Value value = {0};
                value.object = malloc(sizeof(Json_Object));;

                hm_put(object, buffer, value, JSON_OBJECT);

                parse_object(value.object, tokenizer);
            } break;

            case TOKEN_OPBRAKT: {
                Json_Value value = {0};
                value.array = alloc_array();
                parse_array(value.array, tokenizer);
                hm_put(object, buffer, value, JSON_ARRAY);
                break;
            }

            default: {
                panic("Not a valid token "FMT_TOKEN, ARG_TOKEN(tokenizer->token));
            }
        }

        THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(tokenizer);

    } while (cnt++ < MAX_OBJECT_ENTRIES && tokenizer->token->type == TOKEN_COMMA);

    if (tokenizer->token->type != TOKEN_CLCBRAKT) {
        panic("expected `}`, find %s", tokenizer->token->value);
    }
}

void parse_array(Json_Array *array, Json_Tokenizer *tokenizer) {
    double mstrtod(char *value);

    do {
        THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(tokenizer);

        Json_Value value = {0};
        switch(tokenizer->token->type) {
            case TOKEN_STRING: {
                Json_String str = {0};
                parse_string(&str, tokenizer->token->value);
                value.string.content = str.content;
                value.string.len = str.len;
                json_append(array, value, JSON_STRING);
            } break;

            case TOKEN_NULL: {
                value.null = NULL;
                json_append(array, value, JSON_NULL);
            } break;

            case TOKEN_NUMBER: {
                value.number = mstrtod(tokenizer->token->value);
                json_append(array, value, JSON_NUMBER);
            } break;

            case TOKEN_BOOLEAN: {
                if (tokenizer->token->value[0] == 't') {
                    value.boolean = true;
                } else {
                    value.boolean = false;
                }

                json_append(array, value, JSON_BOOLEAN);
            } break;

            case TOKEN_OPCBRAKT: {
                Json_Value value = {0};
                value.object = malloc(sizeof(Json_Object));;
                parse_object(value.object, tokenizer);
                json_append(array, value, JSON_OBJECT);
            } break;

            case TOKEN_OPBRAKT: {
                panic("TODO: array not implemented");
                break;
            }

            default: {
                panic("Not a valid token "FMT_TOKEN, ARG_TOKEN(tokenizer->token));
            }
        }

        THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(tokenizer);

    } while (tokenizer->token->type == TOKEN_COMMA);

    if (tokenizer->token->type != TOKEN_CLBRAKT) {
        panic("expected `]`, find %s", tokenizer->token->value);
    }
}

void parse_json(Json *root, Json_Tokenizer *tokenizer) {
    THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(tokenizer);

    if (root->type != 0) {
        panic("root type must not be set before parsing");
    }

    switch (tokenizer->token->type) {
        case TOKEN_OPCBRAKT: {
            root->type = JSON_OBJECT;
            root->as.object = malloc(sizeof(Json_Object));
            parse_object(root->as.object, tokenizer);
        } break;

        case TOKEN_OPBRAKT: {
            root->type = JSON_ARRAY;
            root->as.array = alloc_array();
            parse_array(root->as.array, tokenizer);
        } break;

        default: {
            panic("Expected `[` or `{` finded "FMT_TOKEN, ARG_TOKEN(tokenizer->token));
        } break;
    }
}

void log_init(Log_Config *config) {
    if (config == NULL) {
        log_config.fdebug = stderr;
        log_config.finfo = stdout;
        log_config.fwarning = stdout;
        log_config.fdebug = stdout;
        log_config.ferror = stdout;
        log_config.max_size = 256;
        log_config.debug = true;
    } else {
        log_config.fdebug = config->fdebug == NULL ? stderr : config->fdebug;
        log_config.fwarning = config->fwarning == NULL ? stdout : config->fwarning;
        log_config.finfo = config->finfo == NULL ? stdout : config->finfo;
        log_config.fdebug = config->fdebug == NULL ? stdout : config->fdebug;
        log_config.ferror = config->ferror == NULL ? stdout : config->ferror;
        log_config.max_size = config->max_size == 0 ? 256 : config->max_size;
        log_config.debug = config->debug;
    }
}

#define _log_wrapper(file, pattern, fmt) {\
    assert(file != NULL && "file in logging configuration not provided. Maybe you forgot to call `log_init(config);`"); \
    va_list valist; \
    va_start(valist, fmt); \
    char info[log_config.max_size]; \
    snprintf(info, log_config.max_size, pattern"%s\n", fmt); \
    vfprintf(file, info, valist); \
    va_end(valist); \
} \

void log_info(char *fmt, ...) {
    _log_wrapper(log_config.finfo, "[ INFO ] ", fmt)
}

void log_error(const char *fmt, ...) {
    _log_wrapper(log_config.finfo, "[ ERROR ] ", fmt)
}

void log_warning(const char *fmt, ...) {
    _log_wrapper(log_config.finfo, "[ WARNING ] ", fmt)
}

void log_debug(const char *fmt, ...) {
    if (!log_config.debug) return;
    _log_wrapper(log_config.finfo, "[ DEBUG ] ", fmt)
}

void panic(const char *fmt, ...) {
    _log_wrapper(log_config.ferror, "[ ERROR ] ", fmt)
    exit(1);
}

/////////////
Json json = {0};
int main(void) {
    Log_Config config = {
        .debug = false,
    };

    log_init(&config);

    char *j = "[{\"teste\": \"teste1\"}]";
    Json_Tokenizer *tokenizer = malloc(sizeof(Json_Tokenizer));
    tokenizer->token = malloc(sizeof(Json_Token));
    tokenizer->json_str = j;
    tokenizer->cursor = 0;

    parse_json(&json, tokenizer);

    Json_Array *array = json.as.array;
    Json_Object *obj = array->items[0].item_as.object;
    unsigned int i = json_geti(obj, "teste");
    printf("%s\n", obj->entries[i].value_as.string.content);

    return 0;
}

/*
Extended version of power function that can work
for double x and negative y

stealed from: https://stackoverflow.com/questions/26860574/pow-implementation-in-cmath-and-efficient-replacement
*/
double powerd(double x, int y)
{
    double temp;
    if (y == 0)
    return 1;
    temp = powerd(x, y / 2);
    if ((y % 2) == 0) {
        return temp * temp;
    } else {
        if (y > 0)
            return x * temp * temp;
        else
            return (temp * temp) / x;
    }
}


/*
Parses a json number string to a C double value
*/
double mstrtod(char *value) {
    double number = 0.0;
    int temp_n = 0;
    int sign = 1;
    char c = *(value++);

    if (c == '-' || c == '+') {
        sign *= (c == '-') ? -1 : 1;
        c = *(value++);
    }

    if (c == '\0') {
        panic("Expected a number, find end of input");
    }

    /* collecting base */
    while (c != '\0' && isdigit(c)) {
        temp_n *= 10;
        temp_n += (c - '0');
        c = *(value++);
    }


    /* collecting mantissa */
    int mantissa = 1;
    if (c == '.') {
        c = *(value++);
        while(c != '\0' && isdigit(c)) {
            temp_n *= 10;
            temp_n += (c - '0');
            c = *(value++);
            mantissa *= 10;
        }
    }

    /* saving result */
    number = (double) (sign*temp_n)/mantissa;

    /* collecting expoent */
    int expoent = 0;
    int exp_sign = 1;
    if (c == 'e') {
        c = *(value++);
        if (c == '-' || c == '+') {
            exp_sign *= (c == '-') ? -1 : 1;
            c = *(value++);
        }

        while(c != '\0' && isdigit(c)) {
            expoent *= 10;
            expoent += (c - '0');
            c = *(value++);
        }

        expoent *= exp_sign;
    }

    return number *= powerd(10.0, expoent);;
}


// Copyright 2024 Jonatha Willian dos Santos

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the “Software”), to deal in
// the Software without restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
// Software, and to permit persons to whom the Software is furnished to do so, subject
// to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies
// or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.