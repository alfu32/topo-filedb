#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "filedb.h"

#define DEFAULT_TEST_DATABASE_NAME "testdb"

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

void test_record_is_deleted() {
    record_t record1 = {.id = "test1", .start = 0, .end = 0};
    record_t record2 = {.id = "test2", .start = 10, .end = 20};

    assert(record__instance__is_deleted(&record1) == 1); // Should be true
    assert(record__instance__is_deleted(&record2) == 0); // Should be false

    printf("test_record_is_deleted passed!\n");
}

void test_database_open_and_close(const char* dbname) {
    database_t *db = database__static_open(dbname);
    assert(db != NULL);
    assert(db->data_file_reference >= 0);
    assert(db->index_file_reference >= 0);
    assert(database__static__close(db) == 0);
}

void test_insert_record(const char* dbname) {
    database_t *db = database__static_open(dbname);
    char data[] = "Hello, database test_insert_record!";
    record_t *record = database__instance__insert_record(db, data, strlen(data));
    assert(record != NULL);
    // assert(strlen(record->id) <= 32);
    assert(database__static__close(db) == 0);
}

void test_delete_record(const char* dbname) {
    database_t *db = database__static_open(dbname);
    char data[] = "Test delete";
    record_t *record = database__instance__insert_record(db, data, strlen(data));
    assert(record != NULL);
    record_t *deleted = database__instance__delete_record(db, record);
    assert(deleted != NULL);
    assert(deleted->start == deleted->end);
    assert(database__static__close(db) == 0);
}


int cbk_print_record(record_t *record, int ord) {
    printf("Record %d: %.32s  %1x  %32x %32x\n", ord, record->id,record__instance__is_deleted(record),record->start,record->end);fflush(stdout);
    return 0;
}
void test_list_all(const char* dbname) {
    database_t *db = database__static_open(dbname);
    char data1[] = "Record 1 test_list_all";
    char data2[] = "Record 2 test_list_all";
    database__instance__insert_record(db, data1, strlen(data1));
    database__instance__insert_record(db, data2, strlen(data2));

    assert(database__instance__list_all(db, cbk_print_record) == 0);
    assert(database__static__close(db) == 0);
}


void test_get_latest_records(const char* dbname){
    database_t *db = database__static_open(dbname);

    if (db) {
        printf("Latest, non-deleted records:\n");
        printf(" - print all records\n");
        database__instance__list_all(db, cbk_print_record);
        printf(" - print latest records\n");
        database__instance__get_latest_records(db, cbk_print_record);
        database__static__close(db);
    }
}

void test_optimize(const char* dbname) {
    database_t *db = database__static_open(dbname);
    char data1[] = "Record 1 test_optimize";
    char data2[] = "Record 2 test_optimize";
    database__instance__insert_record(db, data1, strlen(data1));
    database__instance__insert_record(db, data2, strlen(data2));
    database__instance__delete_record(db, &db->record_list[0]);
    printf(" - print unoptimized\n");
    assert(database__instance__list_all(db, cbk_print_record) == 0);
    assert(database__instance__optimize(db) == 0);
    printf(" - print optimized\n");
    assert(database__instance__list_all(db, cbk_print_record) == 0);
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
void test_list_all_with_content(const char* dbname) {
    // Create a database and insert sample records
    database_t *db = database__static_open(dbname);
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


__declspec(dllexport) int main(int argc,const char** argv) {
    
    if(argc==0){
        printf("use a database test name\n");
        return -1;
    }
    printf("using arg[0] = %s for the test database name\n",argv[0]);
    const char* dbname = argv[0];
    
    printf("=== test_record_creation  ...........====================================================\n");
    test_record_creation();
    printf("=== test_record_is_deleted  .........====================================================\n");
    test_record_is_deleted();
    printf("=== test_database_open_and_close  ...====================================================\n");
    test_database_open_and_close(dbname);
    printf("=== test_insert_record  .............====================================================\n");
    test_insert_record(dbname);
    printf("=== test_delete_record  .............====================================================\n");
    test_delete_record(dbname);
    printf("=== test_list_all  ..................====================================================\n");
    test_list_all(dbname);
    printf("=== test_list_all_with_content  .....====================================================\n");
    test_list_all_with_content(dbname);
    printf("=== test_get_latest_records  ........====================================================\n");
    test_get_latest_records(dbname);
    printf("=== test_optimize  ..................====================================================\n");
    test_optimize(dbname);

    printf("All tests passed!\n");
    return 0;
}
