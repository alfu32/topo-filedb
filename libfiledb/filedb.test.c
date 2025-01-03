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

void test_record_allocation() {
    record_t record = {.start = 0, .end = 20};
    char *buffer = record__instance__allocate_content_buffer(&record);
    assert(buffer != NULL);
    free(buffer);
    printf("test_record_allocation passed!\n");
}

void test_record_copy() {
    record_t original = {
        .id = "abc123def456ghi789jkl012mno345pq",
        .start = 50,
        .end = 100
    };

    record_t *copy = record__instance__copy(&original);
    assert(copy != NULL);
    assert(strncmp(copy->id, original.id, 32) == 0);
    assert(copy->start == original.start);
    assert(copy->end == original.end);

    printf("test_record_copy passed!\n");

    free(copy);
}

void test_record_is_deleted() {
    record_t record1 = {.id = "test1", .start = 0, .end = 0};
    record_t record2 = {.id = "test2", .start = 10, .end = 20};

    assert(record__instance__is_deleted(&record1) == 1); // Should be true
    assert(record__instance__is_deleted(&record2) == 0); // Should be false

    printf("test_record_is_deleted passed!\n");
}

void test_database_open_and_close(const char* dbname) {
    database_t *db = database__static_new(dbname);
    database__static_open(db);
    assert(db != NULL);
    assert(db->data_file_reference >= 0);
    assert(db->index_file_reference >= 0);
    assert(database__static__close(db) == 0);
    assert(database__static__free(db) == 0);
}

void test_insert_record(const char* dbname) {
    database_t *db = database__static_new(dbname);
    database__static_open(db);
    char data[] = "Hello, database test_insert_record!";
    record_t *record = database__instance__insert_record(db, data, strlen(data));
    assert(record != NULL);
    // assert(strlen(record->id) <= 32);
    assert(database__static__close(db) == 0);
    assert(database__static__free(db) == 0);
}

void test_delete_record(const char* dbname) {
    database_t *db = database__static_new(dbname);
    database__static_open(db);
    char data[] = "Test delete";
    record_t *record = database__instance__insert_record(db, data, strlen(data));
    assert(record != NULL);
    record_t *deleted = database__instance__delete_record(db, record);
    assert(deleted != NULL);
    assert(deleted->start == deleted->end);
    assert(database__static__close(db) == 0);
    assert(database__static__free(db) == 0);
}


int cbk_print_record(record_t *record, int ord) {
    if(!record) {
        printf("record %04d is Invalid\n",ord);
    } else {
        printf("Record %04d: %.32s  %d  %032zx %032zx\n", ord, record->id,record__instance__is_deleted(record),record->start,record->end);
    }
    fflush(stdout);
    return 0;
}
void test_list_all(const char* dbname) {
    database_t *db = database__static_new(dbname);
    database__static_open(db);
    char data1[] = "Record 1 test_list_all";
    char data2[] = "Record 2 test_list_all";
    database__instance__insert_record(db, data1, strlen(data1));
    database__instance__insert_record(db, data2, strlen(data2));

    assert(database__instance__list_all(db, cbk_print_record) == 0);
    assert(database__static__close(db) == 0);
    assert(database__static__free(db) == 0);
}


void test_get_latest_records(const char* dbname){
    database_t *db = database__static_new(dbname);
    database__static_open(db);

    if (db) {
        printf("Latest, non-deleted records:\n");
        printf(" - print all records\n");
        database__instance__list_all(db, cbk_print_record);
        printf(" - print latest records\n");
        database__instance__get_latest_records(db, cbk_print_record);
        assert(database__static__close(db) == 0);
        assert(database__static__free(db) == 0);
    }
}

error_t test_counter_fn_1(int* context, record_t* record, int ord) {
        printf("Record %d: ID = %.32s\n", ord, record->id);
        return 0;
    }
error_t test_counter_fn_2(int* context, record_t* record, int ord) {
        printf("Latest Record %d: ID = %.32s\n", ord, record->id);
        return 0;
    }

// void test_aggregation(const char* dbname) {
//     database_t *db = database__static_open(dbname);
// 
//     // Insert some records
//     database__instance__insert_record(db, "Record 1", 8);
//     database__instance__insert_record(db, "Record 2", 8);
//     database__instance__insert_record(db, "Record 3", 8);
// 
//     int count_all=0;
//     int count_latest=0;
// 
//     // Aggregate all records
//     database__instance__aggregate_all(db, test_counter_fn_1, (void*)&count_all);
// 
//     // Aggregate latest records
//     database__instance__aggregate_latest_records(db, test_counter_fn_2, (void*)&count_latest);
// 
//     database__static__close(db);
// }

void test_optimize(const char* dbname) {
    database_t *db = database__static_new(dbname);
    database__static_open(db);
    char data1[] = "Record 1 test_optimize";
    char data2[] = "Record 2 test_optimize";
    database__instance__insert_record(db, data1, strlen(data1));
    database__instance__insert_record(db, data2, strlen(data2));
    database__instance__delete_record(db, &db->record_list[0]);
    printf(" - print unoptimized\n");fflush(stdout);
    assert(database__instance__list_all(db, cbk_print_record) == 0);
    printf(" - optimizing ... \n");fflush(stdout);
    assert(database__instance__optimize(db) == 0);
    printf(" - print optimized\n");fflush(stdout);
    assert(database__instance__list_all(db, cbk_print_record) == 0);
    assert(database__static__close(db) == 0);
    assert(database__static__free(db) == 0);
}


// Callback to validate record content
error_t test_list_all_with_content__validate_and_print(record_t *record, int ord, char *content) {
    printf("Record %04d\t", ord);
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
    database_t *db = database__static_new(dbname);
    database__static_open(db);
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
    assert(database__static__free(db) == 0);
}


int main(int argc,const char** argv) {
    
    if(argc<2){
        printf("use a database test name\n");
        return -1;
    }
    printf("using arg[0] = %s for the test database name\n",argv[1]);
    const char* dbname = argv[1];
    
    printf("=== test_record_creation  ...........====================================================\n");
    test_record_creation();
    printf("=== test_record_copy  ...............====================================================\n");
    test_record_copy();
    printf("=== test_record_allocation  .........====================================================\n");
    test_record_allocation();
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
