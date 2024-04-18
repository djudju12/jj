#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define THROW_NOT_IMPLEMENTED(msg) assert(0 && "TODO: Not Implemented "msg)
#define THROW_UNEXPECTED_END_OF_INPUT assert(0 && "unexpected end of input\n")

typedef
enum {
    TOKEN_OPBRAKT,
    TOKEN_CLBRAKT,
    TOKEN_STRING ,
    TOKEN_COLON  ,
    TOKEN_COMMA  ,
    _TOTAL_TOKENS,
} Token_Type;

char* TOKEN_DESCRIPTION[] = {
    [TOKEN_OPBRAKT]  = "TOKEN_OPBRAKT",
    [TOKEN_CLBRAKT]  = "TOKEN_CLBRAKT",
    [TOKEN_STRING]   = "TOKEN_STRING" ,
    [TOKEN_COLON]    = "TOKEN_COLON"  ,
    [TOKEN_COMMA]    = "TOKEN_COMMA"  ,
};

_Static_assert(
    ARRAY_SIZE(TOKEN_DESCRIPTION) == _TOTAL_TOKENS,
    "assert that you have implemented the description of all tokens"
);

char *token_desc(Token_Type token_type) {
    return TOKEN_DESCRIPTION[token_type];
}

#define MAX_STR_LEN 1024

typedef
struct {
    char value[MAX_STR_LEN];
    Token_Type type;
} Json_Token;

#define FMT_TOKEN "( %s ) => %s"
#define ARG_TOKEN(t) (t)->value, token_desc((t->type))

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
    char next_char (Json_Tokenizer *tokenizer);
    char c = next_char(tokenizer);

    while (c != EOJ && isspace(c)) c = next_char(tokenizer);
    if (c == EOJ) return -1;

    unsigned int i = 0;
    if (c == '"') {
        while (c != EOJ && (c = next_char(tokenizer)) != '"') {
            tokenizer->token->value[i++] = c;
        }

        tokenizer->token->value[i] = '\0';
        tokenizer->token->type = TOKEN_STRING;
        return 0;
    }

    switch (c) {
        case '{': tokenizer->token->type = TOKEN_OPBRAKT; break;
        case '}': tokenizer->token->type = TOKEN_CLBRAKT; break;
        case ':': tokenizer->token->type = TOKEN_COLON;   break;
        case ',': tokenizer->token->type = TOKEN_COMMA;   break;
        default: {
            fprintf(stderr, "Invalid token => %c\n", c);
            exit(1);
        }
    }

    tokenizer->token->value[i++] = c;
    tokenizer->token->value[i] = '\0';

    return 0;
}

//// parser
typedef
enum {
    JSON_ARRAY       ,
    JSON_OBJECT      ,
    JSON_STRING      ,
    _TOTAL_JSON_TYPES,
} Json_Type;

char* JSON_TYPE_DESCRIPTION[] = {
    [JSON_ARRAY]  = "JSON_ARRAY" ,
    [JSON_OBJECT] = "JSON_OBJECT",
    [JSON_STRING] = "JSON_STRING",
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
        assert(object->entries[*i].type == type && "TODO: maybe enable updating to another type");
    }

    switch(type) {
        case JSON_STRING: {
            Json_String *json_str = &object->entries[*i].value_as.string;
            object->entries[*i].type = JSON_STRING;

            unsigned int str_len = value.string.len;
            if (json_str->content == NULL) {
                json_str->content = malloc(sizeof(char) * str_len);
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

        case JSON_OBJECT: {
            THROW_NOT_IMPLEMENTED("JSON_OBJECT");
        } break;

        default: {
            THROW_NOT_IMPLEMENTED("hm_put");
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
void parse_string(Json_String *str, char c_str[MAX_STR_LEN]) {
    assert(str != NULL && "Caller must provide a valid pointer to Json_String");

    unsigned int len = 0;
    while(len < MAX_STR_LEN && c_str[len] != '\0') len++;
    assert(len);
    str->content = c_str;
    str->len = len;
}

char buffer[MAX_STR_LEN];
void parse_json(Json *root, Json_Tokenizer *tokenizer) {
    if (next_token(tokenizer) == -1) THROW_UNEXPECTED_END_OF_INPUT;

    assert(root->type == 0 && "root type must not be set before parsing");

    unsigned int cnt = 0;
    switch (tokenizer->token->type) {
        case TOKEN_OPBRAKT: {
            root->type = JSON_OBJECT;
            root->as.object = malloc(sizeof(Object));

            do {
                if (next_token(tokenizer) == -1) THROW_UNEXPECTED_END_OF_INPUT;
                if (tokenizer->token->type != TOKEN_STRING) {
                    fprintf(stderr, "Expected TOKEN_STRING find %s", token_desc(tokenizer->token->type));
                    exit(1);
                }

                memmove(buffer, tokenizer->token->value, MAX_STR_LEN);

                if (next_token(tokenizer) == -1) THROW_UNEXPECTED_END_OF_INPUT;
                if (tokenizer->token->type != TOKEN_COLON) {
                    fprintf(stderr, "Expected `:` find %s", tokenizer->token->value);
                    exit(1);
                }

                if (next_token(tokenizer) == -1) THROW_UNEXPECTED_END_OF_INPUT;

                switch(tokenizer->token->type) {
                    case TOKEN_STRING: {
                        Json_String str = {0};
                        parse_string(&str, tokenizer->token->value);
                        Json_Value value = {
                            .string = {
                                .content = str.content,
                                .len = str.len
                            }
                        };

                        hm_put(root->as.object, buffer, value, JSON_STRING);
                    } break;

                    default: {
                        fprintf(stderr, FMT_TOKEN"\n", ARG_TOKEN(tokenizer->token));
                        THROW_NOT_IMPLEMENTED("rest of types");
                    }
                }

                if (next_token(tokenizer) == -1) THROW_UNEXPECTED_END_OF_INPUT;
            } while (cnt++ < MAX_OBJECT_ENTRIES && tokenizer->token->type == TOKEN_COMMA);

            if (tokenizer->token->type != TOKEN_CLBRAKT) {
                fprintf(stderr, "data after end of json ignored\n");
            }

        } break;

        default: {
            THROW_NOT_IMPLEMENTED("Only object type is implemented");
        } break;
    }
}

Json json = {0};
int main(void) {
    char *j = "{\"hello\":\"world\", \"another\": \"key value\"}";

    Json_Tokenizer *tokenizer = malloc(sizeof(Json_Tokenizer));
    tokenizer->token = malloc(sizeof(Json_Token));
    tokenizer->json_str = j;
    tokenizer->cursor = 0;

    // example
    parse_json(&json, tokenizer);

    Object *map = json.as.object;
    unsigned int i = json_geti(map, "hello");
    fprintf(stdout,
        "%s: %s\n",
        map->entries[i].key,
        map->entries[i].value_as.string.content
    );

    i = json_geti(map, "another");
    fprintf(stdout,
        "%s: %s\n",
        map->entries[i].key,
        map->entries[i].value_as.string.content
    );

    return 0;
}