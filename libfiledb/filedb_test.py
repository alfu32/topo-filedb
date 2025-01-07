import os
import sys
from pathlib import Path

from filedb import Record, Database  # Replace with the actual module name

# Assuming the Database and Record classes are defined as per the previous code

# Test functions

def test_create_database(db_path):
    print("Running: test_create_database",flush=True)
    db = Database(db_path)
    db.open()
    print("Database created and opened successfully.",flush=True)
    db.close()
    db.free()

def test_insert_record(db_path):
    print("Running: test_insert_record",flush=True)
    db = Database(db_path)
    db.open()
    record = db.insert_buffer("test data", len("test data"))
    print(f"Inserted record ID: {ffi.string(record.ptr.id).decode('utf-8')}",flush=True)
    db.close()
    db.free()

def test_insert_record_object(db_path):
    print("Running: test_insert_record_object",flush=True)
    db = Database(db_path)
    db.open()
    original_record = Record(0, "data for record object", len("data for record object"))
    record = db.insert_record(original_record)
    print(f"Inserted record object ID: {ffi.string(record.ptr.id).decode('utf-8')}",flush=True)
    db.close()
    db.free()

def test_get_record_content(db_path):
    print("Running: test_get_record_content",flush=True)
    db = Database(db_path)
    db.open()
    record = db.insert_buffer("test content", len("test content"))
    content = db.get_record_content(record)
    print(f"Record content: {content.decode('utf-8')}",flush=True)
    db.close()
    db.free()

def test_delete_record(db_path):
    print("Running: test_delete_record",flush=True)
    db = Database(db_path)
    db.open()
    record = db.insert_buffer("data to delete", len("data to delete"))
    deleted_record = db.delete_record(record)
    print(f"Deleted record ID: {ffi.string(deleted_record.ptr.id).decode('utf-8')}",flush=True)
    db.close()
    db.free()

def test_list_all_records(db_path):
    print("Running: test_list_all_records",flush=True)
    def on_record_found(record, ord):
        print(f"Record #{ord}, ID: {ffi.string(record.ptr.id).decode('utf-8')}",flush=True)
        return 0  # Success
    db = Database(db_path)
    db.open()
    db.insert_buffer("record 1", len("record 1"))
    db.insert_buffer("record 2", len("record 2"))
    db.list_all(on_record_found)
    db.close()
    db.free()

def test_aggregate_all_records(db_path):
    print("Running: test_aggregate_all_records",flush=True)
    def on_record_aggregate(context, record, ord):
        print(f"Aggregated Record #{ord}, ID: {ffi.string(record.ptr.id).decode('utf-8')}",flush=True)
        return 0
    db = Database(db_path)
    db.open()
    db.insert_buffer("record 1", len("record 1"))
    db.insert_buffer("record 2", len("record 2"))
    db.aggregate_all(on_record_aggregate, None)
    db.close()
    db.free()

def test_get_latest_records(db_path):
    print("Running: test_get_latest_records",flush=True)
    def on_latest_record_found(record, ord):
        print(f"Latest Record #{ord}, ID: {ffi.string(record.ptr.id).decode('utf-8')}",flush=True)
        return 0
    db = Database(db_path)
    db.open()
    db.insert_buffer("latest record", len("latest record"))
    db.get_latest_records(on_latest_record_found)
    db.close()
    db.free()

def test_aggregate_latest_records(db_path):
    print("Running: test_aggregate_latest_records",flush=True)
    def on_latest_aggregate(context, record, ord):
        print(f"Aggregated Latest Record #{ord}, ID: {ffi.string(record.ptr.id).decode('utf-8')}",flush=True)
        return 0
    db = Database(db_path)
    db.open()
    db.insert_buffer("latest record 1", len("latest record 1"))
    db.insert_buffer("latest record 2", len("latest record 2"))
    db.aggregate_latest_records(on_latest_aggregate, None)
    db.close()
    db.free()

def test_list_all_with_content(db_path):
    print("Running: test_list_all_with_content",flush=True)
    def on_record_with_content(record, ord, content):
        print(f"Record #{ord}, ID: {ffi.string(record.ptr.id).decode('utf-8')}, Content: {content.decode('utf-8')}",flush=True)
        return 0
    db = Database(db_path)
    db.open()
    db.insert_buffer("content record 1", len("content record 1"))
    db.insert_buffer("content record 2", len("content record 2"))
    db.list_all_with_content(on_record_with_content)
    db.close()
    db.free()

def test_optimize_database(db_path):
    print("Running: test_optimize_database",flush=True)
    db = Database(db_path)
    db.open()
    db.insert_buffer("old record", len("old record"))
    db.insert_buffer("new record", len("new record"))
    db.optimize()
    print("Database optimized.",flush=True)
    db.close()
    db.free()

# Main function

def main():
    if len(sys.argv) != 2:
        print("Usage: python script.py <database_file_path>",flush=True)
        return
    
    db_path = os.path.join(".", sys.argv[1])
    db_file = Path(db_path)
    
    # if db_file.exists():
    #     print(f"Removing existing database file: {db_path}",flush=True)
    #     db_file.unlink()  # Remove the existing database file for fresh tests

    # List of test functions
    tests = [
        test_create_database,
        test_insert_record,
        test_insert_record_object,
        test_get_record_content,
        test_delete_record,
        test_list_all_records,
        test_aggregate_all_records,
        test_get_latest_records,
        test_aggregate_latest_records,
        test_list_all_with_content,
        test_optimize_database,
    ]

    for n,test in enumerate(tests):
        print(f"\n{'='*40}\nRunning test {n}: {test.__name__}\n{'='*40}",flush=True)
        test(db_path)

if __name__ == "__main__":
    main()
