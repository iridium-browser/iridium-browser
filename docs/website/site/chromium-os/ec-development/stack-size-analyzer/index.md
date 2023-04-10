---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/ec-development
  - Chromium Embedded Controller (EC) Development
page_name: stack-size-analyzer
title: Stack size Analyzer
---

[TOC]

## Objective

The EC (Embedded Controller) is a chip within Chromebooks that takes care of
tasks such as keyboard scanning, USB-C negotiation, and battery charging among
other things. They are low power chips that are almost always on (even when
system is off), and are quite limited in terms of RAM (few KB), and flash
(100-500 KB)

Each EC task has a specific, separate stack, with a fixed size. We would like to
be able do static analysis of the EC code to be able to tell, in advance, what
is a reasonable stack size value.

# Background

At runtime, the maximum task stack depth can be probed with taskinfo console
command. However, this only gives us the maximum stack depth for the current EC
runtime lifetime, and not the absolute maximum. If the stack ever exceeds the
maximum, the EC will panic and reboot (taking down the whole system with it).

The task size is currently set empirically, by copying from previous
boards,examining taskinfo, and adjusting where the margin is uncomfortably low.
However, this does not cover all cases, especially rare error cases that may
overflow the stack size.

# Overview

### EC Stack Analyzer

Run static analysis on EC firmware binary and report maximum possible stack
usage of each task. The annotation feature allows users to provide extra
information and improve the result of analysis.

# Usage

### Basic Usage

Assume you want to get the report of ${BOARD}.Under src/platform/ec:

    Make sure the EC firmware of ${BOARD} is built.

        make BOARD=${BOARD} -j

    make BOARD=${BOARD} analyzestack

        Each EC firmware has two sections, RO and RW. You can analyze different
        sections by running make BOARD=${BOARD} SECTION=RO or RW analyzestack,
        default is RW.

### Report Explanation

#### Task Stack Usage

For each task, it will output the result like below,

Task: PD_C1, Max size: 1156 (932 + 224), Allocated size: 640

Call Trace:

pd_task (160) \[common/usb_pd_protocol.c:1644\] 1008a6e8

-&gt; pd_task\[common/usb_pd_protocol.c:1808\] 1008ac8a

- handle_request\[common/usb_pd_protocol.c:1191\]

- handle_data_request\[common/usb_pd_protocol.c:798\]

-&gt; pd_task\[common/usb_pd_protocol.c:2672\] 1008c222

-&gt; \[annotation\]

pd_send_request_msg.lto_priv.263 (56) \[common/usb_pd_protocol.c:653\] 1009a0b4

-&gt; pd_send_request_msg.lto_priv.263\[common/usb_pd_protocol.c:712\] 1009a22e0

The first line,

Task: PD_C1, Max size: 1156 (932 + 224), Allocated size: 640

means the maximum possible stack usage of PD_C1 is 932 bytes, plus 224 bytes
used by exception context switching. The total maximum stack usage is 1156
bytes. And the allocated stack size of PD_C1 is 640 bytes.

The pd_task function uses 160 bytes stack by itself and calls
pd_send_request_msg.lto_priv.263 (which uses 56 bytes).

The callsites to the next function will be shown like below,

-&gt; pd_task\[common/usb_pd_protocol.c:1808\] 1008ac8a

- handle_request\[common/usb_pd_protocol.c:1191\]

- handle_data_request\[common/usb_pd_protocol.c:798\]

-&gt; pd_task\[common/usb_pd_protocol.c:2672\] 1008c222

-&gt; \[annotation\]

This means one callsite to the next function(pd_send_request_msg.lto_priv.263)
is at usb_pd_protocol.c:798, but it is inlined to the current function and you
can follow the trace:

usb_pd_protocol.c:1808 -&gt; usb_pd_protocol.c:1191 -&gt; usb_pd_protocol.c:798
to

find the callsite.

The second callsite is at usb_pd_protocol.c:2672.

And the third one is added by annotation (see below)

#### Unresolved Indirect Callsites

The indirect callsites are the function calls to function pointers. The tool
will report all indirect callsites that it can not resolve.

Unresolved indirect callsites:

In function dfp_consume_attention:

-&gt; dfp_consume_attention\[common/usb_pd_policy.c:499\] 802bf4c

In function motion_sense_task:

-&gt; motion_sense_task\[common/motion_sense.c:889\] 8029afe

- motion_sense_process\[common/motion_sense.c:718\]

- motion_sensor_time_to_read\[common/motion_sense.c:194\]

-&gt; motion_sense_task\[common/motion_sense.c:889\] 8029b28

- motion_sense_process\[common/motion_sense.c:720\]

- motion_sense_read\[common/motion_sense.c:657\]

The first item means in dfp_consume_attention, there is one callsite can’t be
resolved, which is at common/usb_pd_policy.c:499.

The second item means in motion_sense_task, there are two unresolved callsites.
Both of them come from inlining, so the real indirect calls are at
common/motion_sense.c:194 and common/motion_sense.c:657.

#### Detected Cycles

The tool will report all recursive call cycles (there should never be recursive
calls in EC code). The cycles will cause the stack usage analysis become
inaccurate. If a cycle won’t happen (e.g. bounded recursion), you can remove it
by annotating an invalid path, otherwise, the code needs to be fixed.

There are cycles in the following function sets:

\[panic_txchar\]

\[typec_set_input_current_limit, pd_set_input_current_limit,
charge_manager_update_charge, pd_request_power_swap,
charge_manager_set_override, set_state\]

The first list means there is a self recursion in panic_txchar

The second list indicates those functions are formed a strongly connected
components, which means for any two functions A, B in the list, there is a path
from A to B and a path from B to A. Therefore there are many cycles between
those functions.

#### Unresolved Annotation Signatures

This section shows the unresolved annotation signatures and their reasons

Unresolved annotation signatures:

panic_assert_faila: function is not found

### Annotation

You can add missing call edges and remove invalid paths by writing an annotation
file.

The tool will try to load the annotation from board/${BOARD}/analyzestack.yaml
by default. So if the annotation file is placed at that path, no extra argument
is required.

Otherwise, the annotation file can be specified by running:

make BOARD=${BOARD} ANNOTATION=${ANNOTATION_FILE} analyzestack

#### Sample Annotation File

The annotation file is in yaml format, the following is an example annotation
file.

# Size of extra stack frame needed by exception context switch.

exception_frame_size: 64

# Add some missing calls.

add:

# console_task also calls command_display_accel_info and command_accel_init.

console_task:

- command_display_accel_info

- command_accel_init

# Function name can be followed by \[source code path\] to indicate where it is
declared (there may be several functions with same names).

motion_lid_calc\[common/motion_lid.c\]:

- get_range\[driver/accel_kionix.c\]

# The full signature (function name\[path:line number\]) can be used to
eliminate the indirect call (see README.md).

tcpm_transmit\[driver/tcpm/tcpm.h:142\]:

- anx74xx_tcpm_transmit

# Remove some call paths.

remove:

# Remove all callsites pointing to panic_assert_fail.

- panic_assert_fail

- panic

- \[software_panic\]

# Remove some invalid paths.

- \[pd_send_request_msg, set_state, pd_power_supply_reset\]

# The set_state can do recursive calls twice (set_state -&gt; set_state), but
not thrice.

- \[set_state, set_state, set_state\]

# Remove two invalid paths with the common prefix.

- \[pd_execute_hard_reset, set_state, \[charge_manager_update_dualrole,
pd_dfp_exit_mode\]\]

# It is equivalent to the following two lines,

# - \[pd_execute_hard_reset, set_state, charge_manager_update_dualrole\]

# - \[pd_execute_hard_reset, set_state, pd_dfp_exit_mode\]

# Remove four invalid paths with the common segment.

- \[\[pd_send_request_msg, pd_request_vconn_swap\], set_state, \[usb_mux_set,
pd_power_supply_reset\]\]

# It is equivalent to the following four lines,

# - \[pd_send_request_msg, set_state, usb_mux_set\]

# - \[pd_send_request_msg, set_state, pd_power_supply_reset\]

# - \[pd_request_vconn_swap, set_state, usb_mux_set\]

# - \[pd_request_vconn_swap, set_state, pd_power_supply_reset\]

# Detailed Design

To calculate the max stack usage of each function, we need the following
information:

    The stack frame sizes of functions

    The caller-callee relations of functions (callgraph).

With these informations, we can get the Maximum Stack Usage (MSU) of a function
by finding the maximum calling path and calculating the total stack usage on the
path.

### Finding Calling Path with Maximum Stack Usage

To find the maximum path on the callgraph, if there is no cycle in the
callgraph, we can use the dynamic programming method. Every function can get its
max stack usage by reusing the calculated max stack usage of its callees. Here
is the transition function (without considering the different calling
relations):

maximum_stack_usage = max(self_stack_frame_size, self_stack_frame_size +
maximum_stack_usage_of_callee_i for all callees)

If there are cycles in the callgraph, since the length of maximum path will
become infinite, we can’t get accurate stack usage result in this situation.
Therefore, we just remove those back edges which cause cycles and get a directed
acyclic callgraph, then report those cycles to users for warning.

### Calling Relation

There are two kinds of calling relations:

    Normal call

        The callee pushes new stack frame and pops after returning.

    Tail call

        The callee reuses the caller’s stack frame and pops both stack frames
        after returning.

        It is basically a jump, generated by compiler when it is sure that there
        is no more things to do after this call, so the caller’s stack frame can
        be destroyed and reused.

We need to handle different kinds of calling relations when calculating the
stack usage of the calling path.

For a normal call: the_max_stack_usage_of_the_call = caller_stack_frame_size +
callee_max_stack_usage

For a tail call: the_max_stack_usage_of_the_call = max(caller_stack_frame_size,
callee_max_stack_usage)

When the calling target is a function pointer, it will be hard to determine the
its callees. So we support the annotation to let users add these missing calling
relations by manually analyzing the code.

### Reporting

A fixed-size stack is assigned to each task, so the tool needs to report the
possible max stack usage of each task. Each task starts from a task routine
function, so the max stack usage of the task routine functions will be the max
stack usage of their tasks. As a result, the tool reports the stack usage
information of each task routine function.

To make it easy for users to trace the calling paths, the tool outputs the
calling paths of each routine functions with the callsites between the callers
and callees on the paths. We only get the addresses of callsites during
analysis, so the tool uses debug informations (which are added if the binary is
compiled with “-g”) to resolve the file and line number of each address.

Function inlining will make the callsites harder to be traced because they are
inlined into other functions, so you can’t find the real function calls in the
caller’s source code directly. The debug information provides inlining stack of
each address, so the tool will output the complete inlining stack of the
callsite in this case.

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/smNpbdQn4fwNyua1T6c4YNg/image?w=560&h=420&rev=1&ac=1"
height=420 width=560>

# Implementation

## Retrieving Stack Frame and Callgraph

We disassemble the ELF binary of EC firmware and analyze the disassembly to get
these information.

### Why We Use Disassembly

To get the stack frame sizes, GCC’s -fstack-usage can report the stack frame
size of each function, but it will be broken if we use the
Linking-Time-Optimization(LTO) to optimize the binary.

To get the most accurate stack frame sizes of functions, we parse the
disassembly of linked EC ELF to get rid of those optimization effects. The
trade-off is that we need to manually match all stack-related instructions, and
any unhandled ones can make the stack frame size calculation incorrect.

The callgraph exported from GCC still be affected by LTO (Parsing the LTO
information or writing LTO plugin may be possible to get accurate callgraph
after LTO). To keep it simple, we choose to disassemble the functions and detect
their call instructions.

The disadvantage of disassembling method is it will be architecture-dependent.
But most of our current firmware are using ARMv6-M (Cortex-M0, M1) or
ARMv7-M(Cortex-M3, M4), their disassembly (from objdump) is parsed by a single
parser with properly crafted regular expressions in this tool. To preserve the
extensibility, the architecture-dependent analyzer is written in an independent
class (e.g. ArmAnalyzer for ARM) and they have a common interface.

### Analyzing Processing

Doing disassembly will encounter several problems like

    Code discovery (which parts are code, which parts are data)

    Correctly switch between ARM thumb-mode and ARM mode (if the processor
    supports).

We use objdump directly to solve these problems (objdump already solves these
problem). The following is our disassembly processing

    Dump the text of symbol table and disassembly from objdump

    Parse the symbol table to get the symbols.

    Locate the functions in the text of disassembly, get the instruction texts
    of the function bodies.

    Pass each function body into architecture-dependent analyzer, the analyzer
    will parse the instruction texts and return the stack frame size and
    callsites.

    Build function objects from the returned data.

The interface of architecture-dependent analyzer is simple, just need to accept
a list of the function instructions and return the analyzed result of that
function.

## Annotation

To fix the missing information of callsites in the disassembly, an annotation
file can specified for adding missing function calls and removing invalid
function call paths.

### Resolving the Annotation

How to map the function signature (function name + the location in source code)
in the annotation file to the functions in disassembly is a problem. There are
four cases

    The function signature points to a single function.

    Multiple function signatures point to a single function.

        The function has different aliases.

    The function signature points to multiple functions.

        Multiple functions have same function name and location in debug
        information.

    The function signature points to nothing.

For case 3, the tool assumes those functions are actually the copies of a single
function (e.g. the function is defined in a header file and included by multiple
compilation units). Any operation on this function signature will be repeated on
all pointed functions.

For case 4, the tool reports an error to the user.

### Operation

After the function signatures are mapped to the functions in disassembly, we
only need to do operations on the functions. There are two kinds of operations:

    Add a caller-callee relation between two functions.

    Remove an invalid path on the callgraph.

        This path is invalid and shouldn’t appear in any calling path.

        The tool has to exclude this path when calculating the maximum calling
        path.

The tool adds the new caller-callee relations on the callgraph directly (creates
a new callsite). For invalid paths, the tool excludes those paths when
traversing the callgraph to calculate the max stack usage.

## Analyzing Stack Usage on Callgraph

For each function, to get the maximum calling path starting from it, the tool
tries to calculate the maximum calling paths of its callees and prepends it to
the head of the largest maximum calling paths of callees (makes the biggest
stack usage).

This is a recursive traversing processing which will be stopped when
encountering a function with no callee. But there are two more stopping cases:

    Found a cycle.

        Traversing on the cycle will cause infinite loop.

    There is an invalid path (from annotation) appearing in current traversing
    path.

For case 2, the tool tries to keep matching the suffix of the current traversing
path with all prefixes of invalid paths. If any matching prefix of invalid paths
is equal to the invalid path itself, it means the invalid path appears in the
suffix of the current traversing path, and the tool should stop from traversing
deeper.

The invalid path also changes the meaning of cycles on the callgraph, because it
can stop an infinite loop after repeating the cycle a limited number of times.
So here we define the “state” and “state transition graph”.

### State Transition Graph

A state transition graph(G) consists of the vertex(V): state and the edge(E):
state transition.

Define a state as a nested tuple:

State(V): (current function on the callgraph, (L1, L2, L3, … Ln))

Where current function is the tail of the current traversing path, n is the
number of invalid paths, the Lx is the length of longest matching suffix of the
current traversing path (excluding the current function at the tail) and prefix
of the invalid path x.

There can be multiple traversing paths stopping on the same function that have
the same state under this definition. Because the definition only considers the
longest matching suffix of traversing path.

There is a transition(E) from state A to state B if the function of A calls the
function of B and the tuple of matching lengths of A will be updated to the one
of B after appending the function of B to the current traversing path (so the
function of A isn’t the tail of the path and will be included in the suffix
matching)

To prove the transition is well-defined, we need to show that all traversing
paths having state A, after we append the function of B to these paths, will
transit to the same state B. For each invalid path, those traversing paths have
the same longest matching suffix with the prefix of the invalid path. For each
path, after we append the function B to it, the new longest matching suffix is
at most “the old one + the function of A”; otherwise the old one won’t be the
longest. Since the longest matching suffixes of all paths are the same, they
will have the same new longest matching suffixes after appended the function of
B. As a result, they will transit to the same state B.

### Traversal

The original idea of finding maximum calling path on callgraph with invalid
paths is traversing all paths (we can traverse a function multiple times).
During traversing, we keep matching the suffix of current traversing path with
invalid paths, and stop if we match any of them. And if we know there is no
invalid path we can stop the current traversing path from going into an infinite
loop in a cycle, stop too.

Since a function can be traversed multiple times, if a path walks into a cycle
on callgraph, it can only be stopped by matching an invalid path.

Assume we only have one invalid path. When traversing on callgraph, every time
the current path reaches a function, we will also have the length of longest
matching prefix of the invalid path with the current suffix. If this function is
already in the current path, there are two cases,

    The matching length at this function has been seen before (says position X
    in the path).

        This means we can follow the same sub path from X to here again, because
        on this sub path, there is no moment that the matching length is equal
        to the length of the invalid path (the suffix matches the invalid path)
        and we already proved that [the changes of matching length is only
        related to the previous matching
        length](https://docs.google.com/document/d/1Yt9mWfFM4BuRU9qYipIs8eo4MjLgXCKyHk_3gGWK_d0/edit?ts=59a7d62f#bookmark=id.m0yxxq2cmd0c).
        So there is a cycle.

    The matching length at this function is new.

        We may also find new matching lengths at other functions and some of
        them may match the invalid path.

Here is the example why it is possible to traverse a function twice and stop by
an invalid path.

If there is a cycle like the below on callgraph.

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/stolPC15LUeA3tjJDWQcnXA/image?w=427&h=203&rev=36&ac=1"
height=203 width=427>

And we set an invalid path A-&gt;B-&gt;C-&gt;A-&gt;B-&gt;C-&gt;A-&gt;B-&gt;C

The first time we reach the C function, the longest matching prefix is “A, B,
C”.

At second time, the matching prefix is “A, B, C, A, B, C”, which is new.

At the third time, the matching prefix is “A, B, C, A, B, C, A, B, C”, and
matches the invalid path.

So the invalid path preserves the cycle with two iterations, and removes the
longer cycles.

But if we set an invalid path A-&gt;B-&gt;C-&gt;C

The first time we reach the A function, the matching prefix is “A”.

At the second time, since our current path is A-&gt;B-&gt;C-&gt;A, the longest
matching prefix of the invalid path is only “A”, which has been seen at the
first time. So we can follow the same pattern to create an infinite loop, and
this invalid path doesn’t stop the loop.

Therefore, the idea of state transition graph is that we build different
vertices for each function with different matching lengths. When we traverse on
the state transition graph, if seeing a vertex twice in the current path, we
know there is an infinite loop.

The tool traverses the state transition graph instead of the original callgraph.
Since the state transition can be determined by the longest matching suffixes of
the traversing paths, which is equal to the longest matching prefixes of the
invalid paths, we don’t need the real traversing path on callgraph. The current
longest matching prefixes of invalid paths and the next callee function on
callgraph are enough for state transition and traversing.

### Invalid Path Matching and Cycle Detection

When reaching a state with a longest matching prefix is equal to the invalid
path, the current state is invalid (and the all traversing paths with this state
are invalid). The tool will stop from traversing deeper.

During traversing, the tool keeps all the parent states in a stack. If the next
state has been seen in the parent states, this means we can follow the same
transition path to reach here again. A cycle on the state transition graph is
detected.

The tool actually runs the strongly connected component algorithm on the state
transition graph during traversing and catch all state cycles.

### Calculating The Maximum Calling Path

The traversal on state transition graph can give us the state transition path
with maximum stack usage (by sum the stack frame size of the functions of those
states). And we can use the same idea of how we calculate the maximum calling
path on callgraph to find these maximum transition paths efficiently.

Then the tool convert the maximum transition paths to the maximum calling paths
for each function.

The main problem is instead of remember the max stack usage of each analyzed
function, this method has to remember the max stack usage of each state. The
number of states can be really large. So we assume the number of invalid paths
is small and they are short.

## Task-Aware Stack Usage

The task routines and configurations are different between boards and affected
by build options. To retrieve the full task configurations of each board, there
is a small C file will be compiled with the board and become a shared library.
It has a interface to return the compiled task configurations. The tool will
load this shared library and get the board-specific task configurations.

# Future Works

## Automatic Indirect Callsite Resolver

Resolve the indirect callsites automatically by some analysis.

Here is a very early buggy prototype, it compiles the EC firmware into LLVM
bitcode and do some constant propagation to resolve some indirect callsites.

<https://chromium-review.googlesource.com/c/chromiumos/platform/ec/+/652094>

It does backward tracing of indirect calls and tries to evaluate the constant
function targets by enumerating the unknown variables. Most indirect function
pointers point are derived from const global variables with some indexing
variables. By enumerating those integer variables in reasonable ranges, we can
evaluate the target of function pointers. LLVM already has lots of analysis
passes which help me do constant evaluation.

## Better Disassembly Parser

Currently we use objdump to solve code discovery and disassembly. Then use the
regular expression to parse the instruction texts from objdump. If the parser
misses some formats of instruction texts, we will miss some instructions which
could change the stack frame or do function call.

There is a idea that combine the objdump and the
[Capstone](https://github.com/aquynh/capstone) library. We use objdump to do
code discovery and get the instruction texts of each function body, then use
Capstone to disassemble the address of each instruction texts. So we can get the
structural disassembled instruction from the Capstone, and get rid of parsing
each instruction text by ourself.

## Stack Usage of Interrupt Handler

When interrupt happens, the interrupt handler will also use a fixed allocated
stack. So the maximum possible stack usages of interrupt handlers should be
checked too.
