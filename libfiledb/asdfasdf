from ctypes import CDLL, Structure, POINTER, CFUNCTYPE, c_char_p, c_int, c_size_t, c_void_p, create_string_buffer

import platform
from typing import Callable, Optional


class Record(Structure):
    _fields_ = [
        ("id", c_char_p),  # UUID as a 32-byte string
        ("start", c_size_t),  # Start position in the data file
        ("end", c_size_t),  # End position in the data file
    ]

    @staticmethod
    def create_from_buffer(lib, start: int, data: bytes) -> "Record":
        """
        Create a new record using the given buffer.

        :param lib: The loaded library instance.
        :param start: Start position of the record.
        :param data: Content buffer.
        :param data_length: Length of the content buffer.
        :return: A new Record instance.
        """
        lib.record__static__new_from_buffer.argtypes = [c_int, c_char_p, c_int]
        lib.record__static__new_from_buffer.restype = POINTER(Record)

        record_ptr = lib.record__static__new_from_buffer(start, data, len(data))
        if not record_ptr:
            raise RuntimeError("Failed to create record.")
        return record_ptr.contents

    def is_deleted(self, lib) -> bool:
        """
        Check if the record is deleted.

        :param lib: The loaded library instance.
        :return: True if deleted, False otherwise.
        """
        lib.record__instance__is_deleted.argtypes = [POINTER(Record)]
        lib.record__instance__is_deleted.restype = c_int

        result = lib.record__instance__is_deleted(POINTER(Record)(self))
        return bool(result)

    def copy(self, lib) -> "Record":
        """
        Create a copy of the record.

        :param lib: The loaded library instance.
        :return: A new Record instance.
        """
        lib.record__instance__copy.argtypes = [POINTER(Record)]
        lib.record__instance__copy.restype = POINTER(Record)

        copy_ptr = lib.record__instance__copy(POINTER(Record)(self))
        if not copy_ptr:
            raise RuntimeError("Failed to copy record.")
        return copy_ptr.contents


class Database(Structure):
    _fields_ = [
        ("path", c_char_p),  # Database path
        ("data_file_reference", c_int),  # Reference to the data file
        ("index_file_reference", c_int),  # Reference to the index file
        ("record_list", POINTER(POINTER(Record))),  # Pointer to a list of records
        ("record_list_length", c_size_t),  # Length of the record list
    ]


class FileDB:
    def __init__(self,library_path: str = None):
        """
        Initialize the wrapper around the database library.

        :param lib_path: Path to the compiled C library.
        """
        
        lib_name = library_path if library_path != None else self._get_library_name()
        try:
            self.lib = CDLL(lib_name)
        except OSError as e:
            raise RuntimeError(f"Failed to load library: {lib_name}. Ensure it is in your PATH. Error: {e}")
        # Define return and argument types for FFI functions
        self.lib.database__static_new.argtypes = [c_char_p]
        self.lib.database__static_new.restype = POINTER(Database)

        self.lib.database__static_open.argtypes = [POINTER(Database)]
        self.lib.database__static_open.restype = c_int

        self.lib.database__static__close.argtypes = [POINTER(Database)]
        self.lib.database__static__close.restype = c_int

        self.lib.database__instance__insert_record.argtypes = [POINTER(Database), c_char_p, c_int]
        self.lib.database__instance__insert_record.restype = POINTER(Record)

        self.lib.database__instance__delete_record.argtypes = [POINTER(Database), POINTER(Record)]
        self.lib.database__instance__delete_record.restype = POINTER(Record)

        self.lib.database__instance__get_record_content.argtypes = [POINTER(Database), POINTER(Record), c_char_p]
        self.lib.database__instance__get_record_content.restype = c_int
        # Define argument and return types for relevant FFI functions
        self.lib.database__instance__list_all.argtypes = [
            POINTER(Database),
            c_void_p,
        ]
        self.lib.database__instance__list_all.restype = c_int

        self.lib.database__instance__aggregate_all.argtypes = [
            POINTER(Database),
            c_void_p,
            c_void_p,
        ]
        self.lib.database__instance__aggregate_all.restype = c_int

        self.lib.database__instance__get_latest_records.argtypes = [
            POINTER(Database),
            c_void_p,
        ]
        self.lib.database__instance__get_latest_records.restype = c_int

        self.lib.database__instance__aggregate_latest_records.argtypes = [
            POINTER(Database),
            c_void_p,
            c_void_p,
        ]
        self.lib.database__instance__aggregate_latest_records.restype = c_int

        self.lib.database__instance__list_all_with_content.argtypes = [
            POINTER(Database),
            c_void_p,
        ]
        self.lib.database__instance__list_all_with_content.restype = c_int

    @staticmethod
    def _get_library_name() -> str:
        """
        Determines the correct library name based on the operating system.

        :return: The name of the dynamic library.
        """
        system = platform.system()
        if system == "Windows":
            return "libfiledb.dll"
        elif system == "Linux":
            return "libfiledb.so"
        elif system == "Darwin":
            return "libfiledb.dynlib"
        else:
            raise RuntimeError(f"Unsupported operating system: {system}")

    def create_database(self, path: str) -> Database:
        """
        Create and initialize a database instance.

        :param path: Path to the database files.
        :return: A Database instance.
        """
        db_ptr = self.lib.database__static_new(path.encode())
        if not db_ptr:
            raise RuntimeError("Failed to create database instance.")
        return db_ptr.contents

    def open_database(self, db: Database):
        """
        Open an existing database.

        :param db: The database instance.
        """
        if self.lib.database__static_open(POINTER(Database)(db)) != 0:
            raise RuntimeError("Failed to open database.")

    def close_database(self, db: Database):
        """
        Close the database files.

        :param db: The database instance.
        """
        if self.lib.database__static__close(POINTER(Database)(db)) != 0:
            raise RuntimeError("Failed to close database.")

    def insert_record(self, db: Database, data: bytes) -> Record:
        """
        Insert a new record into the database.

        :param db: The database instance.
        :param data: Binary data of the record.
        :return: A Record instance.
        """
        record_ptr = self.lib.database__instance__insert_record(
            POINTER(Database)(db), data, len(data)
        )
        if not record_ptr:
            raise RuntimeError("Failed to insert record.")
        return record_ptr.contents
    
    def delete_record(self, db: Database, record: Record) -> Record:
        """
        Delete record into the database.

        :param db: The database instance.
        :param record: Record record.
        :return: Record The deleted Record instance.
        """
        record_ptr = self.lib.database__instance__delete_record(
            POINTER(Database)(db), POINTER(Record)(record)
        )
        if not record_ptr:
            raise RuntimeError("Failed to delete record.")
        record = Record()
        return record_ptr.contents

    def get_record_content(self, db: Database, record: Record) -> bytes:
        """
        Retrieve the content of a record.

        :param db: The database instance.
        :param record: The record whose content is to be retrieved.
        :return: The content of the record as bytes.
        """
        content_buffer = create_string_buffer(record.end - record.start)
        result = self.lib.database__instance__get_record_content(
            POINTER(Database)(db), POINTER(Record)(record), content_buffer
        )
        if result != 0:
            raise RuntimeError("Failed to get record content.")
        return content_buffer.value
    

    def list_all_records(self, db: Database, callback: Callable[[Record, int], None]):
        """
        List all records in the database.

        :param db: The database instance.
        :param callback: A Python function that takes a Record and its ordinal as arguments.
        """
        @CFUNCTYPE(c_int, POINTER(Record), c_int)
        def c_callback(record_ptr, ordinal):
            record = record_ptr.contents
            callback(record, ordinal)
            return 0

        result = self.lib.database__instance__list_all(POINTER(Database)(db), c_callback)
        if result != 0:
            raise RuntimeError("Failed to list all records.")

    def aggregate_all_records(
        self, db: Database, callback: Callable[[object, Record, int], None], context: object
    ):
        """
        Aggregate all records in the database.

        :param db: The database instance.
        :param callback: A Python function that takes a context object, a Record, and its ordinal as arguments.
        :param context: An arbitrary Python object to pass to the callback.
        """
        @CFUNCTYPE(c_int, c_void_p, POINTER(Record), c_int)
        def c_callback(context_ptr, record_ptr, ordinal):
            record = record_ptr.contents
            callback(context, record, ordinal)
            return 0

        result = self.lib.database__instance__aggregate_all(
            POINTER(Database)(db), c_callback, id(context)
        )
        if result != 0:
            raise RuntimeError("Failed to aggregate all records.")

    def get_latest_records(self, db: Database, callback: Callable[[Record, int], None]):
        """
        Get the latest, non-deleted records.

        :param db: The database instance.
        :param callback: A Python function that takes a Record and its ordinal as arguments.
        """
        @CFUNCTYPE(c_int, POINTER(Record), c_int)
        def c_callback(record_ptr, ordinal):
            record = record_ptr.contents
            callback(record, ordinal)
            return 0

        result = self.lib.database__instance__get_latest_records(POINTER(Database)(db), c_callback)
        if result != 0:
            raise RuntimeError("Failed to get latest records.")

    def aggregate_latest_records(
        self, db: Database, callback: Callable[[object, Record, int], None], context: object
    ):
        """
        Aggregate the latest, non-deleted records.

        :param db: The database instance.
        :param callback: A Python function that takes a context object, a Record, and its ordinal as arguments.
        :param context: An arbitrary Python object to pass to the callback.
        """
        @CFUNCTYPE(c_int, c_void_p, POINTER(Record), c_int)
        def c_callback(context_ptr, record_ptr, ordinal):
            record = record_ptr.contents
            callback(context, record, ordinal)
            return 0

        result = self.lib.database__instance__aggregate_latest_records(
            POINTER(Database)(db), c_callback, id(context)
        )
        if result != 0:
            raise RuntimeError("Failed to aggregate latest records.")

    def list_all_with_content(
        self, db: Database, callback: Callable[[Record, int, bytes], None]
    ):
        """
        List all records along with their content.

        :param db: The database instance.
        :param callback: A Python function that takes a Record, its ordinal, and its content as arguments.
        """
        @CFUNCTYPE(c_int, POINTER(Record), c_int, c_char_p)
        def c_callback(record_ptr, ordinal, content_ptr):
            record = record_ptr.contents
            content = content_ptr.decode()
            callback(record, ordinal, content)
            return 0

        result = self.lib.database__instance__list_all_with_content(POINTER(Database)(db), c_callback)
        if result != 0:
            raise RuntimeError("Failed to list all records with content.")

