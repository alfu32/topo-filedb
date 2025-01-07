from cffi import FFI

ffi = FFI()

# Define the C header
ffi.cdef("""
typedef struct record_s {
    char id[32];
    size_t start;
    size_t end;
} record_t;

typedef struct database_s {
    const char* path;
    int data_file_reference;
    int index_file_reference;
    record_t* record_list;
    size_t record_list_length;
} database_t;

typedef int error_t;

record_t* record__static__new_from_buffer(int start, char* data, int data_length);
char* record__instance__allocate_content_buffer(const record_t *self);
record_t* record__instance__copy(const record_t *self);
int record__instance__is_deleted(const record_t *record);

database_t* database__static_new(const char* path);
error_t database__static_open(database_t* self);
error_t database__static__close(database_t* self);
error_t database__static__free(database_t* self);
record_t* database__instance__insert_buffer(database_t* self, char* data, int data_length);
record_t* database__instance__insert_record(database_t* self, record_t* record);
record_t* database__instance__delete_record(database_t* self, record_t* record);
error_t database__instance__get_record_content(database_t* self, record_t* record, char* content);
error_t database__instance__list_all(database_t* self, error_t (*on_record_found)(record_t*, int ord));
error_t database__instance__aggregate_all(database_t* self, error_t (*on_record_found)(void* context, record_t*, int ord), void* context);
error_t database__instance__get_latest_records(database_t *self, error_t (*on_record_found)(record_t*, int ord));
error_t database__instance__aggregate_latest_records(database_t* self, error_t (*on_record_found)(void* context, record_t*, int ord), void* context);
error_t database__instance__list_all_with_content(database_t* self, error_t (*on_record_with_content_found)(record_t*, int ord, char* content));
error_t database__instance__optimize(database_t* self);
""")

# Load the shared library
lib = ffi.dlopen('./libfiledb.so')  # Adjust path to your compiled shared library

# Define Python classes for the structures
class Record:
    def __init__(self, start=None, data=None, data_length=None, ptr=None):
        if ptr is not None:
            self.ptr = ptr
        else:
            self.ptr = lib.record__static__new_from_buffer(start, data, data_length)
    
    def allocate_content_buffer(self):
        return ffi.string(lib.record__instance__allocate_content_buffer(self.ptr))

    def copy(self):
        return Record(ptr=lib.record__instance__copy(self.ptr))

    def is_deleted(self):
        return lib.record__instance__is_deleted(self.ptr)


class Database:
    def __init__(self, path):
        self.ptr = lib.database__static_new(path.encode('utf-8'))

    def open(self):
        return lib.database__static_open(self.ptr)

    def close(self):
        return lib.database__static__close(self.ptr)

    def free(self):
        return lib.database__static__free(self.ptr)

    def insert_buffer(self, data, data_length):
        return Record(ptr=lib.database__instance__insert_buffer(self.ptr, ffi.new("char[]", data.encode('utf-8')), data_length))

    def insert_record(self, record):
        return Record(ptr=lib.database__instance__insert_record(self.ptr, record.ptr))

    def delete_record(self, record):
        return Record(ptr=lib.database__instance__delete_record(self.ptr, record.ptr))

    def get_record_content(self, record):
        content = ffi.new("char[]", 1024)  # Adjust buffer size as needed
        lib.database__instance__get_record_content(self.ptr, record.ptr, content)
        return ffi.string(content)

    def list_all(self, callback):
        @ffi.callback("error_t(record_t*, int)")
        def wrapped_callback(record_ptr, ord):
            record = Record(ptr=record_ptr)
            return callback(record, ord)
        return lib.database__instance__list_all(self.ptr, wrapped_callback)

    def aggregate_all(self, callback, context):
        @ffi.callback("error_t(void*, record_t*, int)")
        def wrapped_callback(ctx, record_ptr, ord):
            record = Record(ptr=record_ptr)
            return callback(ctx, record, ord)
        return lib.database__instance__aggregate_all(self.ptr, wrapped_callback, context)

    def get_latest_records(self, callback):
        @ffi.callback("error_t(record_t*, int)")
        def wrapped_callback(record_ptr, ord):
            record = Record(ptr=record_ptr)
            return callback(record, ord)
        return lib.database__instance__get_latest_records(self.ptr, wrapped_callback)

    def list_all_with_content(self, callback):
        @ffi.callback("error_t(record_t*, int, char*)")
        def wrapped_callback(record_ptr, ord, content):
            record = Record(ptr=record_ptr)
            return callback(record, ord, ffi.string(content))
        return lib.database__instance__list_all_with_content(self.ptr, wrapped_callback)

    def optimize(self):
        return lib.database__instance__optimize(self.ptr)


# Example usage
if __name__ == "__main__":
    db = Database("test_db")
    db.open()
    record = db.insert_buffer("sample data", len("sample data"))
    print(record.allocate_content_buffer())
    db.close()
