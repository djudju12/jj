#include "jj.c"
#include <time.h>

Log_Config config = {
    .debug = false
};

typedef bool(*test_case)(void);

bool test_parse_json_str(void);
bool test_parse_json_numbers(void);
bool test_parse_json_nulls(void);
bool test_parse_json_bools(void);
bool test_json_doest_have_key(void);

bool test_parse_json_array(void);
// bool test_parse_json_object(void);
// bool test_parse_json_nested_array(void);
// bool test_parse_json_nested_object(void);


test_case cases[] = {
    test_parse_json_str,
    test_parse_json_numbers,
    test_parse_json_nulls,
    test_parse_json_bools,
    test_parse_json_array,
    test_json_doest_have_key
};

int main(void) {
    log_init(&config);
    log_info("total tests: %d", ARRAY_SIZE(cases));
    clock_t start = clock();

    int tests_passed = 0;
    for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
        if (cases[i]()) tests_passed++;
    }

    log_info("tests run in %lf seconds", (double)(clock() - start) / CLOCKS_PER_SEC);
    log_info("total passed tests: %d", tests_passed);
    log_info("total failed tests: %d", ARRAY_SIZE(cases) - tests_passed);

    return 0;
}

#define TEST_FAILED  "FAIL."
#define TEST_SUCCESS "OK."

// TODO: this tests cases per value type can be 1 function
bool test_parse_json_str(void) {
    char *j = "{\"hello\": \"world\"}";

    Json *json = parse_json(j);
    if (json == NULL) {
        log_info("%s => %s %s", __func__, TEST_FAILED, "cannot parse json string");
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

    if (strlen("world") != KV.value_as.string.len) {
        log_info("%s => %s strings have differents sizes. Expected %d, find %d",
                __func__,
                TEST_FAILED,
                strlen("world"),
                KV.value_as.string.len
            );
        goto FAIL;
    }

    if (strncmp("world", KV.value_as.string.content, 5) != 0) {
        log_info("%s => %s strings are different. Expected %s, find %s",
                __func__,
                TEST_FAILED,
                "world",
                KV.value_as.string.content
            );
        goto FAIL;
    }

    log_info("%s => %s", __func__, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}

bool test_parse_json_numbers(void) {
    double values[] = {
        1,
        1.25,
        1.25e-2,
        1.25e+2,
        0.25,
    };

    char *jsons[] = {
        "{\"number\": 1}",
        "{\"number\": 1.25}",
        "{\"number\": 1.25e-2}",
        "{\"number\": 1.25e+2}",
        "{\"number\": 0.25}",
    };

    Json *json;
    for (size_t i = 0; i < ARRAY_SIZE(jsons); i++) {
        char *j = jsons[i];
        double v = values[i];

        json = parse_json(j);
        if (json == NULL) {
            log_info("%s => %s %s %s", __func__, TEST_FAILED, "cannot parse json number", j);
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

        int i = json_geti(obj, "number");
        if (i < 0) {
            log_info("%s => %s key not finded in json", __func__, TEST_FAILED);
            goto FAIL;
        }
        Key_Value KV = obj->entries[i];
        if (KV.type != JSON_NUMBER) {
            log_info("%s => %s wrong value type. Expected %s, find %s",
                    __func__,
                    TEST_FAILED,
                    json_type_desc(JSON_NUMBER),
                    json_type_desc(obj->entries[i].type)
                );
            goto FAIL;
        }

        if (-1*(KV.value_as.number - v) > 0.00001) {
            log_info("%s => %s numbers are different. Expected %.6lf, find %.6lf",
                    __func__,
                    TEST_FAILED,
                    v,
                    KV.value_as.number
                );
            goto FAIL;
        }

        free(json);
    }

    log_info("%s => %s", __func__, TEST_SUCCESS);
    return true;

FAIL:
    if (json) free(json);
    return false;
}

bool test_parse_json_nulls(void) {
    char *j = "{\"null\": null}";
    Json *json = parse_json(j);

    if (json == NULL) {
        log_info("%s => %s %s %s", __func__, TEST_FAILED, "cannot parse json null", j);
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

    int i = json_geti(obj, "null");
    if (i < 0) {
        log_info("%s => %s key not finded in json", __func__, TEST_FAILED);
        goto FAIL;
    }

    Key_Value KV = obj->entries[i];
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
        log_info("%s => %s JSON null is not NULL",
                __func__,
                TEST_FAILED
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

bool test_parse_json_bools(void) {
    bool values[] = {
        true, false
    };

    char *jsons[] = {
        "{\"bool\": true}",
        "{\"bool\": false}",
    };

    Json *json;
    for (size_t i = 0; i < ARRAY_SIZE(jsons); i++) {
        char *j = jsons[i];
        bool v = values[i];

        json = parse_json(j);
        if (json == NULL) {
            log_info("%s => %s %s %s", __func__, TEST_FAILED, "cannot parse json bools", j);
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

        int i = json_geti(obj, "bool");
        if (i < 0) {
            log_info("%s => %s key not finded in json", __func__, TEST_FAILED);
            goto FAIL;
        }
        Key_Value KV = obj->entries[i];
        if (KV.type != JSON_BOOLEAN) {
            log_info("%s => %s wrong value type. Expected %s, find %s",
                    __func__,
                    TEST_FAILED,
                    json_type_desc(JSON_BOOLEAN),
                    json_type_desc(obj->entries[i].type)
                );
            goto FAIL;
        }

        if (KV.value_as.boolean != v) {
            log_info("%s => %s booleans are different. Expected %d, find %d",
                    __func__,
                    TEST_FAILED,
                    v,
                    KV.value_as.number
                );
            goto FAIL;
        }

        free(json);
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

