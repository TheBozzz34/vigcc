//! End-to-end regression corpus for VIG C programs.
//!
//! Each source has a same-named `.expected` file.  The source goes through the
//! public `vigcc` command, whose assembler invocation enables stack checking;
//! the resulting program then runs in the VM.
//!
//! Every case prints the values it computes, with the `__vig_print` family from
//! <vig.h>, and the `.expected` file holds what those values are.  A case that
//! only ran to `halt` would say nothing about what it computed: a code generator
//! can turn an `add` into a `sub` and still produce a program that verifies and
//! stops.  The recorded output is therefore the contract, and each number in it
//! was worked out by hand before it was recorded.

const std = @import("std");
const options = @import("corpus_options");

const cases = [_][]const u8{
    "add_one",
    "assert_fail",
    "assert_pass",
    "control_flow",
    "ctype_errno",
    "ffi_clock",
    "floating",
    "ffi",
    "heap_limits",
    "intrinsics",
    "limits_bool",
    "malloc",
    "math",
    "pointer_array",
    "printf",
    "printf_float",
    "qsort",
    "string",
    "struct_copy",
    "struct_call",
    "struct_layout",
    "switch_chain",
    "switch_table",
    "time",
    "unsigned_math",
    "varargs",
};

test "each VIG C program produces its recorded output" {
    const allocator = std.testing.allocator;

    for (cases) |name| {
        const source = try corpusPath(allocator, name, ".c");
        defer allocator.free(source);
        const expected_path = try corpusPath(allocator, name, ".expected");
        defer allocator.free(expected_path);
        const expected = try std.Io.Dir.cwd().readFileAlloc(
            std.testing.io,
            expected_path,
            allocator,
            .limited(1 << 20),
        );
        defer allocator.free(expected);

        var tmp = std.testing.tmpDir(.{});
        defer tmp.cleanup();
        const file = try tmp.dir.createFile(std.testing.io, "program.vig", .{});
        file.close(std.testing.io);
        const output = try tmp.dir.realPathFileAlloc(std.testing.io, "program.vig", allocator);
        defer allocator.free(output);

        const compiler_output = try run(&.{ options.vigcc_path, source, "-o", output });
        defer allocator.free(compiler_output);
        const actual = try run(&.{ options.vig_path, output });
        defer allocator.free(actual);

        expectSameOutput(expected, actual) catch |err| {
            std.debug.print("VIG C corpus case that failed: {s}.c\n", .{name});
            return err;
        };
    }
}

test "separate C objects link through crt0 and run" {
    const allocator = std.testing.allocator;
    const math_source = try std.fs.path.join(
        allocator,
        &.{ options.corpus_dir, "link", "math.c" },
    );
    defer allocator.free(math_source);
    const main_source = try std.fs.path.join(
        allocator,
        &.{ options.corpus_dir, "link", "main.c" },
    );
    defer allocator.free(main_source);
    const expected_path = try std.fs.path.join(
        allocator,
        &.{ options.corpus_dir, "link", "program.expected" },
    );
    defer allocator.free(expected_path);
    const expected_output = try std.Io.Dir.cwd().readFileAlloc(
        std.testing.io,
        expected_path,
        allocator,
        .limited(1 << 20),
    );
    defer allocator.free(expected_output);

    var tmp = std.testing.tmpDir(.{});
    defer tmp.cleanup();
    const math_object = try tempPath(&tmp, allocator, "math.vigo");
    defer allocator.free(math_object);
    const main_object = try tempPath(&tmp, allocator, "main.vigo");
    defer allocator.free(main_object);
    const program = try tempPath(&tmp, allocator, "program.vig");
    defer allocator.free(program);

    allocator.free(try run(&.{ options.vigcc_path, "-c", math_source, "-o", math_object }));
    allocator.free(try run(&.{ options.vigcc_path, "-c", main_source, "-o", main_object }));
    allocator.free(try run(&.{ options.vigcc_path, math_object, main_object, "-o", program }));
    const actual = try run(&.{ options.vig_path, program });
    defer allocator.free(actual);
    try expectSameOutput(expected_output, actual);
}

fn tempPath(
    tmp: *std.testing.TmpDir,
    allocator: std.mem.Allocator,
    name: []const u8,
) ![:0]u8 {
    const file = try tmp.dir.createFile(std.testing.io, name, .{});
    file.close(std.testing.io);
    return tmp.dir.realPathFileAlloc(std.testing.io, name, allocator);
}
fn corpusPath(allocator: std.mem.Allocator, name: []const u8, extension: []const u8) ![]u8 {
    const file_name = try std.fmt.allocPrint(allocator, "{s}{s}", .{ name, extension });
    defer allocator.free(file_name);
    return std.fs.path.join(allocator, &.{ options.corpus_dir, file_name });
}

test "every VIG C corpus source is covered" {
    var directory = try std.Io.Dir.cwd().openDir(std.testing.io, options.corpus_dir, .{ .iterate = true });
    defer directory.close(std.testing.io);

    var missing: usize = 0;
    var iterator = directory.iterate();
    while (try iterator.next(std.testing.io)) |entry| {
        if (entry.kind != .file or !std.mem.endsWith(u8, entry.name, ".c")) continue;
        const name = entry.name[0 .. entry.name.len - ".c".len];
        for (cases) |case| {
            if (std.mem.eql(u8, name, case)) break;
        } else {
            std.debug.print("VIG C corpus source covered by no test: {s}\n", .{entry.name});
            missing += 1;
        }
    }
    try std.testing.expectEqual(@as(usize, 0), missing);
}

fn expectSameOutput(expected: []const u8, actual: []const u8) !void {
    const allocator = std.testing.allocator;
    const normalized_expected = try withoutCarriageReturns(allocator, expected);
    defer allocator.free(normalized_expected);
    const normalized_actual = try withoutCarriageReturns(allocator, actual);
    defer allocator.free(normalized_actual);
    try std.testing.expectEqualStrings(normalized_expected, normalized_actual);
}

fn withoutCarriageReturns(
    allocator: std.mem.Allocator,
    text: []const u8,
) ![]u8 {
    var normalized = std.ArrayList(u8).empty;
    errdefer normalized.deinit(allocator);
    for (text) |byte| {
        if (byte != '\r') try normalized.append(allocator, byte);
    }
    return normalized.toOwnedSlice(allocator);
}

fn run(argv: []const []const u8) ![]u8 {
    const result = try std.process.run(std.testing.allocator, std.testing.io, .{ .argv = argv });
    errdefer std.testing.allocator.free(result.stdout);
    defer std.testing.allocator.free(result.stderr);

    if (!result.term.success()) {
        if (result.stderr.len != 0) std.debug.print("{s}", .{result.stderr});
        return error.ChildProcessFailed;
    }
    return result.stdout;
}
