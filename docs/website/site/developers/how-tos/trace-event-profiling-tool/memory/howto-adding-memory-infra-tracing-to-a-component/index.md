---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/trace-event-profiling-tool
  - The Trace Event Profiling Tool (about:tracing)
- - /developers/how-tos/trace-event-profiling-tool/memory
  - OBSOLETE. Memory profiling in chrome://tracing
page_name: howto-adding-memory-infra-tracing-to-a-component
title: 'HowTo: Adding Memory Infra Tracing to a Component'
---

## Motivation

If you have a component that manages memory allocations, you should be
registering and tracking those allocations with Chrome's memory-infra system.
This lets you:

*   See an overview of your allocations, giving insight into total size
            and breakdown.
*   Understand how your allocations change over time and how they are
            impacted by other parts of Chrome.
*   Catch regressions in your component's allocations size by setting up
            telemetry tests which monitor your allocation sizes under certain
            circumstances.

Some existing components which make use of memory tracing infra:

*   Discardable Memory - Tracks usage of discardable memory throughout
            chrome.
*   GPU - tracks GL and other GPU object allocations.
*   V8 - tracks heap size for JS.
*   many more....

## Overview

In order to hook into Chrome's memory infra system, your component needs to do
two things:

1.  Create a
            [base::trace_event::MemoryDumpProvider](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_provider.h)
            for your component.
2.  Register/Unregister your
            [base::trace_event::MemoryDumpProvider](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_provider.h)
            with the
            [base::trace_event::MemoryDumpManager](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_manager.h).

## Creating a MemoryDumpProvider

In order to tie into the memory infra system, you must write a
[base::trace_event::MemoryDumpProvider](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_provider.h)
for your component. You can implement this as a stand-alone class, or as an
additional interface on an existing class. For example, this interface is
frequently implemented on classes which manage a pool of allocations (see
[cc::ResourcePool](https://code.google.com/p/chromium/codesearch#chromium/src/cc/resources/resource_pool.h)
for an example).

A MemoryDumpProvider has one basic job, to implement
[MemoryDumpProvider::OnMemoryDump](https://code.google.com/p/chromium/codesearch#chromium/src/skia/ext/skia_memory_dump_provider.h).
This function is responsible for iterating over the resources allocated/tracked
by your component, and creating a
[base::trace_event::MemoryAllocatorDump](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_allocator_dump.h)
for each using
[ProcessMemoryDump::CreateAllocatorDump](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/process_memory_dump.h).
Here is a simple example:

> <table>
> <tr>
> <td>bool MyComponent::OnMemoryDump(</td>
> <td> const base::trace_event::MemoryDumpArgs& args,</td>
> <td> base::trace_event::ProcessMemoryDump\* process_memory_dump) {</td>
> <td> for (const auto& allocation : my_allocations_) {</td>
> <td> auto\* dump = process_memory_dump-&gt;CreateAllocatorDump(</td>
> <td> "path/to/my/component/allocation_" + allocation.id().ToString());</td>
> <td> dump-&gt;AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,</td>
> <td> base::trace_event::MemoryAllocatorDump::kUnitsBytes,</td>
> <td> allocation.size_bytes());</td>
> <td> // While you will typically have a kNameSize entry, you can add additional</td>
> <td> // entries to your dump with free-form names. In this example we also dump</td>
> <td> // an object's "free_size", assuming the object may not be entirely in use.</td>
> <td> dump-&gt;AddScalar("free_size",</td>
> <td> base::trace_event::MemoryAllocatorDump::kUnitsBytes,</td>
> <td> allocation.free_size_bytes());</td>
> <td> }</td>
> <td> }</td>
> </tr>
> </table>

For many components, this may be all that is needed. See Handling Shared Memory
Allocations and Sub Allocations for information on more complex use cases.

## Registering/Unregistering a MemoryDumpProvider

### Registration

Once you have created a
[base::trace_event::MemoryDumpProvider](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_provider.h),
you need to register it with the
[base::trace_event::MemoryDumpManager](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_manager.h)
before the system can start polling it for memory information. Registration is
generally straightforward, and involves calling
[MemoryDumpManager::RegisterDumpProvider](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_manager.h):

> <table>
> <tr>
> <td>...</td>
> <td> // Each process uses a singleton MemoryDumpManager</td>
> <td> base::trace_event::MemoryDumpManager::GetInstance()-&gt;RegisterDumpProvider(</td>
> <td> my_memory_dump_provider_, my_single_thread_task_runner_);</td>
> <td>...</td>
> </tr>
> </table>

In the above code, my_memory_dump_provider_ is the
[base::trace_event::MemoryDumpProvider](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_provider.h)
outlined in the previous section. my_single_thread_task_runner_ is more complex
and may be a number of things:

*   Most commonly, if your component is always used from the main
            message loop, my_single_thread_task_runner_ may just be
            [base::ThreadTaskRunnerHandle::Get()](https://code.google.com/p/chromium/codesearch#chromium/src/base/thread_task_runner_handle.h).
*   If your component already uses a custom
            [base::SingleThreadTaskRunner](https://code.google.com/p/chromium/codesearch#chromium/src/base/single_thread_task_runner.h)
            for executing tasks on a specific thread, you should likely use this
            runner.

In all cases, if your
[base::trace_event::MemoryDumpProvider](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_provider.h)
accesses data that may be used from multiple threads, you should make sure the
proper locking is in place in your implementation of
[MemoryDumpProvider::OnMemoryDump](https://code.google.com/p/chromium/codesearch#chromium/src/skia/ext/skia_memory_dump_provider.h).

### Unregistration

Unregistering a
[base::trace_event::MemoryDumpProvider](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_provider.h)
is very similar to registering one:

> <table>
> <tr>
> <td>...</td>
> <td> // Each process uses a singleton MemoryDumpManager</td>
> <td> base::trace_event::MemoryDumpManager::GetInstance()-&gt;UnregisterDumpProvider(</td>
> <td> my_memory_dump_provider_);</td>
> <td>...</td>
> </tr>
> </table>

The main complexity here is that unregistration must happen on the thread
belonging to the
[base::SingleThreadTaskRunner](https://code.google.com/p/chromium/codesearch#chromium/src/base/single_thread_task_runner.h)
provided at registration time. Unregistering on another thread can lead to race
conditions if tracing is active when the provider is unregistered.

## Handling Shared Memory Allocations

When an allocation is shared between two components, it may be useful to dump
the allocation in both components, but you also want to avoid double-counting
the allocation. This can be achieved using memory infra's concept of "ownership
edges". An ownership edge represents that the "source" memory allocator dump
owns a "target" memory allocator dump. If multiple "source" dumps own a single
"target", then the cost of that "target" allocation will be split between the
"source"s. Additionally, importance can be added to a specific ownership edge,
allowing the highest importance "source" of that edge to claim the entire cost
of the "target".

In the typical case, you will use
[base::trace_event::ProcessMemoryDump](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/process_memory_dump.h)
to create a shared global allocator dump. This dump will act as the target of
all component-specific dumps of a specific resource:

> <table>
> <tr>
> <td>...</td>
> <td> // Component 1 is going to create a dump, source_mad, for an allocation,</td>
> <td> // alloc_, which may be shared with other components / processes.</td>
> <td> MyAllocationType\* alloc_;</td>
> <td> base::trace_event::MemoryAllocatorDump\* source_mad;</td>
> <td> // Component 1 creates and populates source_mad;</td>
> <td> ...</td>
> <td> // In addition to creating a source dump, we must create a global shared</td>
> <td> // target dump. This dump should be created with a unique GUID which can be</td>
> <td> // generated any place the allocation is used. I recommend adding a GUID</td>
> <td> // generation function to the allocation type.</td>
> <td> base::trace_event::MemoryAllocatorDumpGUID guid(alloc_-&gt;GetGUIDString());</td>
> <td> // From this GUID we can generate the parent allocator dump.</td>
> <td> base::trace_event::MemoryAllocatorDump\* target_mad =</td>
> <td> process_memory_dump-&gt;CreateSharedGlobalAllocatorDump(guid);</td>
> <td> // We now create an ownership edge from the source dump to the target dump.</td>
> <td> // When creating an edge, you can assign an importance to this edge. If all</td>
> <td> // edges have the same importance, the size of the allocation will be split</td>
> <td> // between all sources which create a dump for the allocation. If one</td>
> <td> // edge has higher importance than the others, its soruce will be assigned the</td>
> <td> // full size of the allocation.</td>
> <td> const int kImportance = 1;</td>
> <td> process_memory_dump-&gt;AddOwnershipEdge(</td>
> <td> source_mad-&gt;guid(), target_mad-&gt;guid(), kImportance);</td>
> <td>...</td>
> </tr>
> </table>

If an allocation is being shared across process boundaries, it may be useful to
generate a GUID which incorporates the ID of the local process, preventing two
processes from generating colliding GUIDs. As it is not recommended to pass a
Process ID between processes for security reasons, a function
[MemoryDumpManager::GetTracingProcessId](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_dump_manager.cc)
is provided which generates a unique ID per process that can be passed with the
resource without security concerns. Frequently this ID is used to generate a
GUID that is based on the allocated resource's ID combined with the allocating
process' tracing ID.

## Sub Allocations

Another advanced use case involves tracking sub-allocations of a larger
allocation. For instance, this is used in
[gpu::gles2::TextureManager](https://code.google.com/p/chromium/codesearch#chromium/src/gpu/command_buffer/service/texture_manager.cc)
to dump both the sub-allocations which make up a texture. Creating a sub
allocation is easy - instead of calling
[ProcessMemoryDump::CreateAllocatorDump](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/process_memory_dump.h)
to create a
[base::trace_event::MemoryAllocatorDump](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/memory_allocator_dump.h),
you simply call
[ProcessMemoryDump::AddSubAllocation](https://code.google.com/p/chromium/codesearch#chromium/src/base/trace_event/process_memory_dump.h),
providing the GUID of the parent allocation as the first parameter.
