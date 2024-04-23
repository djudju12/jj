#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

/////// Logging
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
void log_test(const char *fmt, ...);
void panic(const char *fmt, ...);           // exits with exit code 1 after printing the message
//////

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// errors definitions
typedef enum {
    JSON_NO_ERROR = 0           ,
    ERR_UNCLOSED_STRING         ,
    ERR_UNEXPECTED_CHAR         ,
    ERR_MALFORMED_JSON          ,
    ERR_NOT_ENOUGH_MEMORY       ,
    ERR_EXPECTED_OBJECT_OR_ARRAY,
    ERR_UNEXPECTED_END_OF_INPUT ,
    _TOTAL_ERRORS               ,
} JSON_ERROR;

char *JSON_ERROR_DESC[] = {
    [JSON_NO_ERROR]                = "no error"                          ,
    [ERR_UNCLOSED_STRING]          = "unclosed string"                   ,
    [ERR_UNEXPECTED_CHAR]          = "unexpected character"              ,
    [ERR_MALFORMED_JSON]           = "malformed json"                    ,
    [ERR_NOT_ENOUGH_MEMORY]        = "not enough memory to do allocation",
    [ERR_EXPECTED_OBJECT_OR_ARRAY] = "expected `{` or `[`"               ,
    [ERR_UNEXPECTED_END_OF_INPUT]  = "unexpected end of input"           ,
};

_Static_assert(
    ARRAY_SIZE(JSON_ERROR_DESC) == _TOTAL_ERRORS,
    "assert that you have implemented the description of all the `JSON_ERROR`'s"
);

char *json_error_desc(JSON_ERROR error) {
    return JSON_ERROR_DESC[error];
}

//
typedef
enum {
    TOKEN_BOOLEAN ,
    TOKEN_CLBRAKT ,
    TOKEN_CLCBRAKT,
    TOKEN_COLON   ,
    TOKEN_COMMA   ,
    TOKEN_NULL    ,
    TOKEN_NUMBER  ,
    TOKEN_OPBRAKT ,
    TOKEN_OPCBRAKT,
    TOKEN_STRING  ,
    _TOTAL_TOKENS ,
} Token_Type;

char* TOKEN_DESCRIPTION[] = {
    [TOKEN_BOOLEAN]   = "TOKEN_BOOLEAN" ,
    [TOKEN_CLBRAKT]   = "TOKEN_CLBRAKT" ,
    [TOKEN_CLCBRAKT]  = "TOKEN_CLCBRAKT",
    [TOKEN_COLON]     = "TOKEN_COLON"   ,
    [TOKEN_COMMA]     = "TOKEN_COMMA"   ,
    [TOKEN_NULL]      = "TOKEN_NULL"    ,
    [TOKEN_NUMBER]    = "TOKEN_NUMBER"  ,
    [TOKEN_OPBRAKT]   = "TOKEN_OPBRAKT" ,
    [TOKEN_OPCBRAKT]  = "TOKEN_OPCBRAKT",
    [TOKEN_STRING]    = "TOKEN_STRING"  ,
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

int next_token(Json_Tokenizer *tokenizer, JSON_ERROR *err) {
    double powerd(double x, int y);

    char next_char(Json_Tokenizer *tokenizer);
    char c = next_char(tokenizer);

    while (c != EOJ && isspace(c)) {
        c = next_char(tokenizer);
    }

    if (c == EOJ) return -1;

    unsigned int i = 0;

    switch (c) {
        /* is string */
        case '"': {
            while (c != EOJ && (c = next_char(tokenizer)) != '"') {
                tokenizer->token->value[i++] = c;
            }

            if (c == EOJ) {
                *err = ERR_UNCLOSED_STRING;
                return -1;
            }

            tokenizer->token->type = TOKEN_STRING;
        } break;

        /* is number */
        case '+': case '-':
        case '0': case '1':
        case '2': case '3':
        case '4': case '5':
        case '6': case '7':
        case '8': case '9':
        {
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

            if (c != EOJ) {
                tokenizer->cursor--;
            }

            tokenizer->token->type = TOKEN_NUMBER;
        } break;

        /* is false */
        case 'f': {
            if (next_char(tokenizer) != 'a' ||
                next_char(tokenizer) != 'l' ||
                next_char(tokenizer) != 's' ||
                next_char(tokenizer) != 'e')
            {
                *err = ERR_UNEXPECTED_CHAR;
                return 0;
            }

            *tokenizer->token->value = '\0';
            strncat(tokenizer->token->value, "false", 6);
            i = 6;

            tokenizer->token->type = TOKEN_BOOLEAN;
        } break;

        /* is true */
        case 't': {
            if (next_char(tokenizer) != 'r' ||
                next_char(tokenizer) != 'u' ||
                next_char(tokenizer) != 'e')
            {
                *err = ERR_UNEXPECTED_CHAR;
                return 0;
            }

            *tokenizer->token->value = '\0';
            strncat(tokenizer->token->value, "true", 5);
            i = 5;

            tokenizer->token->type = TOKEN_BOOLEAN;
        } break;

        /* is null */
        case 'n': {
            if (next_char(tokenizer) != 'u' ||
                next_char(tokenizer) != 'l' ||
                next_char(tokenizer) != 'l')
            {
                *err = ERR_UNEXPECTED_CHAR;
                return 0;
            }

            tokenizer->token->type = TOKEN_NULL;
        } break;

        case '{': {
            tokenizer->token->type = TOKEN_OPCBRAKT;
            tokenizer->token->value[i++] = c;
        } break;

        case '}': {
            tokenizer->token->type = TOKEN_CLCBRAKT;
            tokenizer->token->value[i++] = c;
        } break;

        case '[': {
            tokenizer->token->type = TOKEN_OPBRAKT;
            tokenizer->token->value[i++] = c;
        } break;

        case ']': {
            tokenizer->token->type = TOKEN_CLBRAKT;
            tokenizer->token->value[i++] = c;
        } break;

        case ':': {
            tokenizer->token->type = TOKEN_COLON;
            tokenizer->token->value[i++] = c;
        } break;

        case ',': {
            tokenizer->token->type = TOKEN_COMMA;
            tokenizer->token->value[i++] = c;
        } break;

        default: {
            *err = ERR_UNEXPECTED_CHAR;
            return 0;
        }
    }

    tokenizer->token->value[i] = '\0';
    log_debug(FMT_TOKEN, ARG_TOKEN(tokenizer->token));
    return 0;
}

//// parser
typedef void* Null;

typedef
enum {
    JSON_ARRAY = 0   ,
    JSON_BOOLEAN     ,
    JSON_NULL        ,
    JSON_NUMBER      ,
    JSON_OBJECT      ,
    JSON_STRING      ,
    _TOTAL_JSON_TYPES,
} Json_Type;

char *JSON_TYPE_DESCRIPTION[] = {
    [JSON_ARRAY]   = "JSON_ARRAY"  ,
    [JSON_BOOLEAN] = "JSON_BOOLEAN",
    [JSON_NULL]    = "JSON_NULL"   ,
    [JSON_NUMBER]  = "JSON_NUMBER" ,
    [JSON_OBJECT]  = "JSON_OBJECT" ,
    [JSON_STRING]  = "JSON_STRING" ,
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
    bool boolean;
    double number;
    Json_Array *array;
    Json_Object *object;
    Json_String string;
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

#define RETURN_ERROR_IF_END_OF_INPUT(tokenizer, err) if (next_token((tokenizer), (err)) == -1 && *(err) == JSON_NO_ERROR) return ERR_UNEXPECTED_END_OF_INPUT

JSON_ERROR hm_put(Json_Object *object, const char *key, Json_Value value, Json_Type type) {
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
    } else if (type != object->entries[*i].type) {
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
                    return ERR_NOT_ENOUGH_MEMORY;
                }

                json_str->len = str_len;
                *json_str->content = '\0';
                strncat(json_str->content, value.string.content, str_len);

                return JSON_NO_ERROR; /* new entry added */
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

    return 0;
}

int json_geti(Json_Object *object, const char *key) {
    int len = 0;
    long int h = hash_string(key, &len);

    unsigned int *i = &object->table[HASHMAP_INDEX(h)];
    while (*i > 0 && memcmp(object->entries[*i].key, key, len) != 0) {
        h++;
        i = &object->table[HASHMAP_INDEX(h)];
    }

    if (*i == 0 && memcmp(object->entries[*i].key, key, len) != 0) return -1;

    return *i;
}

JSON_ERROR json_append(Json_Array *array, Json_Value value, Json_Type type) {
    const int growth_factor = 2;
    if (array == NULL) {
        panic("array is a null pointer. TODO: simplify memory management");
    }

    if (array->len >= array->capacity) {
        unsigned int new_capacity = array->capacity*growth_factor;
        array->items = realloc(array->items, new_capacity);
        if (array->items == NULL) {
            return ERR_NOT_ENOUGH_MEMORY;
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
                return ERR_NOT_ENOUGH_MEMORY;
            }

            json_str->len = str_len;
            *json_str->content = '\0';
            strncat(json_str->content, value.string.content, str_len);
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

    return 0;
}

/////////////////////////////////
Json_Array *alloc_array(JSON_ERROR *err) {
    Json_Array *array = malloc(sizeof(Json_Array));
    if (array == NULL) {
        *err = ERR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    array->items = malloc(sizeof(Json_Value) * 16);
    if (array->items == NULL) {
        free(array);
        *err = ERR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

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

JSON_ERROR parse_object(Json_Object *object, Json_Tokenizer *tokenizer) {
    double mstrtod(char *value);
    JSON_ERROR parse_array(Json_Array *array, Json_Tokenizer *tokenizer);

    JSON_ERROR err = 0;
    char buffer[MAX_STR_LEN];

    int cnt = 0;
    do {
        RETURN_ERROR_IF_END_OF_INPUT(tokenizer, &err);
        if (err != JSON_NO_ERROR) {
            return err;
        }

        if (tokenizer->token->type != TOKEN_STRING) {
            return ERR_MALFORMED_JSON;
        }

        memmove(buffer, tokenizer->token->value, MAX_STR_LEN);

        RETURN_ERROR_IF_END_OF_INPUT(tokenizer, &err);
        if (err != JSON_NO_ERROR) {
            return err;
        }

        if (tokenizer->token->type != TOKEN_COLON) {
            return ERR_MALFORMED_JSON;
        }

        RETURN_ERROR_IF_END_OF_INPUT(tokenizer, &err);
        if (err != JSON_NO_ERROR) {
            return err;
        }

        switch(tokenizer->token->type) {
            case TOKEN_STRING: {
                Json_Value value = {0};
                Json_String str = {0};
                parse_string(&str, tokenizer->token->value);
                value.string.content = str.content;
                value.string.len = str.len;
                err = hm_put(object, buffer, value, JSON_STRING);
                if (err != JSON_NO_ERROR) {
                    return err;
                }

            } break;

            case TOKEN_NULL: {
                Json_Value value = {0};
                value.null = NULL;
                err = hm_put(object, buffer, value, JSON_NULL);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
            } break;

            case TOKEN_NUMBER: {
                Json_Value value = {0};
                log_debug(FMT_TOKEN, ARG_TOKEN(tokenizer->token));
                value.number = mstrtod(tokenizer->token->value);
                err = hm_put(object, buffer, value, JSON_NUMBER);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
            } break;

            case TOKEN_BOOLEAN: {
                Json_Value value = {0};
                value.boolean = tokenizer->token->value[0] == 't' ? true : false;
                err = hm_put(object, buffer, value, JSON_BOOLEAN);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
            } break;

            case TOKEN_OPCBRAKT: {
                Json_Value value = {0};
                value.object = malloc(sizeof(Json_Object));;
                if (value.object == NULL) {
                    return ERR_NOT_ENOUGH_MEMORY;
                }

                err = hm_put(object, buffer, value, JSON_OBJECT);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
                err = parse_object(value.object, tokenizer);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
            } break;

            case TOKEN_OPBRAKT: {
                Json_Value value = {0};
                value.array = alloc_array(&err);
                if (value.array == NULL) {
                    return err;
                }

                err = parse_array(value.array, tokenizer);
                if (err != JSON_NO_ERROR) {
                    return err;
                }

                err = hm_put(object, buffer, value, JSON_ARRAY);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
                break;
            }

            default: {
                panic("Invalid token %s", token_desc(tokenizer->token->type));
            }
        }

        RETURN_ERROR_IF_END_OF_INPUT(tokenizer, &err);
        if (err != JSON_NO_ERROR) {
            return err;
        }


    } while (cnt++ < MAX_OBJECT_ENTRIES && tokenizer->token->type == TOKEN_COMMA);

    if (tokenizer->token->type != TOKEN_CLCBRAKT) {
        return ERR_MALFORMED_JSON;
    }

    return 0;
}

JSON_ERROR parse_array(Json_Array *array, Json_Tokenizer *tokenizer) {
    double mstrtod(char *value);
    JSON_ERROR err = 0;

    do {
        RETURN_ERROR_IF_END_OF_INPUT(tokenizer, &err);
        if (err != JSON_NO_ERROR) {
            return err;
        }

        Json_Value value = {0};
        switch(tokenizer->token->type) {
            case TOKEN_STRING: {
                Json_String str = {0};
                parse_string(&str, tokenizer->token->value);
                value.string.content = str.content;
                value.string.len = str.len;
                err = json_append(array, value, JSON_STRING);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
            } break;

            case TOKEN_NULL: {
                value.null = NULL;
                err = json_append(array, value, JSON_NULL);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
            } break;

            case TOKEN_NUMBER: {
                value.number = mstrtod(tokenizer->token->value);
                err = json_append(array, value, JSON_NUMBER);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
            } break;

            case TOKEN_BOOLEAN: {
                if (tokenizer->token->value[0] == 't') {
                    value.boolean = true;
                } else {
                    value.boolean = false;
                }

                err = json_append(array, value, JSON_BOOLEAN);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
            } break;

            case TOKEN_OPCBRAKT: {
                Json_Value value = {0};
                value.object = malloc(sizeof(Json_Object));
                if (value.object == NULL) {
                    return ERR_NOT_ENOUGH_MEMORY;
                }

                err = parse_object(value.object, tokenizer);
                if (err != JSON_NO_ERROR) {
                    return err;
                }

                err = json_append(array, value, JSON_OBJECT);
                if (err != JSON_NO_ERROR) {
                    return err;
                }
            } break;

            case TOKEN_OPBRAKT: {
                panic("TODO: array not implemented");
                break;
            }

            default: {
                panic("Invalid token %s", token_desc(tokenizer->token->type));
            }
        }

        RETURN_ERROR_IF_END_OF_INPUT(tokenizer, &err);
        if (err != JSON_NO_ERROR) {
            return err;
        }

    } while (tokenizer->token->type == TOKEN_COMMA);

    if (tokenizer->token->type != TOKEN_CLBRAKT) {
        return ERR_MALFORMED_JSON;
    }

    return 0;
}

Json* parse_json(char *json_str, JSON_ERROR *err) {
    *err = 0;
    JSON_ERROR _parse_json(Json *root, Json_Tokenizer *tokenizer);
    Json *json = malloc(sizeof(Json));
    if (json == NULL) {
        *err = ERR_NOT_ENOUGH_MEMORY;
        goto CLEANUP;
    }

    Json_Tokenizer *tokenizer = malloc(sizeof(Json_Tokenizer));
    if (tokenizer == NULL) {
        *err = ERR_NOT_ENOUGH_MEMORY;
        goto CLEANUP;
    }

    tokenizer->cursor = 0;
    tokenizer->json_str = json_str;

    tokenizer->token = malloc(sizeof(Json_Token));
    if (tokenizer->token == NULL) {
        *err = ERR_NOT_ENOUGH_MEMORY;
        goto CLEANUP;
    }

    *err = _parse_json(json, tokenizer);
    if (*err != JSON_NO_ERROR) {
        goto CLEANUP;
    }

    free(tokenizer->token);
    free(tokenizer);
    return json;

CLEANUP:
    if (json) free(json);
    if (tokenizer) {
        if (tokenizer->token) free(tokenizer->token);
        free(tokenizer);
    }

    return NULL;
}

JSON_ERROR _parse_json(Json *root, Json_Tokenizer *tokenizer) {
    JSON_ERROR err = 0;

    next_token(tokenizer, &err);
    if (err != JSON_NO_ERROR) {
        return err;
    }

    if (root->type != 0) {
        panic("root type must not be set before parsing");
    }

    switch (tokenizer->token->type) {
        case TOKEN_OPCBRAKT: {
            root->type = JSON_OBJECT;
            root->as.object = malloc(sizeof(Json_Object));
            if (root->as.object == NULL) {
                return ERR_NOT_ENOUGH_MEMORY;
            }

            err = parse_object(root->as.object, tokenizer);
            if (err != JSON_NO_ERROR) {
                return err;
            }

        } break;

        case TOKEN_OPBRAKT: {
            root->type = JSON_ARRAY;
            root->as.array = alloc_array(&err);
            if (root->as.array == NULL) {
                return err;
            }

            err = parse_array(root->as.array, tokenizer);
            if (err != JSON_NO_ERROR) {
                return err;
            }

        } break;

        default: {
            return ERR_EXPECTED_OBJECT_OR_ARRAY;
        } break;
    }

    return 0;
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

void log_test(const char *fmt, ...) {
    _log_wrapper(log_config.finfo, "[ TEST ] ", fmt)
}

void log_debug(const char *fmt, ...) {
    if (!log_config.debug) return;
    _log_wrapper(log_config.finfo, "[ DEBUG ] ", fmt)
}

void panic(const char *fmt, ...) {
    _log_wrapper(log_config.ferror, "[ ERROR ] ", fmt)
    exit(1);
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