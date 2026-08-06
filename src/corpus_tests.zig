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
    "control_flow",
    "heap_limits",
    "intrinsics",
    "malloc",
    "pointer_array",
    "printf",
    "qsort",
    "string",
    "struct_copy",
    "struct_call",
    "struct_layout",
    "switch_chain",
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

        std.testing.expectEqualStrings(expected, actual) catch |err| {
            std.debug.print("VIG C corpus case that failed: {s}.c\n", .{name});
            return err;
        };
    }
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
