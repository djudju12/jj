#include "jj.c"
#include <time.h>

Log_Config config = {
    .debug = false
};

typedef bool(*Test)(void);

#define TEST_FAILED  "FAIL."
#define TEST_SUCCESS "OK."

struct Test_Case {
    Test fn;
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
    log_init(&config);
    clock_t start = clock();

    for (size_t i = 0; i < ARRAY_SIZE(parse_tests_cases); i++) {
        total_tests++;
        if (do_parsing_test(&parse_tests_cases[i])) passed_tests++;
    }

    for (size_t i = 0; i < ARRAY_SIZE(other_tests); i++) {
        total_tests++;
        if (other_tests[i]()) passed_tests++;
    }

    log_info("tests run in %lf seconds", (double)(clock() - start) / CLOCKS_PER_SEC);
    log_info("total passed tests: %d", passed_tests);
    if (total_tests == passed_tests) {
        log_info("all tests passed.");
    } else {
        log_warning("total failed tests: %d", total_tests - passed_tests);
    }
    return 0;
}

bool do_parsing_test(struct Test_Case *c) {
    Json *json;
    for (size_t i = 0; i < ARRAY_SIZE(c->input); i++) {
        if (*c->input[i] == '\0') break;

        char *json_str = c->input[i];
        json = parse_json(json_str);
        if (json == NULL) {
            log_info("%s => %s %s %s", c->name, TEST_FAILED, "cannot parse json string", json_str);
            goto FAIL;
        }

        if (json->type != JSON_OBJECT) {
            log_info("%s => %s wrong json type. Expected %s, find %s",
                    c->name,
                    TEST_FAILED,
                    json_type_desc(JSON_OBJECT),
                    json_type_desc(json->type)
                );
            goto FAIL;
        }

        Json_Object *obj = json->as.object;
        if (obj == NULL) {
            log_info("%s => %s %s", c->name, TEST_FAILED, "object is null");
            goto FAIL;
        }

        int j = json_geti(obj, c->key);
        if (j < 0) {
            log_info("%s => %s key %s not finded in json", c->name, c->key, TEST_FAILED);
            goto FAIL;
        }

        Key_Value KV = obj->entries[j];
        if (KV.type != c->type) {
            log_info("%s => %s wrong value type. Expected %s, find %s",
                    c->name,
                    TEST_FAILED,
                    json_type_desc(c->type),
                    json_type_desc(obj->entries[j].type)
                );
            goto FAIL;
        }

        switch (c->type) {
            case JSON_STRING: {
                Json_String value = c->expected[i].string;
                if (value.len != KV.value_as.string.len) {
                    log_info("%s => %s strings have differents sizes. Expected %d, find %d",
                            c->name,
                            TEST_FAILED,
                            value.len,
                            KV.value_as.string.len
                        );
                    goto FAIL;
                }

                if (strncmp(c->expected[i].string.content, KV.value_as.string.content, c->expected[i].string.len) != 0) {
                    log_info("%s => %s strings are different. Expected %s, find %s",
                            c->name,
                            TEST_FAILED,
                            c->expected[i].string.content,
                            KV.value_as.string.content
                        );
                    goto FAIL;
                }
            } break;

            case JSON_NUMBER: {
                double value = c->expected[i].number;
                if (-1*(value - KV.value_as.number) > 0.00001) {
                    log_info("%s => %s numbers are different. Expected %.6lf, find %.6lf",
                            c->name,
                            TEST_FAILED,
                            value,
                            KV.value_as.number
                        );
                    goto FAIL;
                }
            } break;

            case JSON_NULL: {
                if (KV.value_as.null != NULL) {
                    log_info("%s => %s nulls are different. Expected %p, find %p",
                            c->name,
                            TEST_FAILED,
                            NULL,
                            KV.value_as.null
                        );
                    goto FAIL;
                }
            } break;

            case JSON_BOOLEAN: {
                bool value = c->expected[i].boolean;
                if (value != KV.value_as.boolean) {
                    log_info("%s => %s booleans are different. Expected %d, find %d",
                            c->name,
                            TEST_FAILED,
                            value,
                            KV.value_as.boolean
                        );
                    goto FAIL;
                }
            } break;

            default: { panic("Unreachable code"); } break;
        }

        free(json);
    }

    log_info("%s => %s", c->name, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}

bool test_json_doest_have_key(void) {
    char *j = "{\"hello\": \"world\"}";

    Json *json = parse_json(j);
    if (json == NULL) {
        log_info("%s => %s %s", __func__, TEST_FAILED, "cannot parse json string");
        goto FAIL;
    }

    Json_Object *obj = json->as.object;

    int i = json_geti(obj, "world");
    if (i != -1) {
        log_info("%s => %s find value that doesnt exists in table. Expected -1, find %d",
                __func__,
                TEST_FAILED,
                i
            );
        goto FAIL;
    }

    log_info("%s => %s", __func__, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}

bool test_parse_json_array(void) {
    char *j = "{\"array\": [1, 2, 3, 4, 5]}";
    Json *json = parse_json(j);

    if (json == NULL) {
        log_info("%s => %s %s %s", __func__, TEST_FAILED, "cannot parse json array", j);
        goto FAIL;
    }

    if (json->type != JSON_OBJECT) {
        log_info("%s => %s wrong json type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_OBJECT),
                json_type_desc(json->type)
            );
        goto FAIL;
    }

    Json_Object *obj = json->as.object;
    if (obj == NULL) {
        log_info("%s => %s %s", __func__, TEST_FAILED, "object is null");
        goto FAIL;
    }

    int i = json_geti(obj, "array");
    if (i < 0) {
        log_info("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

    Json_Array *array = obj->entries[i].value_as.array;

    if (array->len != 5) {
        log_info("%s => %s wrong array size. Expected %d, find %d",
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
            log_info("%s => %s wrong value type. Expected %s, find %s",
                    __func__,
                    TEST_FAILED,
                    json_type_desc(JSON_NUMBER),
                    json_type_desc(array->items[i].type)
                );
            goto FAIL;
        }

        if ((int) item.item_as.number != k) {
            log_info("%s => %s numbers are different. Expected %d, find %d",
                    __func__,
                    TEST_FAILED,
                    k,
                    item.item_as.number
                );
            goto FAIL;
        }
    }

    free(json);
    log_info("%s => %s", __func__, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}

bool test_json_multiple_keys(void) {
    char *j = "{\"hello\": \"world\", \"number\": 1, \"bool\": true, \"null\": null}";
    Json *json = parse_json(j);

    if (json == NULL) {
        log_info("%s => %s %s %s", __func__, TEST_FAILED, "cannot parse json string", j);
        goto FAIL;
    }

    if (json->type != JSON_OBJECT) {
        log_info("%s => %s wrong json type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_OBJECT),
                json_type_desc(json->type)
            );
        goto FAIL;
    }

    Json_Object *obj = json->as.object;
    if (obj == NULL) {
        log_info("%s => %s %s", __func__, TEST_FAILED, "object is null");
        goto FAIL;
    }

    int i = json_geti(obj, "hello");
    if (i < 0) {
        log_info("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

    Key_Value KV = obj->entries[i];
    if (KV.type != JSON_STRING) {
        log_info("%s => %s wrong value type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_STRING),
                json_type_desc(obj->entries[i].type)
            );
        goto FAIL;
    }

    if (strncmp("world", KV.value_as.string.content, KV.value_as.string.len) != 0) {
        log_info("%s => %s strings are different. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                "world",
                KV.value_as.string.content
            );
        goto FAIL;
    }

    i = json_geti(obj, "number");
    if (i < 0) {
        log_info("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

    KV = obj->entries[i];
    if (KV.type != JSON_NUMBER) {
        log_info("%s => %s wrong value type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_NUMBER),
                json_type_desc(obj->entries[i].type)
            );
        goto FAIL;
    }

    if (KV.value_as.number != 1) {
        log_info("%s => %s numbers are different. Expected %d, find %d",
                __func__,
                TEST_FAILED,
                1,
                KV.value_as.number
            );
        goto FAIL;
    }

    i = json_geti(obj, "bool");
    if (i < 0) {
        log_info("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

    KV = obj->entries[i];
    if (KV.type != JSON_BOOLEAN) {
        log_info("%s => %s wrong value type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_BOOLEAN),
                json_type_desc(obj->entries[i].type)
            );
        goto FAIL;
    }

    if (KV.value_as.boolean != true) {
        log_info("%s => %s booleans are different. Expected %d, find %d",
                __func__,
                TEST_FAILED,
                true,
                KV.value_as.boolean
            );
        goto FAIL;
    }

    i = json_geti(obj, "null");
    if (i < 0) {
        log_info("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

    KV = obj->entries[i];
    if (KV.type != JSON_NULL) {
        log_info("%s => %s wrong value type. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                json_type_desc(JSON_NULL),
                json_type_desc(obj->entries[i].type)
            );
        goto FAIL;
    }

    if (KV.value_as.null != NULL) {
        log_info("%s => %s nulls are different. Expected %p, find %p",
                __func__,
                TEST_FAILED,
                NULL,
                KV.value_as.null
            );
        goto FAIL;
    }

    free(json);
    log_info("%s => %s", __func__, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}