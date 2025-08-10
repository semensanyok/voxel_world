# Reasoning behind graphics engine architectural decisions

# 1.1 Aim for 15-30 command buffers and 5-10 vkQueueSubmit() calls per frame, batch VkSubmitInfo() to a single call as much as possible. Each vkQueueSubmit() has a performance cost on CPU, so lower is generally better. Note that VkSemaphore-based 
https://developer.nvidia.com/blog/vulkan-dos-donts/


#  1.2 Command buffers.

### 1.2.1 https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/performance/command_buffer_usage#allocate-and-free
dont free buffers, always reset. free is worst performancewise as stated:
Command buffers are allocated from a command pool with vkAllocateCommandBuffers. They can then be recorded and submitted to a queue for the Vulkan device to execute them.

A possible approach to managing the command buffers for each frame in our application would be to free them once they are executed, using vkFreeCommandBuffers.

The command pool will not automatically recycle memory from deleted command buffers if the command pool was created without the RESET_COMMAND_BUFFER_BIT flag. This flag however will force separate internal allocators to be used for each command buffer in the pool, which can increase CPU overhead compared to a single pool reset.

!!! This is the worst-performing method of managing command buffers as it involves a significant CPU overhead for allocating and freeing memory frequently. 

### https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/performance/command_buffer_usage#resetting-individual-command-buffers
Resetting the command pool
Resetting the pool with vkResetCommandPool automatically resets all the command buffers allocated by it. Doing this periodically will allow the pool to reuse the memory allocated for command buffers with lower CPU overhead.

To reset the pool the flag RESET_COMMAND_BUFFER_BIT is not required, and it is actually better to avoid it since it prevents it from using a single large allocator for all buffers in the pool thus increasing memory overhead.

### 1.2.1 SUMMARY

Do
- Use secondary command buffers to allow multi-threaded render pass construction.
- Minimize the number of secondary command buffer invocations used per frame.
- Set ONE_TIME_SUBMIT_BIT if you are not going to reuse the command buffer.
- Periodically call vkResetCommandPool() to release the memory if you are not reusing command buffers.

Don’t
- Set RESET_COMMAND_BUFFER_BIT if you only need to free the whole pool. If the bit is not set, some implementations might use a single large allocator for the pool, reducing memory management overhead.
- Call vkResetCommandBuffer() on a high frequency call path.

Impact
- Increased CPU load will be incurred with secondary command buffers.
- Increased CPU load will be incurred if command pool creation and command buffer begin flags are not used appropriately.
- Increased CPU overhead if command buffer resets are too frequent.
- Increased memory usage until a manual command pool reset is triggered.

Debugging
- Evaluate every use of any command buffer flag other than ONE_TIME_SUBMIT_BIT, and review whether it’s a necessary use of the flag combination.
- Evaluate every use of vkResetCommandBuffer() and see if it could be replaced with vkResetCommandPool() instead.

### 1.2.2 https://developer.nvidia.com/blog/vulkan-dos-donts/
Calling vkQueueSubmit() does start work on the GPU. 
Use a separate command pool for each thread that records command buffers, for each frame.

Reuse command buffers when possible. Secondary command buffers can be helpful here, depending on the workload – check carefully to determine if they are actually advantageous.

Use L * T + N pools. (L = the number of buffered frames, T = the number of threads that record command buffers, N = extra pools for secondary command buffers)

Don’t create or destroy command pools, reuse them instead. Save the overhead of allocator creation/destruction and memory allocation/free (###1.2.1)

Multi-threaded recording
To record command buffers concurrently, the framework needs to manage resource pools per frame and per thread. According to the Vulkan Spec:
[A command pool must not be used concurrently in multiple threads.](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkCommandPool.html)
Command pools are externally synchronized, meaning that a command pool must
not be used concurrently in multiple threads. That includes use via recording
commands on any command buffers allocated from the pool, as well as
operations that allocate, free, and reset command buffers or the pool itself.
It's not possible to record 2 command buffers from the same pool on different
threads. Create pool per thread, even for queues from same family.
[The application must not allocate and/or free descriptor sets from the same pool in multiple threads simultaneously.](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkDescriptorPool.html)
23/02/2025 
  - Create transfer workers and put them in queue. Initialize command pool, command buffer, staging buffer in main thread. 
    Only recording must be done from a thread that acquires a worker. Not necessary to postpone the initialization.
  
[ Build command buffers in parallel and evenly across several threads/cores to multiple command lists. Recording commands is a CPU intensive operation and no driver threads come to the rescue. ](https://developer.nvidia.com/blog/vulkan-dos-donts/)
[ Don’t create too many threads or too many command lists. Too many threads will oversubscribe your CPU resources, too many command lists may accumulate too much overhead. ](https://developer.nvidia.com/blog/vulkan-dos-donts/)

[ The following are the queue operations found in VkQueueFlagBits: ](https://docs.vulkan.org/guide/latest/queues.html#_queue_family)
- VK_QUEUE_GRAPHICS_BIT used for vkCmdDraw* and graphic pipeline commands.
- VK_QUEUE_COMPUTE_BIT used for vkCmdDispatch* and vkCmdTraceRays* and compute pipeline related commands.
- VK_QUEUE_TRANSFER_BIT used for all transfer commands.
- VK_PIPELINE_STAGE_TRANSFER_BIT in the Spec describes “transfer commands”.
Queue Families with only VK_QUEUE_TRANSFER_BIT are usually for using DMA to asynchronously transfer data between host and device memory on discrete GPUs, so transfers can be done concurrently with independent graphics/compute operations.
- VK_QUEUE_GRAPHICS_BIT and VK_QUEUE_COMPUTE_BIT can always implicitly accept VK_QUEUE_TRANSFER_BIT commands.
- VK_QUEUE_SPARSE_BINDING_BIT used for binding sparse resources to memory with vkQueueBindSparse.
- VK_QUEUE_PROTECTED_BIT used for protected memory.
- VK_QUEUE_VIDEO_DECODE_BIT_KHR and VK_QUEUE_VIDEO_ENCODE_BIT_KHR used with Vulkan Video.

[ Without DMA, when the CPU is using programmed input/output, it is typically fully occupied for the entire duration of the read or write operation, and is thus unavailable to perform other work. With DMA, the CPU first initiates the transfer, then it does other operations while the transfer is in progress, and it finally receives an interrupt from the DMA controller (DMAC) when the operation is done. This feature is useful at any time that the CPU cannot keep up with the rate of data transfer, or when the CPU needs to perform work while waiting for a relatively slow I/O data transfer.  ](https://en.wikipedia.org/wiki/Direct_memory_access)

### You can only submit work to a VkQueue from one thread at a time, but different threads can submit work to a different VkQueue simultaneously. https://community.khronos.org/t/disadvantages-of-using-multiple-command-buffers-with-one-thread/107399/3
// You cannot submit to the same queue from different threads at the same time. Therefore, if you have multiple threads building CBs, they must either synchronize their vkQueueSubmit calls or send their CBs synchronously to a thread that does a single vkQueueSubmit call.
[ A “Queue Family” just describes a set of VkQueue's that have common properties and support the same functionality ](https://docs.vulkan.org/guide/latest/queues.html). So multithread access to same family is permitted, as family is just a label. And dedicated DMA transfer queues reside under family with only VK_QUEUE_TRANSFER_BIT;

My note: for transfer queue, spin iterate until acquire queue with CAS. Better than single thread submit of per thread lists (need to block each, and not using other 10 HW transfer/compute dedicated queues). GPU exposes many transfer queues.

11/01/2025 
Lock shared transfer queue on submit. Otherwise can have thread with queue, polling other threads prepared commands arrays once per frame. First option is implemented in Godot engine - shared queue, locked on submit.

# 1.3 DEVICE_LOCAL HOST_VISIBLE_BIT etc.

### 1.3.1 https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/
VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT – this is generally referring to GPU memory that is not directly visible from CPU; it’s fastest to access from the GPU and this is the memory you should be using to store all render targets, GPU-only resources such as buffers for compute, and also all static resources such as textures and geometry buffers.
VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT – on AMD hardware, this memory type refers to up to 256 MB of video memory that the CPU can write to directly, and is perfect for allocating reasonable amounts of data that is written by CPU every frame, such as uniform buffers or dynamic vertex/index buffers
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT2 – this is referring to CPU memory that is directly visible from GPU; reads from this memory go over PCI-express bus. In absence of the previous memory type, this generally speaking should be the choice for uniform buffers or dynamic vertex/index buffers, and also should be used to store staging buffers that are used to populate static resources allocated with VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT with data.

When dealing with dynamic resources, in general allocating in non-device-local host-visible memory works well – it simplifies the application management and is efficient due to GPU-side caching of read-only data. For resources that have a high degree of random access though, like dynamic textures, it’s better to allocate them in VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT and upload data using staging buffers allocated in VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT memory – similarly to how you would handle static textures. In some cases you might need to do this for buffers as well – while uniform buffers typically don’t suffer from this, in some applications using large storage buffers with highly random access patterns will generate too many PCIe transactions unless you copy the buffers to GPU first; additionally, host memory does have higher access latency from the GPU side that can impact performance for many small draw calls.

