
#include <iostream>
#include <stdexcept>
#include "filedbpp.cpp"


void testCreateFromBuffer() {
    std::cout << "Running testCreateFromBuffer..." << std::endl;

    const std::string content = "Sample Record Content";
    int start = 0;

    Record* record = Record::createFromBuffer(start, content.c_str(), content.size());
    if (!record) throw std::runtime_error("Failed to create record.");

    // Check ID consistency
    Record* duplicate = Record::createFromBuffer(start, content.c_str(), content.size());
    if (std::string(record->id, sizeof(record->id)) != std::string(duplicate->id, sizeof(duplicate->id))) {
        throw std::runtime_error("Record ID mismatch for identical content.");
    }

    delete record;
    delete duplicate;

    std::cout << "testCreateFromBuffer passed!" << std::endl;
}

void testAllocateContentBuffer() {
    std::cout << "Running testAllocateContentBuffer..." << std::endl;

    const std::string content = "Test Buffer Content";
    int start = 0;

    Record* record = Record::createFromBuffer(start, content.c_str(), content.size());
    if (!record) throw std::runtime_error("Failed to create record.");

    char* buffer = record->allocateContentBuffer();
    if (!buffer) {
        delete record;
        throw std::runtime_error("Failed to allocate content buffer.");
    }

    // Check buffer size
    if (static_cast<size_t>(record->end - record->start) != content.size()) {
        delete[] buffer;
        delete record;
        throw std::runtime_error("Buffer size mismatch.");
    }

    delete[] buffer;
    delete record;

    std::cout << "testAllocateContentBuffer passed!" << std::endl;
}

void testCopy() {
    std::cout << "Running testCopy..." << std::endl;

    const std::string content = "Copy Test Content";
    int start = 0;

    Record* original = Record::createFromBuffer(start, content.c_str(), content.size());
    if (!original) throw std::runtime_error("Failed to create record.");

    Record* copy = original->copy();
    if (!copy) {
        delete original;
        throw std::runtime_error("Failed to copy record.");
    }

    // Check properties
    if (std::string(original->id, sizeof(original->id)) != std::string(copy->id, sizeof(copy->id)) ||
        original->start != copy->start || original->end != copy->end) {
        delete original;
        delete copy;
        throw std::runtime_error("Copied record properties do not match original.");
    }

    delete original;
    delete copy;

    std::cout << "testCopy passed!" << std::endl;
}

void testIsDeleted() {
    std::cout << "Running testIsDeleted..." << std::endl;

    const std::string content = "Active Record";
    int start = 0;

    Record* activeRecord = Record::createFromBuffer(start, content.c_str(), content.size());
    if (!activeRecord) throw std::runtime_error("Failed to create record.");

    if (activeRecord->isDeleted()) {
        delete activeRecord;
        throw std::runtime_error("Active record incorrectly marked as deleted.");
    }

    Record deletedRecord = *activeRecord;
    deletedRecord.start = deletedRecord.end;
    if (!deletedRecord.isDeleted()) {
        delete activeRecord;
        throw std::runtime_error("Deleted record incorrectly marked as active.");
    }

    delete activeRecord;

    std::cout << "testIsDeleted passed!" << std::endl;
}

void testInvalidCreateFromBuffer() {
    std::cout << "Running testInvalidCreateFromBuffer..." << std::endl;

    try {
        Record::createFromBuffer(0, nullptr, 10);
        throw std::runtime_error("Expected exception for null data not thrown.");
    } catch (const std::invalid_argument&) {
        // Expected
    }

    try {
        Record::createFromBuffer(0, "Data", 0);
        throw std::runtime_error("Expected exception for zero-length data not thrown.");
    } catch (const std::invalid_argument&) {
        // Expected
    }

    std::cout << "testInvalidCreateFromBuffer passed!" << std::endl;
}



void testCreateAndOpenDatabase() {
    std::cout << "Running testCreateAndOpenDatabase..." << std::endl;

    std::string path = "test_database";
    Database* db = Database::create(path);
    if (!db) throw std::runtime_error("Failed to create database.");

    db->open();

    // Check file existence
    std::ifstream dataFile(path + ".data");
    std::ifstream indexFile(path + ".index");

    if (!dataFile.good() || !indexFile.good()) {
        throw std::runtime_error("Database files were not created.");
    }

    db->close();
    db->free();
    delete db;

    // Clean up
    std::remove((path + ".data").c_str());
    std::remove((path + ".index").c_str());

    std::cout << "testCreateAndOpenDatabase passed!" << std::endl;
}

void testInsertRecord() {
    std::cout << "Running testInsertRecord..." << std::endl;

    std::string path = "test_database";
    Database* db = Database::create(path);
    db->open();

    std::string content = "Sample Record";
    Record* record = db->insertRecord(content.c_str(), content.size());

    if (!record) throw std::runtime_error("Failed to insert record.");
    if (record->start != 0 || record->end != content.size()) {
        throw std::runtime_error("Record start or end positions are incorrect.");
    }

    db->close();
    db->free();
    delete db;

    // Clean up
    std::remove((path + ".data").c_str());
    std::remove((path + ".index").c_str());

    std::cout << "testInsertRecord passed!" << std::endl;
}

void testDeleteRecord() {
    std::cout << "Running testDeleteRecord..." << std::endl;

    std::string path = "test_database";
    Database* db = Database::create(path);
    db->open();

    std::string content = "Record to Delete";
    Record* record = db->insertRecord(content.c_str(), content.size());
    Record* deletedRecord = db->deleteRecord(record);

    if (!deletedRecord) throw std::runtime_error("Failed to delete record.");
    if (!deletedRecord->isDeleted()) {
        throw std::runtime_error("Record is not marked as deleted.");
    }

    db->close();
    db->free();
    delete db;

    // Clean up
    std::remove((path + ".data").c_str());
    std::remove((path + ".index").c_str());

    std::cout << "testDeleteRecord passed!" << std::endl;
}

void testOptimizeDatabase() {
    std::cout << "Running testOptimizeDatabase..." << std::endl;

    std::string path = "test_database";
    Database* db = Database::create(path);
    db->open();

    // Insert multiple versions of a record
    db->insertRecord("Record Version 1", 17);
    db->insertRecord("Record Version 2", 17);

    db->optimize();

    if (db->recordList.size() != 1) {
        throw std::runtime_error("Database optimization failed.");
    }

    db->close();
    db->free();
    delete db;

    // Clean up
    std::remove((path + ".data").c_str());
    std::remove((path + ".index").c_str());

    std::cout << "testOptimizeDatabase passed!" << std::endl;
}

void testGetRecordContent() {
    std::cout << "Running testGetRecordContent..." << std::endl;

    std::string path = "test_database";
    Database* db = Database::create(path);
    db->open();

    std::string content = "Record Content Test";
    Record* record = db->insertRecord(content.c_str(), content.size());

    char* buffer = new char[content.size()];
    db->getRecordContent(record, buffer);

    if (std::string(buffer, content.size()) != content) {
        delete[] buffer;
        throw std::runtime_error("Record content mismatch.");
    }

    delete[] buffer;
    db->close();
    db->free();
    delete db;

    // Clean up
    std::remove((path + ".data").c_str());
    std::remove((path + ".index").c_str());

    std::cout << "testGetRecordContent passed!" << std::endl;
}

// Export directive for Windows (if needed)
#ifdef _WIN32
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C"
#endif

extern "C" {
    int run_main_tests(int argc, const char* argv[]) {
        try {
            std::cout << "=== test CreateFromBuffer        ===================================================" << std::endl;
            testCreateFromBuffer();
            std::cout << "=== test AllocateContentBuffer   ===================================================" << std::endl;
            testAllocateContentBuffer();
            std::cout << "=== test Copy                    ===================================================" << std::endl;
            testCopy();
            std::cout << "=== test IsDeleted               ===================================================" << std::endl;
            testIsDeleted();
            std::cout << "=== test InvalidCreateFromBuffer ===================================================" << std::endl;
            testInvalidCreateFromBuffer();
            std::cout << "=== test CreateAndOpenDatabase   ===================================================" << std::endl;
            testCreateAndOpenDatabase();
            std::cout << "=== test InsertRecord            ===================================================" << std::endl;
            testInsertRecord();
            std::cout << "=== test DeleteRecord            ===================================================" << std::endl;
            testDeleteRecord();
            std::cout << "=== test OptimizeDatabase        ===================================================" << std::endl;
            testOptimizeDatabase();
            std::cout << "=== test GetRecordContent        ===================================================" << std::endl;
            testGetRecordContent();
            std::cout << "All CPP tests passed!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Some CPP Tests failed: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

    // Export a function that Python can call
    int main_tests(int argc, const char* argv[]) {
        return run_main_tests(argc, argv);
    }

    // Export a function that Python can call
    int main(int argc, const char* argv[]) {
        return run_main_tests(argc, argv);
    }
}
