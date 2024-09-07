# some long term considerations not present in source comments

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
!!! Use a separate command pool for each thread that records command buffers, for each frame.

Reuse command buffers when possible. Secondary command buffers can be helpful here, depending on the workload – check carefully to determine if they are actually advantageous.

Use L * T + N pools. (L = the number of buffered frames, T = the number of threads that record command buffers, N = extra pools for secondary command buffers)

Don’t create or destroy command pools, reuse them instead. Save the overhead of allocator creation/destruction and memory allocation/free (###1.2.1)


# 1.3 DEVICE_LOCAL HOST_VISIBLE_BIT etc.

### 1.3.1 https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/
VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT – this is generally referring to GPU memory that is not directly visible from CPU; it’s fastest to access from the GPU and this is the memory you should be using to store all render targets, GPU-only resources such as buffers for compute, and also all static resources such as textures and geometry buffers.
VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT – on AMD hardware, this memory type refers to up to 256 MB of video memory that the CPU can write to directly, and is perfect for allocating reasonable amounts of data that is written by CPU every frame, such as uniform buffers or dynamic vertex/index buffers
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT2 – this is referring to CPU memory that is directly visible from GPU; reads from this memory go over PCI-express bus. In absence of the previous memory type, this generally speaking should be the choice for uniform buffers or dynamic vertex/index buffers, and also should be used to store staging buffers that are used to populate static resources allocated with VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT with data.

When dealing with dynamic resources, in general allocating in non-device-local host-visible memory works well – it simplifies the application management and is efficient due to GPU-side caching of read-only data. For resources that have a high degree of random access though, like dynamic textures, it’s better to allocate them in VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT and upload data using staging buffers allocated in VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT memory – similarly to how you would handle static textures. In some cases you might need to do this for buffers as well – while uniform buffers typically don’t suffer from this, in some applications using large storage buffers with highly random access patterns will generate too many PCIe transactions unless you copy the buffers to GPU first; additionally, host memory does have higher access latency from the GPU side that can impact performance for many small draw calls.


