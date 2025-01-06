#include <fstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <cstring>

class Record {
public:
    char id[32];
    std::size_t start = 0;
    std::size_t end = 0;

    /**
     * Creates a new record by calculating the UUID of the record
     * based on some hashing algorithm of the given content.
     */
    static Record* createFromBuffer(int start, const char* data, int data_length) {
        if (!data || data_length <= 0) {
            throw std::invalid_argument("Data must not be null and data_length must be positive.");
        }

        auto* record = new Record();
        record->start = static_cast<std::size_t>(start);
        record->end = record->start + static_cast<std::size_t>(data_length);

        // Generate the unique ID
        record->generateId(data, data_length);

        return record;
    }

    /**
     * Allocates a new buffer for the record content.
     */
    char* allocateContentBuffer() const {
        std::size_t buffer_size = end - start;
        if (buffer_size == 0) {
            return nullptr;
        }

        char* buffer = new char[buffer_size];
        std::memset(buffer, 0, buffer_size); // Initialize with zeros
        return buffer;
    }

    /**
     * Creates a copy of the current record.
     */
    Record* copy() const {
        auto* copy_record = new Record();
        std::memcpy(copy_record->id, this->id, sizeof(this->id));
        copy_record->start = this->start;
        copy_record->end = this->end;
        return copy_record;
    }

    /**
     * Checks if the record is deleted.
     * A deleted record is one with `start == end`.
     */
    bool isDeleted() const {
        return start == end;
    }

private:
    /**
     * Generates a deterministic 32-byte unique ID based on the record content.
     */
    void generateId(const char* data, int data_length) {
        char* hash = (char*)calloc(32,sizeof(char));
        
        size_t sum = 0;
        for (int i = 0; i < data_length; i++) {
            sum += (unsigned char)data[i] * (i + 1);
        }
        snprintf(hash, 32, "%032zx", sum);

        //// // Use SHA-256 to generate the hash
        //// SHA256(reinterpret_cast<const unsigned char*>(data), data_length, hash);
        //// 
        //// // Copy the first 32 bytes of the hash to the ID
        //// static_assert(sizeof(id) <= SHA256_DIGEST_LENGTH, "ID size exceeds hash output size.");
        std::memcpy(id, hash, sizeof(id));
        free(hash);
    }
};

class Database {
public:
    std::string path;
    std::fstream* dataFileReference = nullptr;
    std::fstream* indexFileReference = nullptr;
    std::vector<Record*> recordList;

    static constexpr const char* DATA_FILE_EXT = ".data";
    static constexpr const char* INDEX_FILE_EXT = ".index";

    /**
     * Creates a new instance of a database.
     */
    static Database* create(const std::string& path) {
        if (path.empty()) {
            throw std::invalid_argument("Database path must not be empty.");
        }

        auto* db = new Database();
        db->path = path;
        return db;
    }

    /**
     * Initializes a database connection.
     * If any of the index file or data file do not exist, it will create them.
     * If both exist, it will read the binary index file into the new database record list.
     */
    int open() {
        // Open or create data and index files
        std::string dataFilePath = path + DATA_FILE_EXT;
        std::string indexFilePath = path + INDEX_FILE_EXT;

        dataFileReference = openFile(dataFilePath, std::ios::in | std::ios::out | std::ios::binary);
        indexFileReference = openFile(indexFilePath, std::ios::in | std::ios::out | std::ios::binary);

        // If index file exists, load records
        if (fileExists(indexFilePath)) {
            loadIndexFile();
        }

        return 0;
    }

    /**
     * Closes the files of the database.
     */
    int close() {
        if (dataFileReference != nullptr) {
            closeFile(dataFileReference);
            dataFileReference = nullptr;
        }
        if (indexFileReference != nullptr) {
            closeFile(indexFileReference);
            indexFileReference = nullptr;
        }
        return 0;
    }

    /**
     * Frees up the memory of the database.
     */
    int free() {
        for (auto* record : recordList) {
            delete record;
        }
        recordList.clear();
        return 0;
    }

    /**
     * Creates a new record and inserts it into the database.
     */
    Record* insertRecord(const char* data, int data_length) {
        if (!data || data_length <= 0) {
            throw std::invalid_argument("Invalid record data.");
        }

        // Determine the write position in the data file
        std::streampos dataFilePos = getFileSize(dataFileReference);
        writeFile(dataFileReference, data, data_length);

        // Create a new record
        auto* record = Record::createFromBuffer(static_cast<int>(dataFilePos), data, data_length);

        // Append to index and record list
        appendToIndexFile(record);
        recordList.push_back(record);

        return record;
    }

    /**
     * Deletes a record by inserting an empty record with the same ID.
     */
    Record* deleteRecord(Record* record) {
        if (!record) {
            throw std::invalid_argument("Record cannot be null.");
        }

        // Create a "deleted" version of the record
        auto* deletedRecord = new Record(*record);
        deletedRecord->start = deletedRecord->end = getFileSize(dataFileReference);

        appendToIndexFile(deletedRecord);
        recordList.push_back(deletedRecord);

        return deletedRecord;
    }

    /**
     * Reads the content of a record from the database file.
     */
    int getRecordContent(Record* record, char* content) {
        if (!record || !content) {
            throw std::invalid_argument("Invalid record or content buffer.");
        }

        std::streamsize size = record->end - record->start;
        if (size <= 0) {
            return -1;
        }

        readFile(dataFileReference, record->start, content, size);
        return 0;
    }

    /**
     * Lists all records, invoking the provided callback for each one.
     */
    int listAll(const std::function<int(Record*, int)>& onRecordFound) {
        int order = 0;
        for (const auto* record : recordList) {
            if (onRecordFound(const_cast<Record*>(record), order++) != 0) {
                break;
            }
        }
        return 0;
    }

    /**
     * Optimizes the database by removing duplicate and deleted records.
     */
    int optimize() {
        // Simple implementation: filter latest, non-deleted records
        std::vector<Record*> optimizedRecords;

        std::unordered_map<std::string, Record*> latestRecords;
        for (auto* record : recordList) {
            if (!record->isDeleted()) {
                latestRecords[std::string(record->id, sizeof(record->id))] = record;
            }
        }

        for (const auto& pair : latestRecords) {
            optimizedRecords.push_back(pair.second);
        }

        recordList = std::move(optimizedRecords);
        writeIndexFile();
        return 0;
    }

private:
    /**
     * Checks if a file exists.
     */
    bool fileExists(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        return file.good();
    }

    /**
     * Opens a file with the given mode.
     */
    std::fstream* openFile(const std::string& filePath, std::ios::openmode mode) {
        auto* file = new std::fstream(filePath, mode);
        if (!file->is_open()) {
            delete file;
            throw std::runtime_error("Unable to open file: " + filePath);
        }
        return file;
    }

    /**
     * Closes a file.
     */
    void closeFile(std::fstream* file) {
        if (file) {
            file->close();
            delete file;
        }
    }

    /**
     * Gets the size of the file.
     */
    std::streampos getFileSize(std::fstream* file) {
        if (!file || !file->is_open()) {
            throw std::runtime_error("File is not open.");
        }

        auto currentPos = file->tellg();    // Save the current position
        file->seekg(0, std::ios::end);      // Move to the end of the file
        auto size = file->tellg();          // Get the size
        file->seekg(currentPos);            // Restore the position
        return size;
    }

    /**
     * Reads data from a file at a specific position.
     */
    void readFile(std::fstream* file, std::streampos pos, char* buffer, std::streamsize size) {
        if (!file || !file->is_open()) {
            throw std::runtime_error("File is not open.");
        }

        file->seekg(pos);
        file->read(buffer, size);
        if (!file->good()) {
            throw std::runtime_error("Failed to read from file.");
        }
    }

    /**
     * Writes data to a file.
     */
    void writeFile(std::fstream* file, const char* data, std::streamsize size) {
        if (!file || !file->is_open()) {
            throw std::runtime_error("File is not open.");
        }

        file->write(data, size);
        if (!file->good()) {
            throw std::runtime_error("Failed to write to file.");
        }
    }

    /**
     * Loads the index file into the record list.
     */
    void loadIndexFile() {
        std::ifstream indexFile(path + INDEX_FILE_EXT, std::ios::binary);
        while (indexFile.peek() != EOF) {
            auto* record = new Record();
            indexFile.read(reinterpret_cast<char*>(record), sizeof(Record));
            recordList.push_back(record);
        }
    }

    /**
     * Appends a record to the index file.
     */
    void appendToIndexFile(const Record* record) {
        std::ofstream indexFile(path + INDEX_FILE_EXT, std::ios::binary | std::ios::app);
        indexFile.write(reinterpret_cast<const char*>(record), sizeof(Record));
    }

    /**
     * Writes the current record list to the index file.
     */
    void writeIndexFile() {
        std::ofstream indexFile(path + INDEX_FILE_EXT, std::ios::binary | std::ios::trunc);
        for (const auto* record : recordList) {
            indexFile.write(reinterpret_cast<const char*>(record), sizeof(Record));
        }
    }
};
