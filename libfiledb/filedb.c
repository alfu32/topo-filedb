#include "filedb.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#ifdef __MINGW32__
#include <windows.h>

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    off_t original_offset = lseek(fd, 0, SEEK_CUR); // Save current position
    if (lseek(fd, offset, SEEK_SET) == -1) {
        return -1; // Error seeking
    }
    ssize_t result = read(fd, buf, count);         // Read data
    lseek(fd, original_offset, SEEK_SET);          // Restore original position
    return result;
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    off_t original_offset = lseek(fd, 0, SEEK_CUR); // Save current position
    if (lseek(fd, offset, SEEK_SET) == -1) {
        return -1; // Error seeking
    }
    ssize_t result = write(fd, buf, count);        // Write data
    lseek(fd, original_offset, SEEK_SET);          // Restore original position
    return result;
}

// Compute hash
// Helper function to compute a simple hash for record content
static void compute_hash(char *data, int data_length, char *hash) {
    size_t sum = 0;
    for (int i = 0; i < data_length; i++) {
        sum += (unsigned char)data[i] * (i + 1);
    }
    snprintf(hash, 32, "%032zx", sum);
}

int rename_file(const char *oldname, const char *newname) {
    if (MoveFileEx(oldname, newname, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) == 0) {
        fprintf(stderr, "MoveFileEx failed with error code: %lu\n", GetLastError());
        return -1;
    }
    return 0;
}
#else

#include <openssl/sha.h>
#include <openssl/md5.h>

// Compute SHA-256 hash
void compute_hash_sha256(char *data, int data_length, char *hash) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)data, data_length, digest);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hash + (i * 2), "%02x", digest[i]);
    }
    hash[SHA256_DIGEST_LENGTH * 2] = '\0'; // Null-terminate the string
}

// Compute MD5 hash
void compute_hash_md5(const char *data, int data_length, char *hash) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((unsigned char *)data, data_length, digest);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(hash + (i * 2), "%02x", digest[i]);
    }
    hash[MD5_DIGEST_LENGTH * 2] = '\0'; // Null-terminate the string
}
// Compute hash
static void compute_hash(char *data, int data_length, char *hash) {
    compute_hash_md5(data,data_length,hash);
}

int rename_file(const char *oldname, const char *newname) {
    return rename(oldname,newname);
}

#endif

// Record creation function
record_t* record__static__new_from_buffer(int start, char* data, int data_length) {
    if (!data || data_length <= 0) return NULL;

    record_t *record = (record_t*)malloc(sizeof(record_t));
    if (!record) return NULL;

    compute_hash(data, data_length, record->id); // Using SHA-256 here
    record->start = start;
    record->end = start + data_length;
    return record;
}
/**
 * Allocates a buffer large enough to hold the content of a record.
 * The caller is responsible for freeing the allocated memory.
 * Returns NULL if the record is deleted or if the allocation fails.
 */
char* record__instance__allocate_content_buffer(const record_t *self) {
    if (!self || record__instance__is_deleted(self)) {
        fprintf(stderr, "Error: Cannot allocate buffer for a deleted record or NULL record\n");
        return NULL;
    }

    // Calculate the buffer size
    size_t content_size = self->end - self->start;
    if (content_size == 0) {
        fprintf(stderr, "Error: Record has no content\n");
        return NULL;
    }

    // Allocate the buffer
    char *buffer = (char *)malloc(content_size + 1); // +1 for null-termination
    if (!buffer) {
        perror("Failed to allocate memory for content buffer");
        return NULL;
    }

    buffer[content_size] = '\0'; // Null-terminate
    return buffer;
}

/**
 * Creates a copy of the given record.
 * The caller is responsible for freeing the returned copy.
 */
record_t* record__instance__copy(const record_t *self) {
    if (!self) return NULL;

    // Allocate memory for the new record
    record_t *copy = (record_t *)malloc(sizeof(record_t));
    if (!copy) return NULL;

    // Copy the content of the original record to the new one
    memcpy(copy, self, sizeof(record_t));

    return copy;
}


int record__instance__is_deleted(const record_t *record) {
    if (!record) return 0; // Null records are not considered deleted
    return record->start == 0 && record->end == 0;
}

// create a database object
database_t* database__static_new(const char* path) {
    if (!path) {
        fprintf(stderr, "Error: Path cannot be NULL\n");
        return NULL;
    }

    // Allocate memory for the database instance
    database_t *db = (database_t*)malloc(sizeof(database_t));
    if (!db) {
        fprintf(stderr, "Error: Failed to allocate memory for database\n");
        return NULL;
    }

    // Initialize fields
    db->path = strdup(path); // Duplicate the path
    if (!db->path) {
        fprintf(stderr, "Error: Failed to allocate memory for path\n");
        free(db);
        return NULL;
    }

    db->data_file_reference = -1;
    db->index_file_reference = -1;
    db->record_list = NULL;
    db->record_list_length = 0;

    return db;
}
// Open a database
error_t database__static_open(database_t* self) {
    if (!self || !self->path) {
        fprintf(stderr, "Error: Database object or path is uninitialized\n");
        return -1;
    }

    // Open the data and index files
    char data_file_path[256];
    char index_file_path[256];
    snprintf(data_file_path, sizeof(data_file_path), "%s.data", self->path);
    snprintf(index_file_path, sizeof(index_file_path), "%s.index", self->path);

    self->data_file_reference = open(data_file_path, O_RDWR | O_CREAT, 0666);
    self->index_file_reference = open(index_file_path, O_RDWR | O_CREAT, 0666);

    if (self->data_file_reference == -1 || self->index_file_reference == -1) {
        perror("Failed to open database files");
        return -1;
    }

    // Load records from the index file
    struct stat st;
    if (fstat(self->index_file_reference, &st) == 0 && st.st_size > 0) {
        self->record_list_length = st.st_size / sizeof(record_t);
        self->record_list = (record_t*)malloc(st.st_size);
        if (!self->record_list) {
            fprintf(stderr, "Error: Failed to allocate memory for record list\n");
            return -1;
        }
        pread(self->index_file_reference, self->record_list, st.st_size, 0);
    }

    return 0;
}

// Close a database
error_t database__static__close(database_t* self) {
    if (!self) return -1;

    if (self->data_file_reference >= 0) {
        if (close(self->data_file_reference) == -1) {
            perror("Failed to close data file");
            return -1;
        }
    }

    if (self->index_file_reference >= 0) {
        if (close(self->index_file_reference) == -1) {
            perror("Failed to close index file");
            return -1;
        }
    }

    return 0;
}

error_t database__static__free(database_t* self) {
    if (!self) return -1;

    if (self->path) free((void*)self->path);
    if (self->record_list) free(self->record_list);

    free(self);
    return 0;
}

// Insert a record
record_t* database__instance__insert_record(database_t* self, char* data, int data_length) {
    if (!self || !data || data_length <= 0) return NULL;

    // Determine the position to write the new record's content
    lseek(self->data_file_reference, 0, SEEK_END);
    int start = lseek(self->data_file_reference, 0, SEEK_CUR);
    write(self->data_file_reference, data, data_length);

    // Create a new record
    record_t *record = record__static__new_from_buffer(start, data, data_length);
    if (!record) return NULL;

    // Add the record to the record list
    self->record_list = (record_t*)realloc(self->record_list, (self->record_list_length + 1) * sizeof(record_t));
    self->record_list[self->record_list_length++] = *record;

    // Write the new record to the index file
    pwrite(self->index_file_reference, record, sizeof(record_t), (self->record_list_length - 1) * sizeof(record_t));

    return record;
}

// Delete a record
record_t* database__instance__delete_record(database_t* self, record_t* record) {
    if (!self || !record) return NULL;

    // Create a new record with the same ID but with start and end set to 0
    record_t deleted_record = *record;
    deleted_record.start = 0;
    deleted_record.end = 0;

    // Add the deleted record to the record list
    self->record_list = (record_t*)realloc(self->record_list, (self->record_list_length + 1) * sizeof(record_t));
    self->record_list[self->record_list_length++] = deleted_record;

    // Write the deleted record to the index file
    pwrite(self->index_file_reference, &deleted_record, sizeof(record_t), (self->record_list_length - 1) * sizeof(record_t));

    return &self->record_list[self->record_list_length - 1];
}

/**
 * Reads the content of a record from the database.
 * 
 * @param self - The database instance.
 * @param record - The record whose content needs to be read.
 * @param content - A pre-allocated buffer to store the record content.
 *                  The buffer size should be at least (record->end - record->start).
 * 
 * @return 0 on success, -1 on failure.
 */
error_t database__instance__get_record_content(database_t* self, record_t* record, char* content) {
    if (!self || !record || !content) return -1;

    // Check if the record is deleted
    if (record__instance__is_deleted(record)) {
        fprintf(stderr, "Cannot read content: Record is deleted\n");
        return -1;
    }

    // Calculate the size of the content to read
    size_t content_size = record->end - record->start;
    if (content_size == 0) {
        fprintf(stderr, "Cannot read content: Record has no data\n");
        return -1;
    }

    // Seek to the record's start position in the data file
    if (lseek(self->data_file_reference, record->start, SEEK_SET) == (off_t)-1) {
        perror("Failed to seek to record start");
        return -1;
    }

    // Read the record content into the buffer
    if (read(self->data_file_reference, content, content_size) != (ssize_t)content_size) {
        perror("Failed to read record content");
        return -1;
    }

    // Null-terminate the content for safety
    content[content_size] = '\0';

    return 0;
}

// List all records
error_t database__instance__list_all(database_t* self, record_found_fn on_record_found) {
    if (!self || !on_record_found) return -1;

    for (size_t i = 0; i < self->record_list_length; i++) {
        on_record_found(&self->record_list[i], i);
    }
    return 0;
}

/**
 * Aggregates all the records with no sorting.
 * 
 * @param self - The database instance.
 * @param on_record_found - The callback function for each record.
 * @return 0 on success, -1 on failure.
 */
error_t database__instance__aggregate_all(database_t* self, record_aggregator_fn on_record_found, void* context) {
    if (!self || !on_record_found) return -1;

    for (size_t i = 0; i < self->record_list_length; i++) {
        record_t *record = &self->record_list[i];

        // Call the callback function
        error_t result = on_record_found(context, record, (int)i);
        if (result != 0) return result; // Stop if the callback signals an error
    }

    return 0;
}

// List all records with content
error_t database__instance__list_all_with_content(database_t* self, record_found_with_content_fn on_record_with_content_found) {
    if (!self || !on_record_with_content_found) return -1;

    for (size_t i = 0; i < self->record_list_length; i++) {
        record_t *record = &self->record_list[i];
        size_t content_size = record->end - record->start;

        if (content_size == 0) continue;

        char *content = record__instance__allocate_content_buffer(record);
        if (!content) return -1;

        if (lseek(self->data_file_reference, record->start, SEEK_SET) == (off_t)-1) {
            free(content);
            return -1;
        }

        if (read(self->data_file_reference, content, content_size) != (ssize_t)content_size) {
            free(content);
            return -1;
        }

        content[content_size] = '\0';

        error_t result = on_record_with_content_found(record, (int)i, content);
        free(content);

        if (result != 0) return result;
    }

    return 0;
}

// Helper function to check if a record is already in the list
static int is_id_in_list(const char **processed_ids, size_t count, const char *id) {
    for (size_t i = 0; i < count; i++) {
        if (strncmp(processed_ids[i], id, 32) == 0) {
            return 1; // ID found
        }
    }
    return 0;
}

/**
 * Iterates over the latest, non-deleted records.
 * Calls the provided callback function for each valid record.
 */
error_t database__instance__get_latest_records(database_t *self, record_found_fn on_record_found) {
    if (!self || !on_record_found) return -1;

    // Allocate memory for tracking processed IDs
    const char **processed_ids = (const char **)calloc(self->record_list_length, sizeof(char *));
    if (!processed_ids) return -1;

    size_t processed_count = 0;

    // Iterate in reverse order to prioritize the latest records
    for (ssize_t i = self->record_list_length - 1; i >= 0; i--) {
        record_t *record = &self->record_list[i];

        // Skip if the record ID is already processed
        if (is_id_in_list(processed_ids, processed_count, record->id)) continue;

        // If the record is deleted, add its ID to processed and skip
        if (record__instance__is_deleted(record)) {
            processed_ids[processed_count++] = record->id;
            continue;
        }

        // Yield the record via the callback
        error_t result = on_record_found(record, (int)processed_count);
        if (result != 0) {
            free(processed_ids);
            return result; // Stop on callback error
        }

        // Mark the record ID as processed
        processed_ids[processed_count++] = record->id;
    }

    // Cleanup
    free(processed_ids);
    return 0;
}

/**
 * Aggregates the latest records with no sorting.
 * 
 * @param self - The database instance.
 * @param on_record_found - The callback function for each record.
 * @param context - User-provided context passed to the callback.
 * @return 0 on success, -1 on failure.
 */
error_t database__instance__aggregate_latest_records(database_t* self, record_aggregator_fn on_record_found, void* context) {
    if (!self || !on_record_found) return -1;

    // Allocate memory for tracking processed IDs
    const char **processed_ids = (const char **)calloc(self->record_list_length, sizeof(char *));
    if (!processed_ids) return -1;

    size_t processed_count = 0;

    for (ssize_t i = self->record_list_length - 1; i >= 0; i--) {
        record_t *record = &self->record_list[i];

        // Skip if the record ID is already processed
        int already_processed = 0;
        for (size_t j = 0; j < processed_count; j++) {
            if (strncmp(processed_ids[j], record->id, 32) == 0) {
                already_processed = 1;
                break;
            }
        }
        if (already_processed) continue;

        // If the record is deleted, mark the ID as processed and skip
        if (record__instance__is_deleted(record)) {
            processed_ids[processed_count++] = record->id;
            continue;
        }

        // Call the callback for the latest record
        error_t result = on_record_found(context, record, (int)processed_count);
        if (result != 0) {
            free(processed_ids);
            return result;
        }

        // Mark the record ID as processed
        processed_ids[processed_count++] = record->id;
    }

    free(processed_ids);
    return 0;
}

// Optimize the database
error_t database__instance__optimize(database_t *self) {
    if (!self) return -1;

    // Generate the name for the temporary database
    char temp_db_path[256];
    snprintf(temp_db_path, 256, "%s_temp", self->path);

    printf(" - creating the temporary database %s\n",temp_db_path);fflush(stdout);

    // Open the temporary database
    database_t *temp_db = database__static_new(temp_db_path);
    database__static_open(temp_db);
    if (!temp_db) {
        perror("Failed to open temporary database");
        return -1;
    }

    // Iterate through the latest records
    printf(" - reserving space for a list for already processed records %d items\n",self->record_list_length);fflush(stdout);
    const char **processed_ids = (const char **)calloc(self->record_list_length, sizeof(char *));
    if (!processed_ids) {
        database__static__close(temp_db);
        return -1;
    }

    size_t processed_count = 0;
    printf(" - starting iterating records\n");fflush(stdout);
    for (ssize_t i = self->record_list_length - 1; i >= 0; i--) {
        record_t *record = record__instance__copy(&(self->record_list[i]));

        printf(" - copying record %03lu with id %032x \n",i,record->id);fflush(stdout);


        // Skip if the record ID is already processed
        if (is_id_in_list(processed_ids, processed_count, record->id)) {
            printf(" - skipping already processed record %03lu \n",i,record->id);fflush(stdout);
            continue;
        }

        // If the record is deleted, add its ID to processed and skip
        if (record__instance__is_deleted(record)) {
            printf(" - skipping deleted record %03lu \n",i,record->id);fflush(stdout);
            processed_ids[processed_count++] = record->id;
            continue;
        }

        // Mark the record ID as processed
        processed_ids[processed_count++] = record->id;

        // Read the record's content from the original database
        printf(" - copying record content %03lu with id %032x \n",i,record->id);fflush(stdout);
        size_t content_size = record->end - record->start;
        char *buffer = record__instance__allocate_content_buffer(record);

        database__instance__get_record_content(self,record,buffer);

        printf(" - insert record content %03lu with id %032x \n",i,record->id);fflush(stdout);
        // Insert the record into the temporary database
        record_t* new_record = database__instance__insert_record(temp_db, buffer, content_size);
        if(strncmp(record->id,new_record->id,32) != 0) {
            printf("   - somehow the old record id %032x is different of the new one %032x",record->id,new_record->id);
        }

        free(buffer);
    }

    printf(" - freeing processed_ids \n");fflush(stdout);
    free(processed_ids);

    printf(" - renaming data and index files \n");fflush(stdout);

    printf("   - generating original data files paths \n");fflush(stdout);
    char original_data_path[256];
    char original_index_path[256];
    snprintf(original_data_path, 256, "%s.data", self->path);
    snprintf(original_index_path, 256, "%s.index", self->path);

    printf("   - generating temporary data files paths \n");fflush(stdout);
    char temp_data_path[256];
    char temp_index_path[256];
    snprintf(temp_data_path, 256, "%s.data", temp_db_path);
    snprintf(temp_index_path, 256, "%s.index", temp_db_path);


    ///// printf(" - closing database \n");fflush(stdout);
    ///// // Close the original database and replace it with the temporary database
    ///// database__static__close(self);
    ///// 
    printf(" - closing temporary database \n");fflush(stdout);
    // Close the temporary database (now the main database)
    database__static__close(temp_db);
    database__static__close(self);

    printf("   - removing original data files \n");fflush(stdout);
    remove(original_data_path);
    remove(original_index_path);

    printf("   - renaming new data files \n");fflush(stdout);
    if (rename(temp_data_path, original_data_path) == -1) {
        perror("Failed to rename temp data file");
        return -1;
    }
    if (rename(temp_index_path, original_index_path) == -1) {
        perror("Failed to rename temp index file");
        return -1;
    }

    printf("   - reopening new data files \n");fflush(stdout);
    database__static_open(self);

    return 0;
}
