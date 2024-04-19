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
};

_Static_assert(
    ARRAY_SIZE(TOKEN_DESCRIPTION) == _TOTAL_TOKENS,
    "assert that you have implemented the description of all tokens"
);

// TODO: check max string len and max key len
#define MAX_STR_LEN 1024

typedef
union {
    char str[MAX_STR_LEN];
    double number;
} Token_Value;

typedef
struct {
    Token_Value as;
    Token_Type type;
} Json_Token;

#define FMT_TOKEN "( %s ) => %s"
#define ARG_TOKEN(t) token_value((t)), token_desc(((t)->type))

char *token_desc(Token_Type token_type) {
    return TOKEN_DESCRIPTION[token_type];
}

char *token_value(Json_Token *token) {
    if (token->type == TOKEN_NUMBER) {
        char *out = malloc(sizeof(char)*256);
        snprintf(out, 256, "%f", token->as.number);
        return out;
    } else {
        return token->as.str;
    }
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
            tokenizer->token->as.str[i++] = c;
        }

        tokenizer->token->as.str[i] = '\0';
        tokenizer->token->type = TOKEN_STRING;
        return 0; /* token is string, nothing to do anymore */
    }

    /* is number */
    if (c == '-' || isdigit(c)) {
        double number = 0.0;
        int temp_n = 0;

        int sign = 1;
        if (c == '-') {
            sign = -1;
            c = next_char(tokenizer);
        }
        if (c == EOJ) {
            panic("Expected a number, find end of input");
        }

        /* collecting the base */
        while (c != EOJ && isdigit(c)) {
            temp_n *= 10;
            temp_n += (c - '0');
            c = next_char(tokenizer);
        }

        /* collecting the mantissa */
        int mantissa = 1;
        if (c == '.') {
            c = next_char(tokenizer);
            while(c != EOJ && isdigit(c)) {
                temp_n *= 10;
                temp_n += (c - '0');
                c = next_char(tokenizer);
                mantissa *= 10;
            }
        }

        /* saving result */
        number = (double) (sign*temp_n)/mantissa;

        /* collecting expoent */
        int expoent = 0;
        if (c == 'e') {
            int exp_sign = 1;
            c = next_char(tokenizer);
            if (c == '-' || c == '+') {
                exp_sign *= (c == '-') ? -1 : 1;
                c = next_char(tokenizer);
            }

            while(c != EOJ && isdigit(c)) {
                expoent *= 10;
                expoent += (c - '0');
                c = next_char(tokenizer);
            }

            expoent *= exp_sign;
        }

        if (expoent) {
            number *= powerd(10.0, expoent);
        }

        tokenizer->token->as.number = number;
        tokenizer->token->type = TOKEN_NUMBER;
        tokenizer->cursor--;
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

    tokenizer->token->as.str[i++] = c;
    tokenizer->token->as.str[i] = '\0';

    return 0;
}

#define THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(t) if (next_token(t) == -1) panic("unexpected end of input");

//// parser
typedef
enum {
    JSON_ARRAY       ,
    JSON_OBJECT      ,
    JSON_STRING      ,
    JSON_NUMBER      ,
    _TOTAL_JSON_TYPES,
} Json_Type;

char* JSON_TYPE_DESCRIPTION[] = {
    [JSON_ARRAY]  = "JSON_ARRAY" ,
    [JSON_OBJECT] = "JSON_OBJECT",
    [JSON_STRING] = "JSON_STRING",
    [JSON_NUMBER] = "JSON_NUMBER",
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

typedef struct Object Object;

typedef
union {
    Object *object;
    Json_String string;
    double number;
} Json_Value;

typedef
struct {
    char *key;
    Json_Value value_as;
    Json_Type type;
} Key_Value;

#define MAX_OBJECT_ENTRIES 256
#define HASHMAP_INDEX(h) (h & (MAX_OBJECT_ENTRIES - 1))

struct Object {
    Key_Value entries[MAX_OBJECT_ENTRIES];
    unsigned int table[MAX_OBJECT_ENTRIES];
    unsigned int len;
};

typedef
struct {
    union {
        Object *object;
        /* TODO: add array type */
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
void hm_put(Object *object, const char *key, Json_Value value, Json_Type type) {
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

        case JSON_OBJECT: {
            object->entries[*i].type = JSON_OBJECT;
            Object *old_object = object->entries[*i].value_as.object;
            if (old_object != NULL) {
                object->entries[*i].value_as.object = value.object;
                free(old_object);
            } else {
                object->entries[*i].value_as.object = value.object;
            }
        } break;

        default: {
            panic("Invalid Json Type %s", JSON_TYPE_DESCRIPTION[type]);
        }
    }
}

unsigned int json_geti(Object *object, const char *key) {
    int len = 0;
    long int h = hash_string(key, &len);

    unsigned int *i = &object->table[HASHMAP_INDEX(h)];
    while (*i > 0 && memcmp(object->entries[*i].key, key, len) != 0) {
        h++;
        i = &object->table[HASHMAP_INDEX(h)];
    }

    return *i;
}

/////////////////////////////////
char buffer[MAX_STR_LEN];

void parse_string(Json_String *str, char c_str[MAX_STR_LEN]) {
    if (str == NULL) {
        panic("Caller must provide a valid pointer to Json_String");
    }

    unsigned int len = 0;
    while(len < MAX_STR_LEN && c_str[len] != '\0') len++;
    str->content = c_str;
    str->len = len;
}

void parse_object(Object *object, Json_Tokenizer *tokenizer) {
    int cnt = 0;
    do {
        THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(tokenizer);

        if (tokenizer->token->type != TOKEN_STRING) {
            panic("Expected TOKEN_STRING find %s", token_desc(tokenizer->token->type));
        }

        memmove(buffer, tokenizer->token->as.str, MAX_STR_LEN);

        THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(tokenizer);

        if (tokenizer->token->type != TOKEN_COLON) {
            panic("Expected `:` find %s", tokenizer->token->as.str);
        }

        THROW_IF_NEXT_TOKEN_IS_END_OF_INPUT(tokenizer);

        switch(tokenizer->token->type) {
            case TOKEN_STRING: {
                Json_Value value = {0};
                Json_String str = {0};
                parse_string(&str, tokenizer->token->as.str);
                value.string.content = str.content;
                value.string.len = str.len;
                hm_put(object, buffer, value, JSON_STRING);
            } break;

            case TOKEN_OPCBRAKT: {
                Json_Value value = {0};
                value.object = malloc(sizeof(Object));;

                // its important to do the put before parsing the pointer
                // so other nested objects can use the buffer
                hm_put(object, buffer, value, JSON_OBJECT);

                parse_object(value.object, tokenizer);
            } break;

            case TOKEN_NUMBER: {
                Json_Value value = {0};
                value.number = tokenizer->token->as.number;
                hm_put(object, buffer, value, JSON_NUMBER);
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

    } while (cnt++ < MAX_OBJECT_ENTRIES && tokenizer->token->type == TOKEN_COMMA);

    if (tokenizer->token->type != TOKEN_CLCBRAKT) {
        log_warning("data after end of json ignored");
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
            root->as.object = malloc(sizeof(Object));
            parse_object(root->as.object, tokenizer);
        } break;

        case TOKEN_OPBRAKT: {
            panic("TODO: array not implemented");
            break;
        }

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
    char *j = "{\"hello\": 10e-2}";
    log_init(NULL);

    Json_Tokenizer *tokenizer = malloc(sizeof(Json_Tokenizer));
    tokenizer->token = malloc(sizeof(Json_Token));
    tokenizer->json_str = j;
    tokenizer->cursor = 0;

    parse_json(&json, tokenizer);

    Object *obj = json.as.object;
    unsigned int i = json_geti(obj, "hello");
    printf("{\"%s\": %lf}\n", obj->entries[i].key, obj->entries[i].value_as.number);

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