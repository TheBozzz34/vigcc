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
            "alloc.c",    "bind.c",     "dag.c",   "decl.c", "enode.c",  "error.c",
            "event.c",    "expr.c",     "gen.c",   "init.c", "inits.c",  "input.c",
            "lex.c",      "list.c",     "main.c",  "null.c", "output.c", "prof.c",
            "profio.c",   "simp.c",     "stab.c",  "stmt.c", "string.c", "sym.c",
            "symbolic.c", "bytecode.c", "trace.c", "tree.c", "types.c",  "vig.c",
        },
        .flags = c_flags,
    });
    rcc.root_module.addIncludePath(b.path("src"));
    inline for (.{ dagcheck, alpha, mips, sparc, x86, x86linux }) |source| {
        rcc.root_module.addCSourceFile(.{ .file = source, .flags = c_flags });
    }

    const cpp = b.addExecutable(.{
        .name = "cpp",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    cpp.root_module.addCSourceFiles(.{
        .root = b.path("cpp"),
        .files = &.{
            "cpp.c",     "lex.c",     "nlist.c",  "tokens.c", "macro.c", "eval.c",
            "include.c", "hideset.c", "getopt.c", "unix.c",
        },
        .flags = c_flags,
    });
    cpp.root_module.addIncludePath(b.path("cpp"));

    const assembler_package = b.dependency("vig_assembler", .{
        .target = target,
        .optimize = optimize,
    });
    const vigasm = assembler_package.artifact("vigasm");

    const linker_package = b.dependency("vig_linker", .{
        .target = target,
        .optimize = optimize,
    });
    const vigld = linker_package.artifact("vigld");

    const make_crt0 = b.addRunArtifact(vigasm);
    make_crt0.addArg("-c");
    make_crt0.addFileArg(b.path("runtime/crt0.vigas"));
    make_crt0.addArg("-o");
    const crt0 = make_crt0.addOutputFileArg("crt0.vigo");
    const tool_options = b.addOptions();
    tool_options.addOptionPath("cpp_path", cpp.getEmittedBin());
    tool_options.addOptionPath("rcc_path", rcc.getEmittedBin());
    tool_options.addOptionPath("vigasm_path", vigasm.getEmittedBin());
    tool_options.addOptionPath("vigld_path", vigld.getEmittedBin());
    tool_options.addOptionPath("crt0_path", crt0);
    tool_options.addOption(
        []const u8,
        "vig_include_path",
        b.pathJoin(&.{ b.root.root_dir.path orelse ".", b.root.sub_path, "include", "vig" }),
    );
    const tool_options_module = tool_options.createModule();

    const vigcc = b.addExecutable(.{
        .name = "vigcc",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/vigcc.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{.{ .name = "tool_options", .module = tool_options_module }},
        }),
    });

    b.installArtifact(lburg);
    b.installArtifact(rcc);
    b.installArtifact(cpp);
    b.installArtifact(vigcc);
    const install_vigasm = b.addInstallArtifact(vigasm, .{});
    b.getInstallStep().dependOn(&install_vigasm.step);

    const rcc_step = b.step("rcc", "Build rcc and its lburg-generated tables");
    const install_vigld = b.addInstallArtifact(vigld, .{});
    b.getInstallStep().dependOn(&install_vigld.step);
    rcc_step.dependOn(&rcc.step);

    const vm_package = b.dependency("vig", .{
        .target = target,
        .optimize = optimize,
    });
    const vig = vm_package.artifact("vig");
    const corpus_options = b.addOptions();
    corpus_options.addOptionPath("vigcc_path", vigcc.getEmittedBin());
    corpus_options.addOptionPath("vig_path", vig.getEmittedBin());
    corpus_options.addOption(
        []const u8,
        "corpus_dir",
        b.pathJoin(&.{ b.root.root_dir.path orelse ".", b.root.sub_path, "tests", "vig" }),
    );
    const corpus_options_module = corpus_options.createModule();
    const corpus_tests = b.addTest(.{
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/corpus_tests.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{.{ .name = "corpus_options", .module = corpus_options_module }},
        }),
    });
    const test_step = b.step("test", "Run the VIG C compiler corpus");
    const run_corpus_tests = b.addRunArtifact(corpus_tests);
    // The test executable opens `.c` and `.expected` files at run time.  Those
    // files are not individual build inputs, so run it on every `zig build test`
    // rather than letting a cached test hide a corpus edit.
    run_corpus_tests.has_side_effects = true;
    test_step.dependOn(&run_corpus_tests.step);
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
