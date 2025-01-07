import unittest
import os
from tempfile import TemporaryDirectory
from ctypes import POINTER
from filedb import FileDB, Record, Database  # Replace with the actual module name


class TestFileDB(unittest.TestCase):
    def setUp(self):
        """
        Set up the test environment. This method runs before each test.
        """
        self.db_wrapper = FileDB("/home/alfu64/Development/interview-questionnaire/libfiledb/libfiledb.so")
        self.db_path = os.path.join(".", "testdb")
        self.database = self.db_wrapper.create_database(self.db_path)

        if not self.database:
            self.fail("Failed to create database instance.")

    def tearDown(self):
        """
        Clean up the test environment. This method runs after each test.
        """
        # if self.database:
        #     self.db_wrapper.close_database(self.database)
        #     self.db_wrapper.free_database(self.database)

    def test_create_database(self):
        """
        Test creating a database instance.
        """
        self.assertIsNotNone(self.database, "Database instance should not be None.")
        self.assertIsInstance(self.database, Database, "Should return a Database instance.")

    def test_open_database(self):
        """
        Test opening the database.
        """
        result = self.db_wrapper.open_database(self.database)
        self.assertEqual(result, 0, "Opening database should return 0 on success.")

    def test_insert_record(self):
        """
        Test inserting a record into the database.
        """
        self.db_wrapper.open_database(self.database)

        data = b"Test record data"
        record = self.db_wrapper.insert_record(self.database, data, len(data))
        self.assertIsNotNone(record, "Inserted record should not be None.")
        self.assertIsInstance(record, Record, "Should return a Record instance.")

    def test_get_record_content(self):
        """
        Test retrieving record content.
        """
        self.db_wrapper.open_database(self.database)

        data = b"Test record data"
        record = self.db_wrapper.insert_record(self.database, data, len(data))
        self.assertIsNotNone(record, "Inserted record should not be None.")

        content_buffer = bytearray(len(data))
        self.db_wrapper.lib.database__instance__get_record_content(
            POINTER(Database)(self.database), POINTER(Record)(record), content_buffer
        )
        self.assertEqual(content_buffer.decode(), data.decode(), "Record content should match inserted data.")

    def test_close_database(self):
        """
        Test closing the database.
        """
        result = self.db_wrapper.close_database(self.database)
        self.assertEqual(result, 0, "Closing database should return 0 on success.")

    def test_free_database(self):
        """
        Test freeing the database memory.
        """
        result = self.db_wrapper.free_database(self.database)
        self.assertEqual(result, 0, "Freeing database should return 0 on success.")

    def test_is_record_deleted(self):
        """
        Test checking if a record is deleted.
        """
        self.db_wrapper.open_database(self.database)

        data = b"Test record data"
        record = self.db_wrapper.insert_record(self.database, data, len(data))
        self.assertFalse(
            self.db_wrapper.is_record_deleted(record), "Newly inserted record should not be marked as deleted."
        )


if __name__ == "__main__":
    unittest.main()
