#include "filedb.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

// Compute SHA-256 hash
void compute_hash_sha256(const char *data, int data_length, char *hash) {
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

// Record creation function
record_t* record__static__new_from_buffer(int start, char* data, int data_length) {
    if (!data || data_length <= 0) return NULL;

    record_t *record = (record_t*)malloc(sizeof(record_t));
    if (!record) return NULL;

    compute_hash_sha256(data, data_length, record->id); // Using SHA-256 here
    record->start = start;
    record->end = start + data_length;
    return record;
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

    lseek(self->data_file_reference, 0, SEEK_END);
    int start = lseek(self->data_file_reference, 0, SEEK_CUR);
    write(self->data_file_reference, data, data_length);

    record_t *record = record__static__new_from_buffer(start, data, data_length);
    if (!record) return NULL;

    self->record_list = (record_t*)realloc(self->record_list, (self->record_list_length + 1) * sizeof(record_t));
    self->record_list[self->record_list_length++] = *record;

    pwrite(self->index_file_reference, record, sizeof(record_t), self->record_list_length * sizeof(record_t));
    return record;
}

// Delete a record
record_t* database__instance__delete_record(database_t* self, record_t* record) {
    if (!self || !record) return NULL;

    record_t deleted_record = *record;
    deleted_record.start = deleted_record.end = lseek(self->data_file_reference, 0, SEEK_END);

    self->record_list = (record_t*)realloc(self->record_list, (self->record_list_length + 1) * sizeof(record_t));
    self->record_list[self->record_list_length++] = deleted_record;

    pwrite(self->index_file_reference, &deleted_record, sizeof(record_t), self->record_list_length * sizeof(record_t));
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

// Optimize the database
error_t database__instance__optimize(database_t* self) {
    if (!self) return -1;

    char temp_data_path[256];
    char temp_index_path[256];
    snprintf(temp_data_path, 256, "%s.data.temp", self->path);
    snprintf(temp_index_path, 256, "%s.index.temp", self->path);

    int temp_data_fd = open(temp_data_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    int temp_index_fd = open(temp_index_path, O_RDWR | O_CREAT | O_TRUNC, 0666);

    if (temp_data_fd == -1 || temp_index_fd == -1) {
        if (temp_data_fd != -1) close(temp_data_fd);
        if (temp_index_fd != -1) close(temp_index_fd);
        return -1;
    }

    record_t *latest_records = (record_t*)calloc(self->record_list_length, sizeof(record_t));
    size_t latest_count = 0;

    for (size_t i = 0; i < self->record_list_length; i++) {
        record_t *current = &self->record_list[i];
        int exists = 0;
        for (size_t j = 0; j < latest_count; j++) {
            if (strncmp(latest_records[j].id, current->id, 32) == 0) {
                latest_records[j] = *current;
                exists = 1;
                break;
            }
        }
        if (!exists) {
            latest_records[latest_count++] = *current;
        }
    }

    for (size_t i = 0; i < latest_count; i++) {
        record_t *record = &latest_records[i];
        if (record->start == record->end) continue;

        lseek(self->data_file_reference, record->start, SEEK_SET);
        char *buffer = (char*)malloc(record->end - record->start);
        read(self->data_file_reference, buffer, record->end - record->start);

        off_t new_start = lseek(temp_data_fd, 0, SEEK_END);
        write(temp_data_fd, buffer, record->end - record->start);
        free(buffer);

        record->start = new_start;
        record->end = new_start + (record->end - record->start);
        pwrite(temp_index_fd, record, sizeof(record_t), latest_count * sizeof(record_t));
    }

    free(latest_records);
    close(self->data_file_reference);
    close(self->index_file_reference);
    close(temp_data_fd);
    close(temp_index_fd);

    char old_data_path[256];
    char old_index_path[256];
    snprintf(old_data_path, 256, "%s.data", self->path);
    snprintf(old_index_path, 256, "%s.index", self->path);

    rename(temp_data_path, old_data_path);
    rename(temp_index_path, old_index_path);

    self->data_file_reference = open(old_data_path, O_RDWR, 0666);
    self->index_file_reference = open(old_index_path, O_RDWR, 0666);

    return 0;
}
