//! The VIG C compiler and static-link driver.

const std = @import("std");
const tools = @import("tool_options");

pub fn main(init: std.process.Init) !void {
    const allocator = init.arena.allocator();
    const args = try init.minimal.args.toSlice(allocator);

    if (args.len > 1 and std.mem.eql(u8, args[1], "-c")) {
        if (args.len != 5 or !std.mem.eql(u8, args[3], "-o")) {
            usage();
            return error.InvalidArguments;
        }
        try compileToObject(init, args[2], args[4]);
        return;
    }

    if (args.len < 4 or !std.mem.eql(u8, args[args.len - 2], "-o")) {
        usage();
        return error.InvalidArguments;
    }

    const inputs = args[1 .. args.len - 2];
    const output = args[args.len - 1];
    if (inputs.len == 1 and !std.mem.endsWith(u8, inputs[0], ".vigo")) {
        const temporary_object = try std.fmt.allocPrint(
            allocator,
            "{s}.vigcc.vigo",
            .{output},
        );
        try reserveTemporary(init.io, temporary_object);
        defer std.Io.Dir.cwd().deleteFile(init.io, temporary_object) catch {};
        try compileToObject(init, inputs[0], temporary_object);
        try linkObjects(init, &.{temporary_object}, output);
        return;
    }

    for (inputs) |input| {
        if (!std.mem.endsWith(u8, input, ".vigo")) {
            usage();
            return error.InvalidArguments;
        }
    }
    try linkObjects(init, inputs, output);
}

fn usage() void {
    std.debug.print(
        "Usage: vigcc [-c] <source.c|input.vigo>... -o <output.vigo|output.vig>\n",
        .{},
    );
}

fn compileToObject(init: std.process.Init, source: []const u8, output: []const u8) !void {
    const allocator = init.arena.allocator();
    const preprocessed = try std.fmt.allocPrint(allocator, "{s}.vigcc.i", .{output});
    const assembly = try std.fmt.allocPrint(allocator, "{s}.vigcc.vigas", .{output});

    try reserveTemporary(init.io, preprocessed);
    defer std.Io.Dir.cwd().deleteFile(init.io, preprocessed) catch {};
    try reserveTemporary(init.io, assembly);
    defer std.Io.Dir.cwd().deleteFile(init.io, assembly) catch {};

    const host = switch (@import("builtin").os.tag) {
        .windows => "-DVIG_HOST_WINDOWS=1",
        .macos => "-DVIG_HOST_MACOS=1",
        else => "-DVIG_HOST_POSIX=1",
    };
    try run(init, &.{ tools.cpp_path, host, "-I", tools.vig_include_path, source, preprocessed });
    try run(init, &.{ tools.rcc_path, "-target=vig", preprocessed, assembly });
    try run(init, &.{ tools.vigasm_path, "-c", assembly, "-o", output });
}

fn linkObjects(
    init: std.process.Init,
    inputs: []const []const u8,
    output: []const u8,
) !void {
    const allocator = init.arena.allocator();
    var command = std.ArrayList([]const u8).empty;
    try command.append(allocator, tools.vigld_path);
    try command.append(allocator, tools.crt0_path);
    try command.appendSlice(allocator, inputs);
    try command.appendSlice(allocator, &.{
        tools.runtime_ctype_path,
        tools.runtime_errno_path,
        tools.runtime_stdio_path,
        tools.runtime_stdlib_path,
        tools.runtime_string_path,
        tools.runtime_time_path,
    });
    try command.append(allocator, "-o");
    try command.append(allocator, output);
    try run(init, command.items);
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
