#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "filedb.h"

void test_record_creation() {
    char data[] = "Test content creation";
    printf(" - creating record from buffer\n");
    record_t *record = record__static__new_from_buffer(0, data, strlen(data));
    printf(" * record_t(%s %zu %zu)\n",record->id,record->start,record->end);
    printf(" - test not null\n");
    assert(record != NULL);
    printf(" - test not null\n");
    // assert(strlen(record->id) <= 32);
    assert(record->start == 0);
    assert(record->end == strlen(data));
    free(record);
}

void test_database_open_and_close() {
    database_t *db = database__static_open("test_db");
    assert(db != NULL);
    assert(db->data_file_reference >= 0);
    assert(db->index_file_reference >= 0);
    assert(database__static__close(db) == 0);
}

void test_insert_record() {
    database_t *db = database__static_open("test_db");
    char data[] = "Hello, database test_insert_record!";
    record_t *record = database__instance__insert_record(db, data, strlen(data));
    assert(record != NULL);
    // assert(strlen(record->id) <= 32);
    assert(database__static__close(db) == 0);
}

void test_delete_record() {
    database_t *db = database__static_open("test_db");
    char data[] = "Test delete";
    record_t *record = database__instance__insert_record(db, data, strlen(data));
    assert(record != NULL);
    record_t *deleted = database__instance__delete_record(db, record);
    assert(deleted != NULL);
    assert(deleted->start == deleted->end);
    assert(database__static__close(db) == 0);
}


int callback__test_list_all(record_t *record, int ord) {
    printf("Record %d: %.32s\n", ord, record->id);fflush(stdout);
    return 0;
}
void test_list_all() {
    database_t *db = database__static_open("test_db");
    char data1[] = "Record 1 test_list_all";
    char data2[] = "Record 2 test_list_all";
    database__instance__insert_record(db, data1, strlen(data1));
    database__instance__insert_record(db, data2, strlen(data2));

    assert(database__instance__list_all(db, callback__test_list_all) == 0);
    assert(database__static__close(db) == 0);
}

void test_optimize() {
    database_t *db = database__static_open("test_db");
    char data1[] = "Record 1 test_optimize";
    char data2[] = "Record 2 test_optimize";
    database__instance__insert_record(db, data1, strlen(data1));
    database__instance__insert_record(db, data2, strlen(data2));
    database__instance__delete_record(db, &db->record_list[0]);
    assert(database__instance__list_all(db, callback__test_list_all) == 0);
    assert(database__instance__optimize(db) == 0);
    assert(database__instance__list_all(db, callback__test_list_all) == 0);
    assert(database__static__close(db) == 0);
}


// Callback to validate record content
error_t test_list_all_with_content__validate_and_print(record_t *record, int ord, char *content) {
    printf("Record %d\t", ord);
    printf("ID: %.32s\t", record->id);
    printf("Content: %s\n", content);fflush(stdout);

    // Verify content for each record
    // switch (ord) {
    //     case 0:
    //         assert(strcmp(content, data1) == 0);
    //         break;
    //     case 1:
    //         assert(strcmp(content, data2) == 0);
    //         break;
    //     case 2:
    //         assert(strcmp(content, data3) == 0);
    //         break;
    //     default:
    //         assert(0 && "Unexpected record ordinal");
    // }
    return 0; // No error
}
void test_list_all_with_content() {
    // Create a database and insert sample records
    database_t *db = database__static_open("test_db");
    assert(db != NULL);

    char data1[] = "Hello, World!";
    char data2[] = "This is a test record.";
    char data3[] = "Another sample record.";

    printf("inserting record 1\n");fflush(stdout);
    record_t *record1 = database__instance__insert_record(db, data1, strlen(data1));
    printf("inserting record 2\n");fflush(stdout);
    record_t *record2 = database__instance__insert_record(db, data2, strlen(data2));
    printf("inserting record 3\n");fflush(stdout);
    record_t *record3 = database__instance__insert_record(db, data3, strlen(data3));

    assert(record1 != NULL);
    assert(record2 != NULL);
    assert(record3 != NULL);

    // Call the function and pass the validation callback
    printf("listing\n");fflush(stdout);
    error_t result = database__instance__list_all_with_content(db, test_list_all_with_content__validate_and_print);
    assert(result == 0);

    // Cleanup
    assert(database__static__close(db) == 0);
}

int main() {
    printf("=== test_record_creation  ...........====================================================\n");
    test_record_creation();
    printf("=== test_database_open_and_close  ...====================================================\n");
    test_database_open_and_close();
    printf("=== test_insert_record  .............====================================================\n");
    test_insert_record();
    printf("=== test_delete_record  .............====================================================\n");
    test_delete_record();
    printf("=== test_list_all  ..................====================================================\n");
    test_list_all();
    printf("=== test_list_all_with_content  .....====================================================\n");
    test_list_all_with_content();
    printf("=== test_optimize  ..................====================================================\n");
    test_optimize();

    printf("All tests passed!\n");
    return 0;
}
