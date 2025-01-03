#include "filedb.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Compute hash
// Helper function to compute a simple hash for record content
static void compute_hash(char *data, int data_length, char *hash) {
    unsigned long long sum = 0;
    for (int i = 0; i < data_length; i++) {
        sum += (unsigned char)data[i] * (i + 1);
    }
    snprintf(hash, 32, "%032llx", sum);
}

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

int rename_file(const char *oldname, const char *newname) {
    if (MoveFileEx(oldname, newname, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) == 0) {
        fprintf(stderr, "MoveFileEx failed with error code: %lu\n", GetLastError());
        return -1;
    }
    return 0;
}
#else

///////// #include <openssl/sha.h>
///////// #include <openssl/md5.h>
///////// 
///////// // Compute SHA-256 hash
///////// void compute_hash_sha256(const char *data, int data_length, char *hash) {
/////////     unsigned char digest[SHA256_DIGEST_LENGTH];
/////////     SHA256((unsigned char *)data, data_length, digest);
/////////     for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
/////////         sprintf(hash + (i * 2), "%02x", digest[i]);
/////////     }
/////////     hash[SHA256_DIGEST_LENGTH * 2] = '\0'; // Null-terminate the string
///////// }
///////// 
///////// // Compute MD5 hash
///////// void compute_hash_md5(const char *data, int data_length, char *hash) {
/////////     unsigned char digest[MD5_DIGEST_LENGTH];
/////////     MD5((unsigned char *)data, data_length, digest);
/////////     for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
/////////         sprintf(hash + (i * 2), "%02x", digest[i]);
/////////     }
/////////     hash[MD5_DIGEST_LENGTH * 2] = '\0'; // Null-terminate the string
///////// }
///////// // Compute hash
///////// void compute_hash(const char *data, int data_length, char *hash) {
/////////     compute_hash_md5(data,data_length,hash);
///////// }

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

int record__instance__is_deleted(const record_t *record) {
    if (!record) return 0; // Null records are not considered deleted
    return record->start == 0 && record->end == 0;
}

// Open a database
database_t* database__static_open(const char* path) {
    if (!path) return NULL;

    database_t *db = (database_t*)malloc(sizeof(database_t));
    if (!db) return NULL;

    db->path = strdup(path);
    db->record_list = NULL;
    db->record_list_length = 0;

    char data_file_path[256];
    char index_file_path[256];
    snprintf(data_file_path, 256, "%s.data", path);
    snprintf(index_file_path, 256, "%s.index", path);

    db->data_file_reference = open(data_file_path, O_RDWR | O_CREAT, 0666);
    db->index_file_reference = open(index_file_path, O_RDWR | O_CREAT, 0666);

    if (db->data_file_reference == -1 || db->index_file_reference == -1) {
        free(db);
        return NULL;
    }

    struct stat st;
    if (fstat(db->index_file_reference, &st) == 0 && st.st_size > 0) {
        db->record_list_length = st.st_size / sizeof(record_t);
        db->record_list = (record_t*)malloc(st.st_size);
        pread(db->index_file_reference, db->record_list, st.st_size, 0);
    }

    return db;
}

// Close a database
error_t database__static__close(database_t* self) {
    if (!self) return -1;

    if (self->data_file_reference >= 0) close(self->data_file_reference);
    if (self->index_file_reference >= 0) close(self->index_file_reference);

    if (self->record_list) free(self->record_list);
    if (self->path) free((void*)self->path);

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


// List all records
error_t database__instance__list_all(database_t* self, record_found_fn on_record_found) {
    if (!self || !on_record_found) return -1;

    for (size_t i = 0; i < self->record_list_length; i++) {
        on_record_found(&self->record_list[i], i);
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

        char *content = (char *)malloc(content_size + 1);
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
error_t database__instance__get_latest_records(database_t *self, record_iter_fn on_record_found) {
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

// Optimize the database
error_t database__instance__optimize(database_t *self) {
    if (!self) return -1;

    // Prepare paths for temporary files
    char temp_data_path[256];
    char temp_index_path[256];
    snprintf(temp_data_path, 256, "%s.data.temp", self->path);
    snprintf(temp_index_path, 256, "%s.index.temp", self->path);

    // Open temporary files
    int temp_data_fd = open(temp_data_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    int temp_index_fd = open(temp_index_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (temp_data_fd == -1 || temp_index_fd == -1) {
        if (temp_data_fd != -1) close(temp_data_fd);
        if (temp_index_fd != -1) close(temp_index_fd);
        return -1;
    }

    // Callback to write valid records to temporary files
    error_t write_record_to_temp(record_t *record, int ord) {
        size_t content_size = record->end - record->start;

        // Read the record's content from the original data file
        char *buffer = (char *)malloc(content_size);
        if (!buffer) return -1;
        lseek(self->data_file_reference, record->start, SEEK_SET);
        read(self->data_file_reference, buffer, content_size);

        // Write the content to the temporary data file
        off_t new_start = lseek(temp_data_fd, 0, SEEK_END);
        write(temp_data_fd, buffer, content_size);
        free(buffer);

        // Update the record's start and end for the new data file
        record->start = new_start;
        record->end = new_start + content_size;

        // Write the updated record to the temporary index file
        pwrite(temp_index_fd, record, sizeof(record_t), ord * sizeof(record_t));

        return 0;
    }

    // Use the iterator to process the latest, non-deleted records
    error_t result = database__instance__get_latest_records(self, write_record_to_temp);
    if (result != 0) {
        close(temp_data_fd);
        close(temp_index_fd);
        return result;
    }

    // Cleanup
    close(self->data_file_reference);
    close(self->index_file_reference);
    close(temp_data_fd);
    close(temp_index_fd);

    // Replace old files with optimized files
    char old_data_path[256];
    char old_index_path[256];
    snprintf(old_data_path, 256, "%s.data", self->path);
    snprintf(old_index_path, 256, "%s.index", self->path);

    if (rename(temp_data_path, old_data_path) == -1) {
        perror("Failed to rename temp_data_path to old_data_path");
        return -1;
    }
    if (rename(temp_index_path, old_index_path) == -1) {
        perror("Failed to rename temp_index_path to old_index_path");
        return -1;
    }

    // Reopen the new files for the database
    self->data_file_reference = open(old_data_path, O_RDWR, 0666);
    self->index_file_reference = open(old_index_path, O_RDWR, 0666);

    return 0;
}
