#include <gtest/gtest.h>
#include "filedbpp.cpp" // Include the Database class header file
#include <cstdio>       // For file management (e.g., remove)

// Test fixture for Record
class RecordTest : public ::testing::Test {
protected:
    // Utility function to generate sample data
    static std::string generateSampleData(const std::string& content) {
        return content;
    }
};

// Test: Creating a record and generating an ID
TEST_F(RecordTest, CreateFromBuffer_GeneratesCorrectId) {
    const std::string data = generateSampleData("Sample Record Content");
    int start = 0;

    // Create the record
    Record* record = Record::createFromBuffer(start, data.c_str(), data.size());
    ASSERT_NE(record, nullptr); // Ensure record is created

    // Verify ID generation (hash-based deterministic UUID)
    Record* recordDuplicate = Record::createFromBuffer(start, data.c_str(), data.size());
    ASSERT_EQ(std::string(record->id, sizeof(record->id)),
              std::string(recordDuplicate->id, sizeof(recordDuplicate->id)));

    delete record;
    delete recordDuplicate;
}

// Test: Buffer allocation
TEST_F(RecordTest, AllocateContentBuffer_ReturnsValidBuffer) {
    const std::string data = generateSampleData("Another Record");
    int start = 10;

    // Create the record
    Record* record = Record::createFromBuffer(start, data.c_str(), data.size());
    ASSERT_NE(record, nullptr);

    // Allocate content buffer
    char* buffer = record->allocateContentBuffer();
    ASSERT_NE(buffer, nullptr);

    // Ensure buffer size matches expected length
    ASSERT_EQ(static_cast<size_t>(record->end - record->start), data.size());

    delete[] buffer;
    delete record;
}

// Test: Copying a record
TEST_F(RecordTest, Copy_CreatesIdenticalRecord) {
    const std::string data = generateSampleData("Copy Test Record");
    int start = 0;

    // Create the record
    Record* originalRecord = Record::createFromBuffer(start, data.c_str(), data.size());
    ASSERT_NE(originalRecord, nullptr);

    // Copy the record
    Record* copiedRecord = originalRecord->copy();
    ASSERT_NE(copiedRecord, nullptr);

    // Verify that the ID, start, and end are identical
    ASSERT_EQ(std::string(originalRecord->id, sizeof(originalRecord->id)),
              std::string(copiedRecord->id, sizeof(copiedRecord->id)));
    ASSERT_EQ(originalRecord->start, copiedRecord->start);
    ASSERT_EQ(originalRecord->end, copiedRecord->end);

    delete originalRecord;
    delete copiedRecord;
}

// Test: Checking if a record is deleted
TEST_F(RecordTest, IsDeleted_ReturnsCorrectStatus) {
    const std::string data = generateSampleData("Active Record");
    int start = 0;

    // Create an active record
    Record* activeRecord = Record::createFromBuffer(start, data.c_str(), data.size());
    ASSERT_NE(activeRecord, nullptr);
    ASSERT_FALSE(activeRecord->isDeleted());

    // Create a deleted record
    Record deletedRecord = *activeRecord;
    deletedRecord.start = deletedRecord.end;

    ASSERT_TRUE(deletedRecord.isDeleted());

    delete activeRecord;
}

// Test: Handling invalid inputs
TEST_F(RecordTest, CreateFromBuffer_ThrowsOnInvalidInput) {
    // Test with null data
    EXPECT_THROW(Record::createFromBuffer(0, nullptr, 10), std::invalid_argument);

    // Test with zero-length data
    EXPECT_THROW(Record::createFromBuffer(0, "Data", 0), std::invalid_argument);
}

class DatabaseTest : public ::testing::Test {
protected:
    std::string testDatabasePath = "test_db";
    Database* database = nullptr;

    void SetUp() override {
        // Create a test database instance
        database = Database::create(testDatabasePath);
    }

    void TearDown() override {
        if (database) {
            database->close();
            database->free();
            delete database;
        }

        // Clean up test files
        remove((testDatabasePath + ".data").c_str());
        remove((testDatabasePath + ".index").c_str());
    }

    // Helper to insert a test record
    Record* insertTestRecord(const std::string& content) {
        return database->insertRecord(content.c_str(), content.size());
    }
};

// Test: Create and open a database
TEST_F(DatabaseTest, CreateAndOpenDatabase) {
    ASSERT_NE(database, nullptr);

    // Open the database
    ASSERT_EQ(database->open(), 0);

    // Verify file paths
    ASSERT_TRUE(std::ifstream(testDatabasePath + ".data").good());
    ASSERT_TRUE(std::ifstream(testDatabasePath + ".index").good());
}

// Test: Insert a record
TEST_F(DatabaseTest, InsertRecord) {
    ASSERT_EQ(database->open(), 0);

    // Insert a record
    const std::string content = "Sample Record";
    Record* record = insertTestRecord(content);
    ASSERT_NE(record, nullptr);

    // Verify record properties
    ASSERT_EQ(record->start, 0);
    ASSERT_EQ(record->end, content.size());
    ASSERT_EQ(database->recordList.size(), 1);
}

// Test: Get record content
TEST_F(DatabaseTest, GetRecordContent) {
    ASSERT_EQ(database->open(), 0);

    const std::string content = "Record Content Test";
    Record* record = insertTestRecord(content);

    char* buffer = new char[content.size()];
    ASSERT_EQ(database->getRecordContent(record, buffer), 0);
    ASSERT_EQ(std::string(buffer, content.size()), content);

    delete[] buffer;
}

// Test: Delete a record
TEST_F(DatabaseTest, DeleteRecord) {
    ASSERT_EQ(database->open(), 0);

    const std::string content = "Record to Delete";
    Record* record = insertTestRecord(content);

    // Delete the record
    Record* deletedRecord = database->deleteRecord(record);
    ASSERT_NE(deletedRecord, nullptr);

    // Verify deletion status
    ASSERT_TRUE(deletedRecord->isDeleted());
    ASSERT_EQ(database->recordList.size(), 2);
}

// Test: Optimize the database
TEST_F(DatabaseTest, OptimizeDatabase) {
    ASSERT_EQ(database->open(), 0);

    // Insert multiple versions of a record
    const std::string content1 = "Record Version 1";
    const std::string content2 = "Record Version 2";
    Record* record1 = insertTestRecord(content1);
    Record* record2 = insertTestRecord(content2);

    // Optimize the database
    ASSERT_EQ(database->optimize(), 0);

    // Verify that only the latest version remains
    ASSERT_EQ(database->recordList.size(), 1);
    ASSERT_EQ(std::string(database->recordList[0]->id, sizeof(record2->id)),
              std::string(record2->id, sizeof(record2->id)));
}

// // Test: List all records
// TEST_F(DatabaseTest, ListAllRecords) {
//     ASSERT_EQ(database->open(), 0);
// 
//     const std::string content1 = "Record One";
//     const std::string content2 = "Record Two";
//     insertTestRecord(content1);
//     insertTestRecord(content2);
// 
//     database->listAll([](Record* record, int ord) {
//         ASSERT_NE(record, nullptr);
//     });
// }

// Test: Handle missing database files
TEST_F(DatabaseTest, HandleMissingFiles) {
    // Attempt to open a database when files are missing
    ASSERT_NO_THROW(database->open());

    // Ensure the files are created
    ASSERT_TRUE(std::ifstream(testDatabasePath + ".data").good());
    ASSERT_TRUE(std::ifstream(testDatabasePath + ".index").good());
}

// Test: Insert invalid record
TEST_F(DatabaseTest, InsertInvalidRecord) {
    ASSERT_EQ(database->open(), 0);

    // Attempt to insert invalid record data
    EXPECT_THROW(database->insertRecord(nullptr, 10), std::invalid_argument);
    EXPECT_THROW(database->insertRecord("Valid Data", 0), std::invalid_argument);
}

// Test: Close database
TEST_F(DatabaseTest, CloseDatabase) {
    ASSERT_EQ(database->open(), 0);
    ASSERT_EQ(database->close(), 0);

    // Verify that files are properly closed
    ASSERT_EQ(database->dataFileReference, nullptr);
    ASSERT_EQ(database->indexFileReference, nullptr);
}

// Test: Free database resources
TEST_F(DatabaseTest, FreeDatabase) {
    ASSERT_EQ(database->open(), 0);

    const std::string content = "Sample Record";
    insertTestRecord(content);

    ASSERT_EQ(database->free(), 0);

    // Verify that the record list is cleared
    ASSERT_TRUE(database->recordList.empty());
}

