//! The VIG C compiler driver.
//!
//! lcc splits preprocessing from parsing, and the VIG assembler deliberately
//! stays a separate program.  This small driver puts the three stages back into
//! one command while keeping their intermediate VIGasm private to the build.

const std = @import("std");
const tools = @import("tool_options");

pub fn main(init: std.process.Init) !void {
    const allocator = init.arena.allocator();
    const args = try init.minimal.args.toSlice(allocator);
    if (args.len != 4 or !std.mem.eql(u8, args[2], "-o")) {
        std.debug.print("Usage: vigcc <source.c> -o <output.vig>\n", .{});
        return error.InvalidArguments;
    }

    const source = args[1];
    const output = args[3];
    const preprocessed = try std.fmt.allocPrint(allocator, "{s}.vigcc.i", .{output});
    const assembly = try std.fmt.allocPrint(allocator, "{s}.vigcc.vigas", .{output});

    try reserveTemporary(init.io, preprocessed);
    defer std.Io.Dir.cwd().deleteFile(init.io, preprocessed) catch {};
    try reserveTemporary(init.io, assembly);
    defer std.Io.Dir.cwd().deleteFile(init.io, assembly) catch {};

    try run(init, &.{ tools.cpp_path, "-I", tools.vig_include_path, source, preprocessed });
    try run(init, &.{ tools.rcc_path, "-target=vig", preprocessed, assembly });
    // Keep the stack checker on for every compiled program.  It makes a code
    // generator mistake fail at the instruction that caused it.
    try run(init, &.{ tools.vigasm_path, assembly, "-o", output, "--check-stack" });
}

fn reserveTemporary(io: std.Io, path: []const u8) !void {
    const file = try std.Io.Dir.cwd().createFile(io, path, .{ .exclusive = true });
    file.close(io);
}

fn run(init: std.process.Init, argv: []const []const u8) !void {
    const result = try std.process.run(init.gpa, init.io, .{ .argv = argv });
    defer init.gpa.free(result.stdout);
    defer init.gpa.free(result.stderr);

    if (!result.term.success()) {
        if (result.stdout.len != 0) std.debug.print("{s}", .{result.stdout});
        if (result.stderr.len != 0) std.debug.print("{s}", .{result.stderr});
        return error.CompilerStageFailed;
    }
}
