# CS 441 Milestone 2 README

Hi, welcome to the README for CS 441 Milestone 2.
Details regarding the usage, optimized SSA, and
value numbering are provided below.

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

## Optimized SSA

### Where is Optimization Code
The better SSA optimizer is provided in `src/BetterSSAOptimizer.h`.
Most of the logic is reused from the standard SSA optimizer in terms
of converting variables to variables with versioning. The main difference
is that the phi node placement now depends on the iterative algorithm
that uses the dominance frontiers rather than just placing a phi node
for every variable at the start of a block with multiple predecessors.
To compute the dominators, immediate dominators, and dominance frontiers
I wrote another class in `src/DominatorSolver.h` that does this.

### Example Program
An example program is provided in `test/phi.441`. When running this program
with `ir441 perf`, the number of `phi` nodes listed and executed is as follows:

- With simple SSA (`-simpleSSA`) and no value numbering (`-noVN`)
  `ExecStats { phis: 65 }, Number of phi nodes in IR: 30`
- With simple SSA (`-simpleSSA`) and value numbering
  `ExecStats { phis: 50 }, Number of phi nodes in IR: 21`
- With better SSA and with/without value numbering (`-noVN` does not affect phi nodes here)
  `ExecStats { phis: 24 }, Number of phi nodes in IR: 9`

### Extra Notes
The better SSA optimizer appears to work correctly for all inputs that
I have tested. In testing the dominator solver, it appears to correctly
compute the dominators. There is a helper class which goes through
and finds the phi nodes prior to running the versioning. The versioning
works as before and if a phi node needs to be placed, we simulate the
version update prior to placing it as we don't know the phi parameters
until later (if the CFG has a cycle, need to evaluate children later before
adding it in). Later once we are done versioning, we add the phi
nodes in. The implementation of this algorithm is direct from the
textbook and class notes and appears to work well.

## Value Numbering

### Where is Optimization Code
The value number optimizer is provided in `src/ValueNumberOptimizer.h`.
This uses `src/DominatorSolver.h` to compute the dominator tree.
I calculate this from the immediate dominators and wrap the CFG nodes
in dominator nodes that expose the sub-tree within the CFG to traverse.
The value numbering implemented is global value numbering (GVN) using
dominators. The example program provided is rather simple but
due to tag checks it does demonstrate that value numbering works
across blocks. You may also try experimenting with if/ifonly/while and
see that it works correctly with more complex programs. It appears
to work correctly for all example programs that were provided
in the class Discord channel.

### Example Program
The example program is included in `test/vn.441`. This program
demonstrates the following identities:
- Multiplicative commutativity (`(z * 3) = (3 * z)`)
- Addition identity (`(z + 0) = z`)
- Additive commutativity (`(x + 7) = (7 + x)`)
- Mult/div identities (`(z * 1) = (z / 1)`)
The program is just a single class method that gets called by main.
The method is in a class rather than in main, as if this were part of
main the entire method would compile to just some jumps and print
statements. Here in the method, the parameters cannot be assumed
so we are able to see exactly which assignments are kept and which
are removed. In particular the program is sectioned into 7 areas:
- `p = (2 * z)`, `y = (z * 3)`, `x = (3 * z)`. Here we update
  `p` and `y`. The assignment to `x` is removed and on printing `x`
  we instead print the value stored in `y`. Note that every time we
  use new variables, we perform number tag checks, but after the
  first use the number is not checked. See here that the initial
  values of `x` and `y` passed to the method are never used, so we
  have tag checks on `z` and later on `p` and `y` (we greedily
  tag-check every operand to an arithmetic expression that has not
  been checked yet), but do not have tag checks on the initial
  `x` and `y`.
- `x = (y + z)`, `x = (z + 3)`. We first compute the sum of `y + z`
  into a version of `x` (`x2`) and then compute the actual value of
  `x` as `z + 3` and store in `x3`. This is the value printed.
- `y = (y + z)`. Here we reuse `x2`, the previously computed but
  unused value of `x`, rather than compute `y`.
- `p = (z + 0)`. Here we print the value of `z` and complete skip
  the assignment and the addition of zero.
- `y = (x + 7)`, `x = (7 + x)`. We compute `x + 7` and store into
  `y` then print it. We skip the addition and assignment for `x`
  and instead print the value of `y` twice.
- `q = (z * 1)`, `p = (z / 1)`. We compute `z` to `q` and print it
  but skip the multiplication by 1. Note that due to tag checking
  the multiplication step in the original IR required `z / 2` instead
  of `z` which results in some redundant steps (this is explained below
  in "Extra Notes"). However the multiplication is skipped and we
  print `q`. For `p`, we skip all steps and instead print `q` a
  second time.
- `z = (z + 1)`, `p = (2 * z)`, `x = (x + 1)`. Here we update
  `z`. For `p`, we compute `2 * z` instead of reusing `2 * z` from
  earlier as we have a new version of `z`. To compute `x` we need
  to use `y3` from before as we skipped the last assignment for `x`.
  `y3` stores `7 + x` which is the current value of `x` so we use
  that and increment to that to calculate the new value of `x`.

Here are some performance metrics:
- Without value numbering
  `ExecStats { fast_alu_ops: 53, slow_alu_ops: 36, conditional_branches: 30, unconditional_branches: 0, calls: 1, rets: 2, mem_reads: 2, mem_writes: 6, allocs: 1, prints: 20, phis: 0 }`
- With value numbering
  `ExecStats { fast_alu_ops: 31, slow_alu_ops: 16, conditional_branches: 11, unconditional_branches: 19, calls: 1, rets: 2, mem_reads: 2, mem_writes: 6, allocs: 1, prints: 20, phis: 0 }`

Another program `test/vn2.441` is provided for further testing.
This program is pulled from an example from the paper that mentioned
the dominator-based value numbering technique. Like the paper suggests
in this example value numbering removes all assignments in both
the if block and the else block. Two of three phi nodes are removed
and occurrences of x at the end are replaced with y (the paper replaces
y with x but they are equivalent, my program just iterates the variables
in a different order).

Performance metrics:
- Without value numbering
  `ExecStats { fast_alu_ops: 52, slow_alu_ops: 11, conditional_branches: 27, unconditional_branches: 1, calls: 1, rets: 2, mem_reads: 5, mem_writes: 3, allocs: 1, prints: 6, phis: 3 }`
- With value numbering
  `ExecStats { fast_alu_ops: 27, slow_alu_ops: 10, conditional_branches: 16, unconditional_branches: 12, calls: 1, rets: 2, mem_reads: 5, mem_writes: 3, allocs: 1, prints: 6, phis: 1 }`

### Extra Notes
Value Numbering will optimize the output IR greatly but it will not
capture all optimizations as mentioned in class. In particular there
are some things you should be aware of with how my code does
value numbering.
- The algorithm is recursive as designed, but runs iteratively.
  The iterative running of the algorithm is mainly due to phi nodes.
  On optimizing some variable, we might optimize it away and find
  that it is not necessary for program execution. We have to then
  later update the direct CFG children's phi nodes to capture this
  change so that the phi nodes do not reference some removed variable.
  However, this could in-turn make those children phi nodes redundant.
  If we do not run into those children later during the dominator tree
  traversal (e.g. they are part of a loop and were processed earlier),
  then we cannot optimize this out. Furthermore optimizing the child's
  phi node out might in turn affect other phi nodes, which could in
  turn be made redundant. The paper detailing dominator based value
  numbering did not seem to provide a useful explanation regarding
  the algorithm in terms of loops (really cycles in the CFG). To
  get around this, if any child phi node is updated, I re-run the
  algorithm after it completes to remove any additional redundancies.
  The algorithm loops repeatedly until no phi nodes are updated,
  indicating that the value numbering has converged on an "optimal"
  program.
- Load statements are hashed and reused through value numbering.
  Currently I only use load for loading the vtable or the fields map
  which both are arrays that stay constant throughout execution of
  a program, so it is fine to cache these for later use.
- Direct identities will be captured, but not complex identities.
  You can actually see this issue with the `nothing.441` program
  provided in the class Discord channel.
  This can result in some redundant code due to tag checking. For
  example, to convert a number from tagged to non-tagged, we divide
  by 2 (floored division). To convert back to tagged, we multiply
  by 2 and add 1. This conversion to-and-from tagged ints is an
  identity if the input given is odd. If the input is somehow
  even, then the result will be the original number plus 1. So
  the complex identity only works if we know the input is tagged
  prior. Unfortunately due to how tagged ints are required by
  the compiler, simple operations like `(x * 1)` end up compiling
  into a de-tag of `x`, a multiply, and a re-tag, wasting cycles.
  The compiler isn't smart enough currently to recognize that `x`
  is tagged, and the value number optimizer does not handle
  complex identities - only direct identities. This means it can
  handle `(x * 1)` if it appears in the IR directly, but not
  necessarily at the source level (as mentioned, this will really
  compile into `x/2 * 1`, and we need to calculate `x/2` first).
  That being said once `x/2` has been calculated at least once,
  it can be reused by value numbering and other identities like
  `(x / 1)` can also be optimized as they work in a similar
  fashion.
- Redundant tag checks will be pruned, but if/else IR that evaluates
  constant conditionals that do not lead to tag checks will not
  be adjusted. For the optimizer, it's simple enough to check whether
  an if/else is a tag check based on the labels that it can jump to,
  and is simple enough to hash into the VN hashtable so that future
  tag checks on the same variable can be removed. When we remove
  the check, we replace with a jump statement and prune the failure
  block. Since we know the failure label will lead to a single block
  and that block has no other predecessors, it is very easy
  to remove. However if/else statements that end up evaluating
  constant conditionals after value numbering do not get adjusted.
  The main issue here is that the "missed" label that we do not jump
  to might have other predecessors and as a result should not be
  pruned. In addition the other label might lead to a sub-CFG with
  multiple blocks, meaning we might need to prune a sub-graph rather
  than one block. I attempted to implement this but it ended up
  being more work than expected, and lead to a majority of segfault
  issues that I was experiencing. I have disabled this adjustment
  as I feel that removing tag checks makes sense for value
  numbering to handle, but removing other redundant if/else
  statements is basically dead code elimination and does not
  count as value numbering. Perhaps in the future I will add another
  optimizer pass which later goes through and fixes these checks.
- Anything that can be inlined will be. There will be no direct
  assignment statements of the form `x = y` (`y` a variable or
  a constant). If you have a really simple program that can be
  precomputed, then the entire code will precompute and you
  will get rid of all arithmetic statements. For example you
  can see this with the `optimal.441` program provided in the
  class Discord channel. This can make it hard at times to see
  whether value numbering is working correctly as you essentially
  lose access to the variables. There are multiple ways to force
  variables to appear in the IR output, such as assigning
  variables to non-const or non-variable expressions (e.g.
  phi nodes, method calls, alloc), using a for loop that updates
  a variable, and using method fields and parameters in a
  class method (we cannot assume anything about the value of these
  variables, so we have to use them in arithmetic expressions).

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
- All method locals and class fields are initialized to tagged 0.
- By default if a method has no return statement we assume the method
  returns literal (not tagged) 0.

## Updates to Existing Code Since Milestone 1

- SSA is now performed at the method level rather than the entire program
  level. Some refactoring was done to only track basic blocks per each
  method rather than track basic blocks across the entire program, so that
  we can build distinct dominator trees per method. This should not
  affect IR output.
- There was a very minor bug where SSA would set the first assignment of a
  method parameter inside a class method to be version 0 instead of version 1.
  For the majority of programs (99.99%) this would not really affect anything.
  Even if you encountered the bug, it would not really break your code as the
  assignment had to be the first statement the method would run with that variable
  (meaning version 0 of the variable just went unused). However this is not
  "technically" correct and so this issue has been fixed.
- Parser has been updated slightly to be more lazy. Now, you may list
  the keyword "fields" in a class with no fields to indicate a class
  has no fields. Previously the only way to do this was to just
  completely remove the line containing "fields". Also, the main method
  accepts zero variables if you title it "main with:". Previously
  the parser would only accept a main method with 1 or more locals.

