#include "jj.c"
#include <time.h>

Log_Config config = {
    .debug = false
};

typedef bool(*Test)(void);

#define TEST_FAILED  "FAIL"
#define TEST_SUCCESS "OK"

struct Test_Case {
    char key[256];
    char name[256];
    char input[16][256];
    Json_Value expected[16];
    Json_Type type;
};

struct Test_Case parse_tests_cases[] = {
    {
        .key = "hello",
        .name = "test_parse_json_str",
        .input = {
            "{\"hello\": \"world\"}",
            "{\"hello\": \"\"}",
        },
        .expected = {
            { .string = { .content = "world", .len = 5 }},
            { .string = { .content = "",      .len = 0 }}
        },
        .type = JSON_STRING
    },
    {
        .key = "number",
        .name = "test_parse_json_numbers",
        .input = {
            "{\"number\": 1}",
            "{\"number\": 1.25}",
            "{\"number\": 1.25e-2}",
            "{\"number\": 1.25e+2}",
            "{\"number\": 0.25}",
        },
        .expected = {
            { .number = 1 },
            { .number = 1.25 },
            { .number = 1.25e-2 },
            { .number = 1.25e+2 },
            { .number = 0.25 },
        },
        .type = JSON_NUMBER
    },
    {
        .key = "null",
        .name = "test_parse_json_nulls",
        .input = {
            "{\"null\": null}",
        },
        .expected = {
            { .null = NULL },
        },
        .type = JSON_NULL
    },
    {
        .key = "bool",
        .name = "test_parse_json_bools",
        .input = {
            "{\"bool\": true}",
            "{\"bool\": false}",
        },
        .expected = {
            { .boolean = true },
            { .boolean = false },
        },
        .type = JSON_BOOLEAN
    },
};

struct Test_Case_Errors {
    char name[256];
    char input[16][256];
    JSON_ERROR expected;
};

struct Test_Case_Errors errors_tests_cases[] = {
    {
        .name = "test_malformed_json",
        .input = {
            "{1 :\"world\"}",
            "{false :\"world\"}",
            "{null :\"world\"}",
            "{true :\"world\"}",
            "{\"hello\": \"world\" 1}",
            "[\"hello\": \"world\"]",
        },
        .expected = ERR_MALFORMED_JSON
    },
    {
        .name = "test_unexpected_end_of_json",
        .input = {
            "[1, 2, 3, 4, 5",
            "{",
            "[false",
            "{\"hello\": \"world\"",
            "{\"hello\": \"world\", \"number\": 1",
        },
        .expected = ERR_UNEXPECTED_END_OF_INPUT
    },
    {
        .name = "test_unclosed_string",
        .input = {
            "[\"test]",
            "{\"hello\": \"world}",
            "{\"hello\": \"world",
        },
        .expected = ERR_UNCLOSED_STRING
    },
    {
        .name = "test_unexpected_char",
        .input = {
            "{hello: \"world\"}",
            "{\"test\": world}",
            "{\"test\": flase}",
            "{\"test\": nil}",
            "{\"test\": 10e.10}",
            "{\"test\": 10e+10.10}",
            "|||",
        },
        .expected = ERR_UNEXPECTED_CHAR
    },
    {
        .name = "test_expected_object_or_array",
        .input = {
            "1",
            "1, 2, 3"
        },
        .expected = ERR_EXPECTED_OBJECT_OR_ARRAY
    },
};

bool test_json_doest_have_key(void);
bool test_parse_json_array(void);
bool test_json_multiple_keys(void);

Test other_tests[] = {
    test_json_doest_have_key,
    test_parse_json_array,
    test_json_multiple_keys,
};

int total_tests = 0;
int passed_tests = 0;
int main(void) {
    bool do_parsing_test(struct Test_Case *c);
    bool do_errors_test(struct Test_Case_Errors *er);
    log_init(&config);
    clock_t start = clock();

    for (size_t i = 0; i < ARRAY_SIZE(parse_tests_cases); i++) {
        total_tests++;
        if (do_parsing_test(&parse_tests_cases[i])) passed_tests++;
    }

    for (size_t i = 0; i < ARRAY_SIZE(errors_tests_cases); i++) {
        total_tests++;
        if (do_errors_test(&errors_tests_cases[i])) passed_tests++;
    }

    for (size_t i = 0; i < ARRAY_SIZE(other_tests); i++) {
        total_tests++;
        if (other_tests[i]()) passed_tests++;
    }

    log_test("tests run in %lf seconds", (double)(clock() - start) / CLOCKS_PER_SEC);
    log_test("total passed tests: %d", passed_tests);
    if (total_tests == passed_tests) {
        log_test("all tests passed");
    } else {
        log_warning("total failed tests: %d", total_tests - passed_tests);
    }
    return 0;
}

bool do_parsing_test(struct Test_Case *c) {
    Json *json;
    JSON_ERROR error;
    for (size_t i = 0; i < ARRAY_SIZE(c->input); i++) {
        if (*c->input[i] == '\0') break;

        char *json_str = c->input[i];
        json = parse_json(json_str, &error);
        if (error != JSON_NO_ERROR) {
            log_test("%s => %s %s %s", c->name, TEST_FAILED, json_error_desc(error), json_str);
            goto FAIL;
        }

        if (json == NULL) {
            log_test("%s => %s %s %s", c->name, TEST_FAILED, "cannot parse json string", json_str);
            goto FAIL;
        }

        if (json->type != JSON_OBJECT) {
            log_test("%s => %s wrong json type. Expected %s, find %s",
                    c->name,
                    TEST_FAILED,
                    json_type_desc(JSON_OBJECT),
                    json_type_desc(json->type)
                );
            goto FAIL;
        }

        Json_Object *obj = json->as.object;
        if (obj == NULL) {
            log_test("%s => %s %s", c->name, TEST_FAILED, "object is null");
            goto FAIL;
        }

        Key_Value *KV = json_get(obj, c->key);
        if (KV == NULL) {
            log_test("%s => %s key %s not finded in json", c->name, c->key, TEST_FAILED);
            goto FAIL;
        }

        if (KV->type != c->type) {
            log_test("%s => %s wrong value type. Expected %s, find %s",
                    c->name,
                    TEST_FAILED,
                    json_type_desc(c->type),
                    json_type_desc(KV->type)
                );
            goto FAIL;
        }

        switch (c->type) {
            case JSON_STRING: {
                Json_String value = c->expected[i].string;
                if (value.len != KV->value_as.string.len) {
                    log_test("%s => %s strings have differents sizes. Expected %d, find %d",
                            c->name,
                            TEST_FAILED,
                            value.len,
                            KV->value_as.string.len
                        );
                    goto FAIL;
                }

                if (strncmp(c->expected[i].string.content, KV->value_as.string.content, c->expected[i].string.len) != 0) {
                    log_test("%s => %s strings are different. Expected %s, find %s",
                            c->name,
                            TEST_FAILED,
                            c->expected[i].string.content,
                            KV->value_as.string.content
                        );
                    goto FAIL;
                }
            } break;

            case JSON_NUMBER: {
                double value = c->expected[i].number;
                if (-1*(value - KV->value_as.number) > 0.00001) {
                    log_test("%s => %s numbers are different. Expected %.6lf, find %.6lf",
                            c->name,
                            TEST_FAILED,
                            value,
                            KV->value_as.number
                        );
                    goto FAIL;
                }
            } break;

            case JSON_NULL: {
                if (KV->value_as.null != NULL) {
                    log_test("%s => %s nulls are different. Expected %p, find %p",
                            c->name,
                            TEST_FAILED,
                            NULL,
                            KV->value_as.null
                        );
                    goto FAIL;
                }
            } break;

            case JSON_BOOLEAN: {
                bool value = c->expected[i].boolean;
                if (value != KV->value_as.boolean) {
                    log_test("%s => %s booleans are different. Expected %d, find %d",
                            c->name,
                            TEST_FAILED,
                            value,
                            KV->value_as.boolean
                        );
                    goto FAIL;
                }
            } break;

            default: { panic("Unreachable code"); } break;
        }

        free(json);
    }

    log_test("%s => %s", c->name, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}

bool do_errors_test(struct Test_Case_Errors *er) {
    Json *json;
    JSON_ERROR error;
    for (size_t i = 0; i < ARRAY_SIZE(er->input); i++) {
        if (*er->input[i] == '\0') break;

        char *json_str = er->input[i];
        json = parse_json(json_str, &error);
        if (error != er->expected) {
            log_test("%s => %s `%s` != `%s` | %s", er->name, TEST_FAILED, json_error_desc(error), json_error_desc(er->expected), er->input[i]);
            goto FAIL;
        }

        if (json) free(json);
    }

    log_test("%s => %s", er->name, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}

bool test_json_doest_have_key(void) {
    JSON_ERROR error;
    char *j = "{\"hello\": \"world\"}";

    Json *json = parse_json(j, &error);
    if (error != JSON_NO_ERROR) {
        log_test("%s => %s %s %s", __func__, TEST_FAILED, json_error_desc(error), j);
        goto FAIL;
    }

    if (json == NULL) {
        log_test("%s => %s %s", __func__, TEST_FAILED, "cannot parse json string");
        goto FAIL;
    }

    Json_Object *obj = json->as.object;

    Key_Value *kv = json_get(obj, "world");
    if (kv) {
        log_test("%s => %s find value that doesnt exists in table. Expected NULL, find %d",
                __func__,
                TEST_FAILED,
                kv->type
            );
        goto FAIL;
    }

    log_test("%s => %s", __func__, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}

bool test_parse_json_array(void) {
    JSON_ERROR error;

    char *j = "{\"array\": [1, 2, 3, 4, 5]}";
    Json *json = parse_json(j, &error);
    if (error != JSON_NO_ERROR) {
        log_test("%s => %s %s %s", __func__, TEST_FAILED, json_error_desc(error), j);
        goto FAIL;
    }

    if (json == NULL) {
        log_test("%s => %s %s %s", __func__, TEST_FAILED, "cannot parse json array", j);
        goto FAIL;
    }

    if (json->type != JSON_OBJECT) {
        log_test("%s => %s wrong json type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_OBJECT),
                json_type_desc(json->type)
            );
        goto FAIL;
    }

    Json_Object *obj = json->as.object;
    if (obj == NULL) {
        log_test("%s => %s %s", __func__, TEST_FAILED, "object is null");
        goto FAIL;
    }

    Key_Value *kv = json_get(obj, "array");
    if (kv == NULL) {
        log_test("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

    if (kv->type != JSON_ARRAY) {
        log_test("%s => %s wrong value type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_ARRAY),
                json_type_desc(kv->type)
            );
        goto FAIL;
    }

    Json_Array *array = kv->value_as.array;

    if (array->len != 5) {
        log_test("%s => %s wrong array size. Expected %d, find %d",
                __func__,
                TEST_FAILED,
                5,
                array->len
            );
        goto FAIL;
    }

    int k = 1;
    for (size_t i = 0; i < array->len; i++, k++) {
        Item item = array->items[i];
        if (item.type != JSON_NUMBER) {
            log_test("%s => %s wrong value type. Expected %s, find %s",
                    __func__,
                    TEST_FAILED,
                    json_type_desc(JSON_NUMBER),
                    json_type_desc(array->items[i].type)
                );
            goto FAIL;
        }

        if ((int) item.item_as.number != k) {
            log_test("%s => %s numbers are different. Expected %d, find %d",
                    __func__,
                    TEST_FAILED,
                    k,
                    item.item_as.number
                );
            goto FAIL;
        }
    }

    free(json);
    log_test("%s => %s", __func__, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}

bool test_json_multiple_keys(void) {
    char *j = "{\"hello\": \"world\", \"number\": 1, \"bool\": true, \"null\": null}";
    JSON_ERROR error;

    Json *json = parse_json(j, &error);
    if (error != JSON_NO_ERROR) {
        log_test("%s => %s %s %s", __func__, TEST_FAILED, json_error_desc(error), j);
        goto FAIL;
    }

    if (json == NULL) {
        log_test("%s => %s %s %s", __func__, TEST_FAILED, "cannot parse json string", j);
        goto FAIL;
    }

    if (json->type != JSON_OBJECT) {
        log_test("%s => %s wrong json type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_OBJECT),
                json_type_desc(json->type)
            );
        goto FAIL;
    }

    Json_Object *obj = json->as.object;
    if (obj == NULL) {
        log_test("%s => %s %s", __func__, TEST_FAILED, "object is null");
        goto FAIL;
    }

    Key_Value *KV_STR = json_get(obj, "hello");
    if (KV_STR == NULL) {
        log_test("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

    if (KV_STR->type != JSON_STRING) {
        log_test("%s => %s wrong value type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_STRING),
                json_type_desc(KV_STR->type)
            );
        goto FAIL;
    }

    if (strncmp("world", KV_STR->value_as.string.content, KV_STR->value_as.string.len) != 0) {
        log_test("%s => %s strings are different. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                "world",
                KV_STR->value_as.string.content
            );
        goto FAIL;
    }

    Key_Value *KV_NUMBER = json_get(obj, "number");
    if (KV_NUMBER == NULL) {
        log_test("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

        if (KV_NUMBER->type != JSON_NUMBER) {
        log_test("%s => %s wrong value type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_NUMBER),
                json_type_desc(KV_NUMBER->type)
            );
        goto FAIL;
    }

    if (KV_NUMBER->value_as.number != 1) {
        log_test("%s => %s numbers are different. Expected %d, find %d",
                __func__,
                TEST_FAILED,
                1,
                KV_NUMBER->value_as.number
            );
        goto FAIL;
    }

    Key_Value *KV_BOOL = json_get(obj, "bool");
    if (KV_BOOL == NULL) {
        log_test("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

    if (KV_BOOL->type != JSON_BOOLEAN) {
        log_test("%s => %s wrong value type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_BOOLEAN),
                json_type_desc(KV_BOOL->type)
            );
        goto FAIL;
    }

    if (KV_BOOL->value_as.boolean != true) {
        log_test("%s => %s booleans are different. Expected %d, find %d",
                __func__,
                TEST_FAILED,
                true,
                KV_BOOL->value_as.boolean
            );
        goto FAIL;
    }

    Key_Value *KV_NULL = json_get(obj, "null");
    if (KV_NULL == NULL) {
        log_test("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

    if (KV_NULL->type != JSON_NULL) {
        log_test("%s => %s wrong value type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_NULL),
                json_type_desc(KV_NULL->type)
            );
        goto FAIL;
    }

    if (KV_NULL->value_as.null != NULL) {
        log_test("%s => %s nulls are different. Expected %p, find %p",
                __func__,
                TEST_FAILED,
                NULL,
                KV_NULL->value_as.null
            );
        goto FAIL;
    }

    free(json);
    log_test("%s => %s", __func__, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}

// MIT License

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