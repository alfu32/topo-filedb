import unittest
import os
from tempfile import TemporaryDirectory
from ctypes import POINTER, create_string_buffer

from filedb import FileDB, Record, Database  # Replace with the actual module name


class TestFileDB(unittest.TestCase):
    db_wrapper: FileDB = None
    db_path: str = None
    database: Database = None

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
        #     # self.db_wrapper.free_database(self.database)

    def test_record_methods(self):
        """
        Test the methods of the Record class.
        """
        data = b"Record test data"
        record = self.db_wrapper.insert_record(self.database, data)

        # Test allocate_content_buffer
        buffer = record.allocate_content_buffer(self.db_wrapper.lib)
        self.assertEqual(buffer, data, "Allocated content buffer should match the inserted data.")

        # Test copy
        record_copy = record.copy(self.db_wrapper.lib)
        self.assertEqual(record.id, record_copy.id, "Copied record ID should match the original.")
        self.assertEqual(record.start, record_copy.start, "Copied record start position should match.")
        self.assertEqual(record.end, record_copy.end, "Copied record end position should match.")

        # Test is_deleted
        self.assertFalse(record.is_deleted(self.db_wrapper.lib), "Newly created record should not be deleted.")

    
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
        record = self.db_wrapper.insert_record(self.database, data)
        self.assertIsNotNone(record, "Inserted record should not be None.")
        self.assertIsInstance(record, Record, "Should return a Record instance.")

    def test_delete_record(self):
        """
        Test deleting a record.
        """
        data = b"Test delete record"
        record = self.db_wrapper.insert_record(self.database, data)

        deleted_record = self.db_wrapper.delete_record(self.database, record)
        self.assertIsInstance(deleted_record, Record, "Deleted record should be an instance of Record.")
        self.assertTrue(deleted_record.is_deleted(self.db_wrapper.lib), "Deleted record should be marked as deleted.")

    def test_get_record_content(self):
        """
        Test retrieving record content.
        """
        self.db_wrapper.open_database(self.database)

        data = b"Test record data"
        record = self.db_wrapper.insert_record(self.database, data)
        self.assertIsNotNone(record, "Inserted record should not be None.")

        # Use create_string_buffer for a proper ctypes-compatible buffer
        content_buffer = create_string_buffer(len(data))
        result = self.db_wrapper.lib.database__instance__get_record_content(
            POINTER(Database)(self.database), POINTER(Record)(record), content_buffer
        )
        self.assertEqual(result, 0, "Getting record content should return 0 on success.")
        self.assertEqual(content_buffer.value, data, "Record content should match inserted data.")


    def test_close_database(self):
        """
        Test closing the database.
        """
        result = self.db_wrapper.close_database(self.database)
        self.assertEqual(result, 0, "Closing database should return 0 on success.")

    def test_is_record_deleted(self):
        """
        Test checking if a record is deleted.
        """
        self.db_wrapper.open_database(self.database)

        data = b"Test record data"
        record = self.db_wrapper.insert_record(self.database, data)
        self.assertFalse(
            self.db_wrapper.is_record_deleted(record), "Newly inserted record should not be marked as deleted."
        )

    def test_list_all_records(self):
        """
        Test listing all records.
        """
        # Insert sample records
        data1 = b"Test data 1"
        data2 = b"Test data 2"
        self.db_wrapper.insert_record(self.database, data1)
        self.db_wrapper.insert_record(self.database, data2)

        results = []

        def callback(record: Record, ordinal: int):
            results.append((record, ordinal))

        self.db_wrapper.list_all_records(self.database, callback)
        self.assertEqual(len(results), 2, "Should list all records.")
        self.assertEqual(results[0][1], 0, "First record ordinal should be 0.")
        self.assertEqual(results[1][1], 1, "Second record ordinal should be 1.")

    def test_aggregate_all_records(self):
        """
        Test aggregating all records.
        """
        # Insert sample records
        data1 = b"Aggregate data 1"
        data2 = b"Aggregate data 2"
        self.db_wrapper.insert_record(self.database, data1)
        self.db_wrapper.insert_record(self.database, data2)

        aggregated = []

        def callback(context, record: Record, ordinal: int):
            aggregated.append((record, ordinal))

        context = "test_context"
        self.db_wrapper.aggregate_all_records(self.database, callback, context)
        self.assertEqual(len(aggregated), 2, "Should aggregate all records.")

    def test_get_latest_records(self):
        """
        Test getting the latest non-deleted records.
        """
        # Insert and delete records
        data1 = b"Latest record 1"
        data2 = b"Latest record 2"
        record1 = self.db_wrapper.insert_record(self.database, data1)
        self.db_wrapper.insert_record(self.database, data2)
        self.db_wrapper.delete_record(self.database, record1)  # Simulate a delete for record1

        latest = []

        def callback(record: Record, ordinal: int):
            latest.append((record, ordinal))

        self.db_wrapper.get_latest_records(self.database, callback)
        self.assertEqual(len(latest), 1, "Should only list the latest records.")
        self.assertNotEqual(latest[0][0].id, record1.id, "Deleted record should not appear.")

    def test_aggregate_latest_records(self):
        """
        Test aggregating the latest non-deleted records.
        """
        # Insert and delete records
        data1 = b"Aggregate latest 1"
        data2 = b"Aggregate latest 2"
        record1 = self.db_wrapper.insert_record(self.database, data1)
        record2 = self.db_wrapper.insert_record(self.database, data2)
        self.db_wrapper.delete_record(self.database, record2)  # Simulate a delete for record1

        aggregated_latest = []

        def callback(context, record: Record, ordinal: int):
            aggregated_latest.append((record, ordinal))

        context = "aggregate_context"
        self.db_wrapper.aggregate_latest_records(self.database, callback, context)
        self.assertEqual(len(aggregated_latest), 1, "Should only aggregate the latest records.")
        self.assertNotEqual(aggregated_latest[0][0].id, record1.id, "Deleted record should not be aggregated.")

    def test_list_all_with_content(self):
        """
        Test listing all records along with their content.
        """
        # Insert sample records
        data1 = b"Content data 1"
        data2 = b"Content data 2"
        self.db_wrapper.insert_record(self.database, data1)
        self.db_wrapper.insert_record(self.database, data2)

        results_with_content = []

        def callback(record: Record, ordinal: int, content: bytes):
            results_with_content.append((record, ordinal, content))

        self.db_wrapper.list_all_with_content(self.database, callback)
        self.assertEqual(len(results_with_content), 2, "Should list all records with content.")
        self.assertEqual(results_with_content[0][2], data1, "Content should match for record 1.")
        self.assertEqual(results_with_content[1][2], data2, "Content should match for record 2.")



if __name__ == "__main__":
    unittest.main()
