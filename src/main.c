#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define THROW_NOT_IMPLEMENTED(msg) assert(0 && "TODO: Not Implemented "msg)

#define THROW_UNEXPECTED_END_OF_INPUT { fprintf(stderr, "unexpected end of input\n"); assert(0); }

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
#define MAX_ENTRIES_JSON 256

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
    char s[MAX_STR_LEN];
    unsigned int len;
} Json_String;

typedef struct Object Object;

struct Object {
    Json_String key;
    union {
        Object *object;
        Json_String *string;
    } as;
    Json_Type type;
};

typedef
struct {
    union {
        Object *object[1024];
        /* TODO: add array type */
    } as;
    Json_Type type;
} Json;

void parse_object(Object *object, Json_Tokenizer *tokenizer);

void parse_json(Json *root, Json_Tokenizer *tokenizer) {
    if (next_token(tokenizer) == -1) THROW_UNEXPECTED_END_OF_INPUT

    assert(root->type == 0 && "root type must not be set before parsing");

    unsigned int cnt = 0;
    switch (tokenizer->token->type) {
        case TOKEN_OPBRAKT: {
            root->type = JSON_OBJECT;

            Object *object = malloc(sizeof(Object));

            parse_object(object, tokenizer);
            root->as.object[cnt++] = object;

            if (next_token(tokenizer) == -1) THROW_UNEXPECTED_END_OF_INPUT

            while (tokenizer->token->type == TOKEN_COMMA) {
                Object *object = malloc(sizeof(Object));
                root->as.object[cnt++] = object;
                parse_object(object, tokenizer);
                if (next_token(tokenizer) == -1) THROW_UNEXPECTED_END_OF_INPUT
            }

            if (tokenizer->token->type != TOKEN_CLBRAKT) {
                fprintf(stderr, "data after end of json ignored\n");
            }
        } break;

        default: {
            THROW_NOT_IMPLEMENTED("Only object type is implemented");
        } break;
    }
}

void parse_string(Json_String *str, char c_str[MAX_STR_LEN]) {
    if (str == NULL) {
        str = malloc(sizeof(Json_String));
    }

    unsigned int len = 0;
    while(len < MAX_STR_LEN && c_str[len] != '\0') {
        str->s[len] = c_str[len];
        len += 1;
    }

    str->len = len-1;
}

void parse_object(Object *object, Json_Tokenizer *tokenizer) {
    assert(next_token(tokenizer) != -1);
    if (tokenizer->token->type != TOKEN_STRING) {
        fprintf(stderr, "Expected %s find %s", token_desc(TOKEN_STRING), token_desc(tokenizer->token->type));
        exit(1);
    }

    parse_string(&object->key, tokenizer->token->value);

    if (next_token(tokenizer) == -1) THROW_UNEXPECTED_END_OF_INPUT

    if (tokenizer->token->type != TOKEN_COLON) {
        fprintf(stderr, "Expected %s find %s", token_desc(TOKEN_COLON), token_desc(tokenizer->token->type));
        exit(1);
    }

    if (next_token(tokenizer) == -1) THROW_UNEXPECTED_END_OF_INPUT

    switch(tokenizer->token->type) {
        case TOKEN_STRING: {
            object->as.string = malloc(sizeof(Json_String));
            parse_string(object->as.string, tokenizer->token->value);
            object->type = JSON_STRING;
        } break;

        default: {
            THROW_NOT_IMPLEMENTED("Only string type is implemented");
        }
    }
}

char *c_str(Json_String j_str) {
    char *s = malloc(sizeof(char) * j_str.len+1);
    memcpy(s, j_str.s, j_str.len+1);
    s[j_str.len+1] = '\0';
    return s;
}

#define MAKE_INDENT(n, d) (n)*(d), ' '

void print_object(Object *obj, int indent, int depth) {
    if (obj == NULL) return;

    fprintf(stdout, "%*c\"%s\": ", MAKE_INDENT(indent, depth), c_str(obj->key));
    switch (obj->type) {
        case JSON_OBJECT: {
            fprintf(stdout, "{\n");
            Object *nested_obj = obj->as.object;
            print_object(nested_obj, indent, depth + 1);
            fprintf(stdout, "%*c}\n", MAKE_INDENT(indent, depth));
        } break;

        case JSON_STRING: {
            Json_String *j_str = obj->as.string;
            fprintf(stdout, "\"%s\"", c_str(*j_str));
            break;
        }

        default: {
            THROW_NOT_IMPLEMENTED("add proper error handling for malformat json");
        }
    }
}

void print_json(Json *json, int indent) {
    switch(json->type) {
        case JSON_OBJECT: {
            fprintf(stdout, "{\n");
            unsigned int i = 0;
            Object *obj = json->as.object[i];
            while (1) {
                print_object(obj, indent, 1);
                obj = json->as.object[++i];
                fprintf(stdout, obj != NULL ? ",\n" : "\n");
                if (obj == NULL) break;
            }
            fprintf(stdout, "}\n");
        } break;

        case JSON_ARRAY: {
            THROW_NOT_IMPLEMENTED("Array not implemented");
        } break;

        default: {
            THROW_NOT_IMPLEMENTED("TODO: add a proper error handling for malformed JSON");
        }
    }
}

Json json = {0};
int main(void) {
    char *j = "{\"hello\":\"world\", \"another\": \"key value\"}";

    Json_Tokenizer *tokenizer = malloc(sizeof(Json_Tokenizer));
    tokenizer->token = malloc(sizeof(Json_Token));
    tokenizer->json_str = j;
    tokenizer->cursor = 0;

    parse_json(&json, tokenizer);
    print_json(&json, 2);
    return 0;
}