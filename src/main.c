#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

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

#define MAX_STR_LEN 1024

typedef
struct {
    char value[MAX_STR_LEN];
    Token_Type type;
} Json_Token;

char *token_desc(Token_Type token_type) {
    return TOKEN_DESCRIPTION[token_type];
}

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
    switch (c) {
        case '"': {
            while (c != EOJ && (c = next_char(tokenizer)) != '"') {
                tokenizer->token->value[i++] = c;
            }

            tokenizer->token->value[i] = '\0';
            tokenizer->token->type = TOKEN_STRING;
        } break;

        case '{': {
            tokenizer->token->value[i++] = c;
            tokenizer->token->value[i] = '\0';
            tokenizer->token->type = TOKEN_OPBRAKT;
        } break;

        case '}': {
            tokenizer->token->value[i++] = c;
            tokenizer->token->value[i] = '\0';
            tokenizer->token->type = TOKEN_CLBRAKT;
        } break;

        case ':': {
            tokenizer->token->value[i++] = c;
            tokenizer->token->value[i] = '\0';
            tokenizer->token->type = TOKEN_COLON;
        } break;

        case ',': {
            tokenizer->token->value[i++] = c;
            tokenizer->token->value[i] = '\0';
            tokenizer->token->type = TOKEN_COMMA;
        } break;

        default: {
            fprintf(stderr, "Invalid token => %c \n", c);
            exit(1);
        }
    }

    tokenizer->token->value[i] = '\0';
    return 0;
}

//// parser
#define MAX_ENTRIES_JSON 256

typedef
enum {
    JSON_ARRAY ,
    JSON_OBJECT,
    JSON_STRING,
} Json_Type;

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
union {
    Object *object[1024];
    /* TODO: add array type */
} _JSON;

typedef
struct {
    _JSON as;
    Json_Type type;
} Json;

void parse_object(Object *object, Json_Tokenizer *tokenizer);

void parse_json(Json *root, Json_Tokenizer *tokenizer) {
    assert(next_token(tokenizer) != -1 && "expted [TOKEN_OPBRAKT, ] got End of Input");

    unsigned int cnt = 0;
    switch (tokenizer->token->type) {
        case TOKEN_OPBRAKT: {
            assert(root->type == 0);
            root->type = JSON_OBJECT;

            Object *object = malloc(sizeof(Object));

            parse_object(object, tokenizer);
            root->as.object[cnt++] = object;
            assert(next_token(tokenizer) != -1 && "expted TOKEN_COMMA got End of Input");
            while (tokenizer->token->type == TOKEN_COMMA) {
                Object *object = malloc(sizeof(Object));
                root->as.object[cnt++] = object;
                parse_object(object, tokenizer);
                assert(next_token(tokenizer) != -1 && "expted TOKEN_COMMA got End of Input");
            }

            if (tokenizer->token->type != TOKEN_CLBRAKT) {
                fprintf(stderr, "Expected end of input got"FMT_TOKEN, ARG_TOKEN(tokenizer->token));
                exit(1);
            }

        } break;

        default: {
            fprintf(stderr, "Parse for "FMT_TOKEN" Not Implemented", ARG_TOKEN(tokenizer->token));
            exit(1);
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

    assert(next_token(tokenizer) != -1);
    assert(tokenizer->token->type == TOKEN_COLON);
    assert(next_token(tokenizer) != -1);

    switch(tokenizer->token->type) {
        case TOKEN_STRING: {
            object->as.string = malloc(sizeof(Json_String));
            parse_string(object->as.string, tokenizer->token->value);
            object->type = JSON_STRING;
        } break;

        default: {
            fprintf(stderr, "Parse for "FMT_TOKEN" Not Implemented", ARG_TOKEN(tokenizer->token));
            exit(1);
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
            fprintf(stderr, "%d\n", obj->type);
            assert(0 && "TODO: add proper error handling for malformat json");
        } break;
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
            assert(0 && "TODO: not implemented");
        } break;

        default: {
            assert(0 && "TODO: add a proper error handling for malformed JSON");
        }
    }
}

Json json = {0};
int main(void) {
    char *j = "{\"obj\": {\"hello\":\"world\", \"another\": \"key value\"}}";

    Json_Tokenizer *tokenizer = malloc(sizeof(Json_Tokenizer));
    tokenizer->token = malloc(sizeof(Json_Token));
    tokenizer->json_str = j;
    tokenizer->cursor = 0;

    parse_json(&json, tokenizer);
    print_json(&json, 2);
    return 0;
}