#ifndef __filedb_h__
#define __filedb_h__
#include <stddef.h>

typedef struct record_s {
    /**
     * 256bits/32 bytes uuid generated hashing the contents of the record
    */
    char id[32];
    /**
     * record start fseek position inside the database.data file
     */
    size_t start;
    /**
     * record end fseek position inside the database.data file
     */
    size_t end;
} record_s;

typedef record_s record_t;

/**
 * allocates a new record calculating the uuid of the record based on some hashing algorhithm of the given content ( preferably not outsourced to an external library )
 */
record_t* record__static__new_from_buffer(int start,char* data,int data_length);
/**
 * allocates a new buffer for the record content
 */
char* record__instance__allocate_content_buffer(const record_t *self);
/**
 * Creates a copy of the given record.
 * The caller is responsible for freeing the returned copy.
 */
record_t* record__instance__copy(const record_t *self);
/**
 * checks if the record is deleted or not
 */
int record__instance__is_deleted(const record_t *record);

typedef int error_t;

typedef struct database_s {
    /**
     * the name of the database, is effectively a path
     * serves to identify the data file and the index file
     * following the following formula: 
     * index file : <database.path>.index 
     * data file : <database.path>.data
     */
    const char* path;

    /**
     * stores a reference to the openend data file
     */
    int data_file_reference;
    /**
     * stores a reference to the index file.
     * the index file is basically a binary dump of a list of record*
     */
    int index_file_reference;

    record_t* record_list;
    size_t record_list_length;

} database_s;

typedef database_s database_t;
/**
 * creates a new instance of a database
 */
database_t* database__static_new(const char* path);
/**
 * initialises a database connection.
 * if any of the index file or data file do not exist it will create them
 * if both exist it will read the binary index file into the new database record_list
 */
error_t database__static_open(database_t* self);
/**
 * closes the files of the database
 */
error_t database__static__close(database_t* self);
/**
 * frees up the memory of the database
 */
error_t database__static__free(database_t* self);
/**
 * creates a new record.
 * the binary data is stored at the end of the data 
 *   file then the record object is initialized, stored
 *   in the index file and finally added to the record_list
 */
record_t* database__instance__insert_record(database_t* self,char* data,int data_length);
/**
 * deletes the record by simply inserting a new
 *   record that copies the given record id but uses the content ''
 * effectively the deleted record will have the start and end fields equal with the current data fseek index
 * and will be stored as such in the index file 
 */
record_t* database__instance__delete_record(database_t* self,record_t* record);

/**
 * functional type used in the record iterator functions
 */
typedef error_t (*record_found_fn)(record_t*,int ord);

/**
 * reads the record content
 */
error_t database__instance__get_record_content(database_t* self, record_t* record, char* content);

/**
 * lists all the records with no sorting
 */
error_t database__instance__list_all(database_t* self,record_found_fn on_record_found);

/**
 * functional type used in the record aggregator functions
 */
typedef error_t (*record_aggregator_fn)(void* context,record_t*,int ord);
/**
 * aggregates all the records with no sorting
 */
error_t database__instance__aggregate_all(database_t* self,record_aggregator_fn on_record_found,void* context);

/**
 * Iterates over the latest, non-deleted records.
 * Calls the provided callback function for each valid record.
 */
error_t database__instance__get_latest_records(database_t *self, record_found_fn on_record_found);
/**
 * aggregates latest records with no sorting
 */
error_t database__instance__aggregate_latest_records(database_t* self,record_aggregator_fn on_record_found,void* context);

/**
 * functional type used in the record with content iterator functions
 */
typedef error_t (*record_found_with_content_fn)(record_t*,int ord,char* content);

/**
 * lists all the records with no sorting and with their 
 *     content buffer read from the database.data file.
 * the content buffer is freed after the call on on_record_with_content_found 
 */
error_t database__instance__list_all_with_content(database_t* self,record_found_with_content_fn on_record_with_content_found);

/**
 * will eliminate all but the last version of a record from the database, updating the index and the data file
 */
error_t database__instance__optimize(database_t* self);

#endif