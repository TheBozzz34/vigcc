const std = @import("std");

/// Build the self-hosting pieces of lcc that the VIG toolchain needs.  `lburg`
/// generates the instruction-selector tables before `rcc` is compiled, so the
/// generated C sources stay in the Zig build cache rather than the checkout.
pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const c_flags = &.{"-std=gnu89"};

    const lburg = b.addExecutable(.{
        .name = "lburg",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    lburg.root_module.addCSourceFiles(.{
        .root = b.path("lburg"),
        .files = &.{ "lburg.c", "gram.c" },
        .flags = c_flags,
    });
    lburg.root_module.addIncludePath(b.path("lburg"));

    const dagcheck = generateMatcher(b, lburg, "src/dagcheck.md", "dagcheck.c");
    const alpha = generateMatcher(b, lburg, "src/alpha.md", "alpha.c");
    const mips = generateMatcher(b, lburg, "src/mips.md", "mips.c");
    const sparc = generateMatcher(b, lburg, "src/sparc.md", "sparc.c");
    const x86 = generateMatcher(b, lburg, "src/x86.md", "x86.c");
    const x86linux = generateMatcher(b, lburg, "src/x86linux.md", "x86linux.c");

    const rcc = b.addExecutable(.{
        .name = "rcc",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    rcc.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{
            "alloc.c", "bind.c", "dag.c", "decl.c", "enode.c", "error.c",
            "event.c", "expr.c", "gen.c", "init.c", "inits.c", "input.c",
            "lex.c", "list.c", "main.c", "null.c", "output.c", "prof.c",
            "profio.c", "simp.c", "stab.c", "stmt.c", "string.c", "sym.c",
            "symbolic.c", "bytecode.c", "trace.c", "tree.c", "types.c",
        },
        .flags = c_flags,
    });
    rcc.root_module.addIncludePath(b.path("src"));
    inline for (.{ dagcheck, alpha, mips, sparc, x86, x86linux }) |source| {
        rcc.root_module.addCSourceFile(.{ .file = source, .flags = c_flags });
    }

    b.installArtifact(lburg);
    b.installArtifact(rcc);

    const rcc_step = b.step("rcc", "Build rcc and its lburg-generated tables");
    rcc_step.dependOn(&rcc.step);
}

fn generateMatcher(
    b: *std.Build,
    lburg: *std.Build.Step.Compile,
    input: []const u8,
    output: []const u8,
) std.Build.LazyPath {
    const run = b.addRunArtifact(lburg);
    run.addFileArg(b.path(input));
    return run.addOutputFileArg(output);
}
