# CS 441 Milestone 3 README

Hi, welcome to the README for CS 441 Milestone 3.
Details regarding the usage and type checking are below.

## Requirements

g++ compiler with support for C++17 (this is available via Tux).
This is guaranteed to work on Linux, I have no guarantees for Windows.
Ideally it should work, but the Windows C++ compiler is *weird*.

## How to build

A makefile is included. Run "make" to build the compiler.
Run "make clean" to remove the compiler and intermediate binaries.

## Usage

After building with "make" run as follows:
`./comp < p.441 > p.ir`
where `p.441` is the input file (source language code) and
`p.ir` is the compiler output (IR code). By default the output
will be in SSA form and the IR will be optimized by a
peephole optimization.

Please let me know if the compiler fails to compile any program
with valid syntax. Also let me know if you run into any
segfaults, I spent a couple of hours this week ironing out any
bugs caused by value numbering and I believe it should work
correctly for all inputs, but let me know otherwise.

Some flags are provided for testing.

- `-printAST` prints the AST generated from the source language
  file as a JSON object. You can then load this into a JSON
  pretty printer (i.e. `jq`) to see what data is contained in
  the tree and how it is structured. There will be no IR
  output if this flag is enabled.
- `-noSSA` disables conversion to SSA form. The IR output
  will still be valid for the given program but it will not
  have versioning for the various parameters and as a result
  contain zero phi statements. This flag will also disable
  value numbering (as value numbering requires SSA enabled
  in order to optimize the program).
- `-noopt` disables the peephole optimization (pre-evaluate
  constant arithmetic expressions). The IR output will still
  be valid but some arithmetic expressions might perform more
  work. You can examine the IR output with and without this
  option with the ir441 binary to check performance gains
  with the peephole optimization enabled.
- `-simpleSSA` disables the optimized SSA transformation
  and instead runs the maximal suboptimal SSA transformation.
  By default the compiler will use optimized SSA.
- `-noVN` disables value numbering. Value numbering is
  performed after SSA transformation. With this flag enabled,
  IR output should have more ALU operations, more memory
  reads, and more conditional branch tag checks.

## Type Checker

### Where is Optimization Code

The better SSA optimizer is provided in `src/TypeChecker.h`.
This iterates over the AST output before passing the program to the CFG.
It ensures that the program is statically typed correctly. If there is an
issue, it will throw an exception which will be printed and the program
will exit (there will be no IR output).

The exceptions try to be somewhat meaningful and detail the expression or
function name causing an issue but they are not "great" error messages, as
an FYI.

## Refactoring

With the new static typing there were some major overhauls made to parts
of the compiler, such as

- CFGs are structured slightly better in terms of maps and some steps
  were moved around to make more sense (e.g. class definitions are
  parsed first before class methods to ensure all type information
  is correct).
- Optimizers now track typing of each register. In the case of the SSA
  optimizers they also track the type of new SSA registers. The tracking
  of typing is relatively unimpactful save for field lookups which now
  require the type info so that the field map can just be inlined.
- Tagging is removed, all integers are preserved as literal values and
  there are no more tag checks. Some identities like x / 1 and x * 1
  now work properly with value numbering.
- Field access is direct. We store a map of field name to offset for
  each object internally when building the CFG and use that to directly
  compute getelt/setelt offsets. This also means without the field map
  classes are now 1 byte smaller to allocate.
- Method lookups are now no longer checked to see if they succeed but
  the vtable is still in place.
- Non-null checks now occur before every getelt/setelt/call. The only
  time a non-null check does not occur is if it is being called on
  %this (%this0 in SSA form) as we assume that it exists.
- Value numbering has been adjusted, now instead of potentially removing
  tag checks it has the potential to remove unnecessary non-null checks
  on SSA output.

## Test Programs

Two large test programs are provided in `test/untyped.441` and `test/typed.441`
which are untyped and typed as the names suggest. The untyped program will
only compile on the milestone 2 compiler and likewise the typed program
will only compile on the milestone 3 compiler. These programs test a stack,
queue, and Fibonacci function. Other programs are provided in the `test`
directory for additional testing but the two listed are the ones that
are required.

Here are the performance results (GVN, SSA, all optimizations enabled):

- `test/untyped.441 ExecStats`
   - `fast_alu_ops: 186974`
   - `slow_alu_ops: 81230`
   - `conditional_branches: 135115`
   - `unconditional_branches: 54599`
   - `calls: 24980`
   - `rets: 24981`
   - `mem_reads: 54995`
   - `mem_writes: 4251`
   - `allocs: 432`
   - `prints: 41`
   - `phis: 21`
- `test/typed.441 ExecStats`
   - `fast_alu_ops: 73554`
   - `slow_alu_ops: 33472`
   - `conditional_branches: 43981`
   - `unconditional_branches: 29199`
   - `calls: 24980`
   - `rets: 24981`
   - `mem_reads: 44080`
   - `mem_writes: 3819`
   - `allocs: 432`
   - `prints: 41`
   - `phis: 21`


