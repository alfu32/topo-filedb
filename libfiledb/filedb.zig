const std = @import("std");

// Define an explicit error set for database operations
pub const DatabaseError = error{
    FileOpenFailed,
    RecordAllocationFailed,
    EmptyRecord,
    Other,
};

pub const Record = struct {
    id: [32]u8, // 256-bit UUID
    start: usize, // Start position in the data file
    end: usize, // End position in the data file

    /// Checks if the record is marked as deleted
    pub fn isDeleted(self: *Record) bool {
        return self.start == 0 and self.end == 0;
    }

    /// Allocates a buffer for the record's content
    pub fn allocateContentBuffer(self: *Record) ![]u8 {
        const size = self.end - self.start;
        if (size == 0) {
            return error.EmptyRecord;
        }
        return try std.heap.page_allocator.alloc(u8, size + 1); // +1 for null-termination
    }
};

pub const Database = struct {
    path: []const u8, // Database path
    recordList: std.ArrayList(Record), // List of records
    dataFile: ?std.fs.File, // Handle for the data file
    indexFile: ?std.fs.File, // Handle for the index file

    /// Creates a new database instance
    pub fn init(allocator: *std.mem.Allocator, path: []const u8) !*Database {
        const db = try allocator.create(Database);
        db.path = path;
        db.recordList = try std.ArrayList(Record).init(allocator);
        db.dataFile = null;
        db.indexFile = null;
        return db;
    }

    /// Opens the database files
    pub fn open(self: *Database, allocator: *std.mem.Allocator) !void {
        const fs = std.fs.cwd();
        self.dataFile = try fs.openFile(self.getFilePath("data"), .{ .read = true, .write = true, .create = true });
        self.indexFile = try fs.openFile(self.getFilePath("index"), .{ .read = true, .write = true, .create = true });

        // Load records from the index file
        var indexFileHandle = self.indexFile.?;
        const records = try std.io.Reader(indexFileHandle.reader()).readAllAlloc(allocator, Record);
        for (records) |record| {
            try self.recordList.append(record);
        }
    }

    /// Closes the database files
    pub fn close(self: *Database) !void {
        if (self.dataFile) |dataFile| {
            try dataFile.close();
        }
        if (self.indexFile) |indexFile| {
            try indexFile.close();
        }
    }

    /// Frees the database and its resources
    pub fn deinit(self: *Database, allocator: *std.mem.Allocator) void {
        self.recordList.deinit();
        allocator.destroy(self);
    }

    /// Inserts a record into the database
    pub fn insertRecord(self: *Database, content: []const u8) !Record {
        var dataFileHandle = self.dataFile.?;
        const start = try dataFileHandle.seekToEnd();
        try dataFileHandle.write(content);

        const end = try dataFileHandle.tell();
        const newRecord = Record{ .id = generateId(content), .start = start, .end = end };
        try self.recordList.append(newRecord);

        var indexFileHandle = self.indexFile.?;
        try indexFileHandle.writeStruct(newRecord);

        return newRecord;
    }

    /// Deletes a record in the database
    pub fn deleteRecord(self: *Database, record: Record) !void {
        var deletedRecord = record;
        deletedRecord.start = 0;
        deletedRecord.end = 0;

        try self.recordList.append(deletedRecord);

        var indexFileHandle = self.indexFile.?;
        try indexFileHandle.writeStruct(deletedRecord);
    }

    /// Aggregates all records
    pub fn aggregateAll(self: *Database, aggregator: fn (*Database, Record, usize) void!DatabaseError) void!DatabaseError {
        var idx = 0;
        for (self.recordList.items) |record| {
            try aggregator(self, record, idx);
            idx += 1;
        }
    }

    /// Aggregates the latest non-deleted records
    pub fn aggregateLatestRecords(self: *Database, aggregator: fn (*Database, Record, usize) void!DatabaseError) void!DatabaseError {
        var processedIds: std.HashSet([32]u8) = std.HashSet.init(std.heap.page_allocator);
        var idx = 0;

        for (self.recordList.items.reverse()) |record| {
            if (processedIds.contains(record.id)) continue;

            if (!record.isDeleted()) {
                try aggregator(self, record, idx);
            }
            try processedIds.put(record.id);
            idx += 1;
        }

        return .{};
    }

    /// Constructs a file path for the database files
    fn getFilePath(self: *Database, extension: []const u8) []u8 {
        return std.fmt.allocPrint("{s}.{s}", .{ self.path, extension });
    }

    /// Generates a unique ID (placeholder)
    fn generateId(content: []const u8) [32]u8 {
        // Simple hash for demonstration; replace with a real hash function
        var hash: [32]u8 = undefined;
        var idx = 0;
        for (content) |byte| {
            hash[idx % 32] ^= byte;
            idx += 1;
        }
        return hash;
    }
};
