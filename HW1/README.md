# CS 441 Milestone 1 README

Hi, welcome to the README for CS 441 Milestone 1.
Details regarding the usage and peephole optimization are
provided below, after which I note some assumptions about
the language, compiler, and design decisions.

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

If for any reason the compiler fails to compile a correct
program (i.e. you get a *ParserException*), please contact
me and let me know what the issue was and what code you
provided. I have tested this compiler with multiple complex
programs and it has parsed correctly for each one, but
I do not know if there are any remaining bugs that I have
not addressed. The compiler *should work* for every program
you give it, but let me know otherwise.

Some flags are provided for testing.
- `-printAST` prints the AST generated from the source language
  file as a JSON object. You can then load this into a JSON
  pretty printer (i.e. `jq`) to see what data is contained in
  the tree and how it is structured. There will be no IR
  output if this flag is enabled.
- `-noSSA` disables conversion to SSA form. The IR output
  will still be valid for the given program but it will not
  have versioning for the various parameters and as a result
  contain zero phi statements.
- `-noopt` disables the peephole optimization (pre-evaluate
  constant arithmetic expressions). The IR output will still
  be valid but some arithmetic expressions might perform more
  work. You can examine the IR output with and without this
  option with the ir441 binary to check performance gains
  with the peephole optimization enabled.

## Peephole Optimization

### Description of Optimization

I implemented the peephole optimization to pre-evaluate
constant arithmetic expressions. The conversion from AST
to CFG removes a lot of immediately obvious unnecessary
tag checks (don't need to tag check intermediate arithmetic
in a compound expression, and don't need to tag check
integer operands). However there is still a lot of unnecessary
arithmetic; a large arithmetic expression that builds a
constant integer will still perform each step individually
and there are a lot of unnecessary conversions from the
tagged versions of constants to the untagged equivalents.
Every arithmetic result de-tags the operands, computes
the value, then re-tags. For a sequence of multiple operations
with purely integer constants this is wasteful. Hence
I implemented pre-evaluating constant arithmetic expressions
which both removes intermediate computation in large constant
evaluation as well as removes unnecessary tagging and
de-tagging for constants.

### Example Programs

See `testopt<N>.441` where `<N>` is a number to see various
example source language inputs for testing the peephole optimization.
Output IR is provided as `testopt<N>_noopt.ir` for the IR with
`-noopt` enabled and `testopt<N>_opt.ir` for the IR with the
peephole optimization enabled. Performance metrics are provided
below.

### Where is Optimization Code

Please see **src/ArithmeticOptimizer.h**. This header file provides
code for an ArithmeticOptimizer object which performs the
optimization. The CFG representing the IR is run through this
(if -noopt is not specified) and the optimizer outputs a new
CFG with constant arithmetic expressions optimized. The optimization
is rather straightforward, (per method) every time we see a
constant arithmetic expression we compute the value of it. If
the destination is a temporary register, then we can just remove
the statement and replace every occurrence of the temporary
register with the constant value. If the destination is not
a temporary, then it is a variable, so we update the statement
to directly assign the variable register the precomputed constant
value.

## Language Assumptions

- Return statements will not modify the value. This is to preserve
  pointers/tagged ints so that on return from a class method, we
  can tell whether the return value is a pointer or an integer with
  no ambiguity. Note that main can also return. In this case for
  consistency we do not de-tag the value returned by main. The final
  value output if an integer will be the tagged version of the integer
  (2n + 1). This question was asked before on the class Discord but
  the answer seemed to indicate that return should not modify the
  underlying literal value.
- Conditional statements (if/ifonly/while) and print only accept
  integers. To evaluate these statements if the underlying expression
  is an integer, we need to de-tag the integer to get the "true value".
  As a result using a pointer (object) as the subexpression in a
  condition or in a print statement throws an error. In my opinion
  it doesn't really make sense to use an object as a conditional value
  (what is the definition of truthy in our source language) and it
  does not make sense to print as well.
- The parser will accept valid code and will reject most code that
  is syntactically wrong. It is lazy in terms of parse so that
  variable indentation and spacing between certain operands will
  still parse correctly. However it is not strict enough to account
  for invalid code with correct syntax, i.e. the parser will not
  reject a program that uses local variables that are not declared
  at the start of the method. Since it was brought up in class that
  there will be NO testing with regard to invalid code, the parser
  was only written to check for basic syntax issues and nothing else.

