from ctypes import (
    CDLL,
    Structure,
    POINTER,
    c_char_p,
    c_int,
    c_size_t,
    c_void_p,
)
import platform
from typing import Callable, Optional


class Record(Structure):
    _fields_ = [
        ("id", c_char_p),  # UUID as a 32-byte string
        ("start", c_size_t),  # Start position in the data file
        ("end", c_size_t),  # End position in the data file
    ]


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
        self.lib.record__static__new_from_buffer.argtypes = [c_int, c_char_p, c_int]
        self.lib.record__static__new_from_buffer.restype = POINTER(Record)

        self.lib.record__instance__allocate_content_buffer.argtypes = [POINTER(Record)]
        self.lib.record__instance__allocate_content_buffer.restype = c_char_p

        self.lib.record__instance__copy.argtypes = [POINTER(Record)]
        self.lib.record__instance__copy.restype = POINTER(Record)

        self.lib.record__instance__is_deleted.argtypes = [POINTER(Record)]
        self.lib.record__instance__is_deleted.restype = c_int

        self.lib.database__static_new.argtypes = [c_char_p]
        self.lib.database__static_new.restype = POINTER(Database)

        self.lib.database__static_open.argtypes = [POINTER(Database)]
        self.lib.database__static_open.restype = c_int

        self.lib.database__static__close.argtypes = [POINTER(Database)]
        self.lib.database__static__close.restype = c_int

        self.lib.database__static__free.argtypes = [POINTER(Database)]
        self.lib.database__static__free.restype = c_int

        self.lib.database__instance__insert_record.argtypes = [POINTER(Database), c_char_p, c_int]
        self.lib.database__instance__insert_record.restype = POINTER(Record)

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
    
    def create_record_from_buffer(self, start: int, data: bytes, data_length: int) -> Optional[Record]:
        """
        Create a new record from a buffer.

        :param start: Start position of the record.
        :param data: Binary data of the record.
        :param data_length: Length of the data.
        :return: A new Record instance or None if failed.
        """
        record_ptr = self.lib.record__static__new_from_buffer(start, data, data_length)
        return record_ptr.contents if record_ptr else None

    def allocate_content_buffer(self, record: Record) -> Optional[bytes]:
        """
        Allocate a content buffer for a record.

        :param record: The record instance.
        :return: The allocated buffer as bytes.
        """
        buffer_ptr = self.lib.record__instance__allocate_content_buffer(POINTER(Record)(record))
        return buffer_ptr if buffer_ptr else None

    def copy_record(self, record: Record) -> Optional[Record]:
        """
        Create a copy of the given record.

        :param record: The record to copy.
        :return: A new Record instance or None if failed.
        """
        record_ptr = self.lib.record__instance__copy(POINTER(Record)(record))
        return record_ptr.contents if record_ptr else None

    def is_record_deleted(self, record: Record) -> bool:
        """
        Check if a record is deleted.

        :param record: The record to check.
        :return: True if deleted, False otherwise.
        """
        return bool(self.lib.record__instance__is_deleted(POINTER(Record)(record)))

    def create_database(self, path: str) -> Optional[Database]:
        """
        Create a new database instance.

        :param path: Path to the database files.
        :return: A new Database instance or None if failed.
        """
        db_ptr = self.lib.database__static_new(path.encode())
        return db_ptr.contents if db_ptr else None

    def open_database(self, db: Database) -> int:
        """
        Open an existing database.

        :param db: The database instance.
        :return: 0 on success, non-zero on failure.
        """
        return self.lib.database__static_open(POINTER(Database)(db))

    def close_database(self, db: Database) -> int:
        """
        Close the database files.

        :param db: The database instance.
        :return: 0 on success, non-zero on failure.
        """
        return self.lib.database__static__close(POINTER(Database)(db))

    def free_database(self, db: Database) -> int:
        """
        Free the memory of the database.

        :param db: The database instance.
        :return: 0 on success, non-zero on failure.
        """
        return self.lib.database__static__free(POINTER(Database)(db))

    def insert_record(self, db: Database, data: bytes, data_length: int) -> Optional[Record]:
        """
        Insert a new record into the database.

        :param db: The database instance.
        :param data: Binary data of the record.
        :param data_length: Length of the data.
        :return: A new Record instance or None if failed.
        """
        record_ptr = self.lib.database__instance__insert_record(
            POINTER(Database)(db), data, data_length
        )
        return record_ptr.contents if record_ptr else None
