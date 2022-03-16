# CS 441 Milestone 4 README

Hi, welcome to the README for CS 441 Milestone 4 (+ honors project).
Details regarding the usage and functionality are below.

For Milestone 4, I implemented the GC Option.

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
- `-vectorize` enable a vectorization optimization.
  Vectorization is disabled by default. On enabling this,
  code will be optimized to remove redundant jump statements
  first so that basic block vectorization is easier. The
  optimization then applies the SLP extraction algorithm
  and then a second pass through value numbering.

## GC

### Where is Optimization Code

Changes were made to `src/CFGBuilder.cpp` so that class
allocation (for non-null allocs) populates slot -1
with a bitfield indicating which "real" slots hold
pointers. After allocating space, we subtract 8 to
get slot -1 and then calculate the bitfield. The bitfield
is determined from the type info stored in the backend;
any non-int field that is a class that we can look up
is determined to be a pointer and has the corresponding
bit set to 1. This information is relatively simple to
compute; the CFG has both type info and mapping of field
names to slot offsets, so we just compute this directly
(using a sequence of binary ORs) and store it.

See method `void CFGBuilder::visit(NewObjectExpression& node)`
(line 97) in `src/CFGBuilder.cpp` for the changes.

No other changes were required to support garbage
collection.

### GC Test Program

A program is included that tests the garbage collection.
This program is located in `test/gc.441`. This program
creates a stack and runs a loop 50 times. In the loop,
a number is added to the stack (creating a new node
object), then immediately popped off the stack (removing
the node) and the number is printed. Without garbage
collection, the removed nodes are still in memory wasting
space. With garbage collection, the removed nodes are
cleaned every time the GC runs and the program runs
to completion.

If you run with `exec-fixedmem` the program will
crash with an OutOfMemory error.
`./comp < ./test/gc.441 | ir441 exec-fixedmem`

If you run with `exec-gc` the program will run
to completion and print numbers 50-1 because the
GC works.
`./comp < ./test/gc.441 | ir441 exec-gc`

Note the latest `ir441` interpreter (`v2.1.2`) is
necessary to run the program, I had to make
the changes to the interpreter as my test program
would not run otherwise.

## Vectorization (Honors Project)

### How it Works

My honors project is an implementation of the
SLP extraction algorithm described in
"Exploiting Superword Level Parallelism with
Multimedia Instruction Sets" by Larsen and
Amarasinghe. The technique works by first
finding candidate sets of instructions to
pack together. Here the authors denote a
candidate pack as two instructions that
operate on directly adjacent data in memory,
are independent, and isomorphic (same instruction).
To identify candidate packs, I find adjacent
getelt or setelt instructions operating on
the same object that are 1 slot apart. To
simulate an array type, it is easy enough
to create a class representing an array or
vector by giving it enough fields. As the
fields are directly adjacent by slots, we
may treat the object like an array and may
vectorize it.

From there we then find uses of these slots
and group the uses together. This is where
we find arithmetic operations that can
be grouped together. We then trace back
operands inside these operations and work
backwards to find their definitions so that
the other operands can also be vectorized.
Once all packs are found, we then merge
packs together if they share a common
memory boundary (up until this point,
packs were each two instructions, but
we may now group together by shared
instructions).

Finally, the instructions are rescheduled
inside the new basic block. The scheduling
performs basic code motion to move the
instructions around to an optimal location
for vectorization. We schedule instructions
based on whether their dependencies are
already scheduled, resulting in a new
code order that is ideal for vectorization
but still preserves dependency ordering.

### Where is Optimization Code

The vectorization optimization is applied by
`src/VectorOptimizer.h`. This implements
the SLP extraction algorithm. Note that originally
the intent was to implement a portion of this
algorithm however I found it necessary to basically
implement the entire algorithm as a prototype in
order for it to work correctly. Some aspects like
finding optimal savings are not there but the core
algorithm is intact.

Also note that prior to writing this most
optimized code had a lot of redundant jump statements
created by value numbering. These statements got
in the way when vectorizing as the algorithm only
works on a basic block level. I implemented a jump
optimizer in `src/JumpOptimizer.h` which is supposed
to remove redundant jumps and merge blocks upward
so that vectorization is easier to perform. This
optimization is DISABLED and only enabled when
compiling with `-vectorize`.

### Limitations

There are a number of known limitations with this
technique and as a warning the vectorization will
NOT produce valid code for all inputs. However, of
the inputs that it does support, it produces
valid vectorization code and demonstrates that
the algorithm works correctly. Thus I feel the work
done here is sufficient for the honors project.

First, the jump optimization is largely untested
on complex inputs and similarly dependency tracing
is barebones. I believe jump optimization is correct
but it is disabled by default as I would rather not
have this optional component break code for the GC
requirement. Dependency tracing is based on SSA -
if the RHS variables have been defined in the past,
then the dependency requirement is satisfied and
the instruction can be scheduled. For a single block
this works however across multiple blocks it
gets complicated. Phi statements are meant to resolve
conditional dependencies so when doing dependency
tracing, phi statements are automatically scheduled
(we assume dependencies are already resolved by
the predecessor blocks). Code with conditional
statements and loops should still compile but I make
no guarantee that this works everywhere.

The main limitation right now is that code with field
dependencies on read/write can be scheduled incorrectly.
While cyclic dependencies can happen, usually the
algorithm schedules them correctly as we schedule the
earliest statement first. However combining field
reads and writes to the same object can be problematic.
Observe if we have something like

```
a = read(data[0])
write(data[1], x)
b = read(data[1])
```

We could vectorize this as

```
write(data[1], x)
a = read(data[0])
b = read(data[1])
```

where the two subsequent reads are done in parallel after
the write. However, getelt/setelt calls do not use SSA
convention to mark each update (we just update the field
directly) so the dependencies fail to get traced by
the optimizer and we end up with something like

```
a = read(data[0])
b = read(data[1])
write(data[1], x)
```

which is wrong. So, I would recommend NOT mixing reads/writes
to the same object in a method. Rather, you can get around
this issue by just using variables which are handled by SSA
and thus can easily find dependencies. For example:

```
a = read(data[0])
temp = x
write(data[1], temp)
b = temp
```

We eliminate the second read and replace it with the value
of the read fixing the vectorized code but removing the
vectorization optimization in its place. Unfortunately
the paper does not detail examples involving writes to
adjacent memory and so it is hard to address this issue
without writing some code analysis to find these dependencies.

It is perfectly fine however to read from one array and
write to another as long as you do not read/write from
the same array in a method. So something like

```
x[0] = y[0] + z[0]
x[1] = y[1] + z[1]
```

is totally fine.

### IR Changes

There are six new IR commands. Here is an overview of each.

- `%VEC = vecload(%a, %b, %c, %d)` loads the registers/scalars into a vector.
- `%a, %b, %c, %d = vecstore(%VEC)` unloads the vector into the destination registers.
- `%VEC3 = vecadd(%VEC1, %VEC2)` perform parallel addition on vectors
- `%VEC3 = vecsub(%VEC1, %VEC2)` perform parallel subtraction on vectors
- `%VEC3 = vecmult(%VEC1, %VEC2)` perform parallel multiplication (lower 64 bits) on vectors
- `%VEC3 = vecdiv(%VEC1, %VEC2)` perform parallel division (integer division) on vectors

When scheduling the packed statements the arithmetic statements are unrolled by size 4
so we use vectors storing 4 64-bit integers. Note that each arithmetic statement essentially
translates to two loads, an operation, and a store, e.g.:

```
%VEC1 = vecload(...)
%VEC2 = vecload(...)
%VEC3 = vecadd(%VEC1, %VEC2)
... = vecstore(%VEC3)
```

Note this is rather redundant if you repeatedly perform operations on the same vector.
This translation was done purely to preserve code functionality so that any later operations
depending on intermediate variable results still work. With some future changes to
value numbering perhaps redundant loads and stores could be removed. The algorithm provided
in the paper detailed how to reschedule the operations to support vectorization. It did
not however directly translate the operations to vector instructions. After rescheduling
operations, each packed set of operations was given a macro to schedule the operations
with the AltiVec instruction set. Here I just extended the scheduling of packed instructions
to replace the instructions with a direct vector translation that still preserves functionality,
but in the long run the excessive loads and stores might actually result in worse performance.

(This is really meant as a prototype and the point is, the implementation of the
algorithm works - regardless of whether it is more performant or not).

### Vectorization Test Program

An extensive test program has been supplied in `test/vector.441`. This program defines
a 4x1 vector type and a 4x4 matrix type. Matrix-vector multiplication is implemented.
If you compile by default, you will see a lot of getelt statements followed by individual
multiplications and additions in various orders. In total there are 16 multiplications
and 12 additions. The compiler by default will perform these operations in the order listed,
so we will compute the first vector component, then the second, then the third, and finally
the fourth.

When running the compiler with `-vectorize` you will see that the code is greatly cleaned
up and much more straightforward. All getelt statements are moved to the top of the method
and all setelts are moved to the bottom. In between we have the matrix-vector multiplication
fully vectorized. There are four calls to `vecmul` for the 16 multiplications; one call
per row of the matrix. Afterwards there are then three calls to `vecadd` for the 12
additions. The first `vecadd` call computes the leftmost and rightmost sums for
the first two rows of the vector. The second `vecadd` call computes the leftmost
and rightmost sums for the last two rows of the vector. The final `vecadd` call merges
the leftmost and rightmost sums together (the middle sum) for all rows resulting
in the final vector result. The operations are scheduled correctly for dependencies
and thus demonstrates that the vectorization optimization works for simple basic
block examples.

