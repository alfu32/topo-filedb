const std = @import("std");
const Database = @import("filedb.zig").Database;
const Record = @import("filedb.zig").Record;
const DatabaseError = @import("filedb.zig").DatabaseError;

test "Database Initialization" {
    var allocator = std.testing.allocator;

    // Create a new database instance
    var db = try Database.init(&allocator, "test_db");
    defer db.deinit(&allocator);

    // Ensure the database is initialized correctly
    try std.testing.expect(std.mem.eql(u8, db.path, "test_db"));
    try std.testing.expect(db.recordList.len == 0);
}

test "Database Open and Close" {
    var allocator = std.testing.allocator;

    // Initialize and open the database
    var db = try Database.init(&allocator, "test_db");
    defer db.deinit(&allocator);
    try db.open(&allocator);

    // Ensure files are opened correctly
    try std.testing.expect(db.dataFile != null);
    try std.testing.expect(db.indexFile != null);

    // Close the database
    try db.close();
    try std.testing.expect(db.dataFile == null);
    try std.testing.expect(db.indexFile == null);
}

fn validateInsertedRecord(
    db: *Database,
    record: Record,
    idx: usize,
    content: []const u8,
) void!DatabaseError {
    const buffer = try record.allocateContentBuffer();
    defer std.heap.page_allocator.free(buffer);
    _ = db;
    _ = idx;

    // Compare the buffer content with the original content
    try std.testing.expectEqualStrings(content, buffer[0 .. buffer.len - 1]);
    return .{};
}

test "Insert and Retrieve Record" {
    var allocator = std.testing.allocator;

    // Initialize and open the database
    var db = try Database.init(&allocator, "test_db");
    defer db.deinit(&allocator);
    try db.open(&allocator);

    // Insert a record
    const content = "Test Record";
    const record = try db.insertRecord(content);

    // Ensure the record was inserted correctly
    try std.testing.expect(record.start != 0);
    try std.testing.expect(record.end > record.start);
    try std.testing.expect(record.id.len == 32);

    // Aggregate all records and validate
    try db.aggregateAll(
        struct { content: []const u8 }{ .content = content },
        validateInsertedRecord,
    );

    try db.close();
}

test "Delete Record" {
    var allocator = std.testing.allocator;

    // Initialize and open the database
    var db = try Database.init(&allocator, "test_db");
    defer db.deinit(&allocator);
    try db.open(&allocator);

    // Insert and delete a record
    const content = "Record to Delete";
    const record = try db.insertRecord(content);
    try db.deleteRecord(record);

    // Ensure the record is marked as deleted
    try db.aggregateAll(fn (db: *Database, record: Record, idx: usize) void!DatabaseError{if (record.id == record.id) {
        try std.testing.expect(record.isDeleted());
    }});

    try db.close();
}
fn validateLatestRecord(
    db: *Database,
    record: Record,
    idx: usize,
    record2: *Record,
) !DatabaseError {
    _ = db;
    if (std.mem.eql(u8, record.id, record2.id)) {
        std.debug.print("Latest Record {d}: ID matches\n", .{idx});
    }
    return .{};
}

test "Aggregate Latest Records" {
    var allocator = std.testing.allocator;

    var db = try Database.init(&allocator, "test_db");
    defer db.deinit(&allocator);
    try db.open(&allocator);

    // Insert multiple versions of a record
    const content1 = "First Version";
    const content2 = "Second Version";
    const record1 = try db.insertRecord(content1);
    const record2 = try db.insertRecord(content2);
    _ = record1;
    _ = record2;
    // Aggregate latest records
    try db.aggregateLatestRecords(validateLatestRecord);

    try db.close();
}
