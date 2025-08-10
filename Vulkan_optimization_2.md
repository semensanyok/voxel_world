# 2025 updates. QnA

https://developer.nvidia.com/blog/vulkan-dos-donts/#work_submission
If command recording on the CPU is heavy, aggressive queue submission batching may result in additional latency penalty, while performance may remain the same. If latency is important for your application consider submitting GPU workloads earlier.
Don’t wait for a queue submission to finish, continue preparing the next queue submission or start preparing the next frame to avoid serialization
https://claude.ai/share/ce625bb0-5d12-4310-bd3f-934234d1cb66

# Claude

https://developer.nvidia.com/blog/vulkan-dos-donts/#work_submissionshould I submit every transfer ASAP and vkWaitForFences immediately?Fetched Tips and Tricks: Vulkan Dos and Don’ts | NVIDIA Technical Blognvidia.com
Based on the NVIDIA Vulkan guide, you should NOT submit every transfer ASAP and wait immediately. Here's what they recommend:
From the guide - Work Submission Don'ts:
"Don't wait for a queue submission to finish, continue preparing the next queue submission or start preparing the next frame to avoid serialization" Tips and Tricks: Vulkan Dos and Don’ts | NVIDIA Technical Blog
"Don't submit a small amount of GPU work. If a queue submission is processed on the GPU faster than new ones can be submitted on the CPU, it will result in wasted / idle GPU cycles." Tips and Tricks: Vulkan Dos and Don’ts | NVIDIA Technical Blog
Better approach for transfers:
1. Batch transfers when possible:
```cpp
// Collect multiple transfers, submit as batch
std::vector<VkBufferCopy> copyRegions;
// ... accumulate transfers
vkCmdCopyBuffer(transferCmd, stagingBuffer, deviceBuffer, 
                copyRegions.size(), copyRegions.data());
vkQueueSubmit(transferQueue, 1, &submitInfo, batchFence);
// Don't wait here - continue with other work`
```
2. Use your multi-queue approach:
```cpp
// Submit to available queue, don't wait
TransferQueue* tq = GetAvailableTransferQueue();
if (tq) {
    vkQueueSubmit(tq->queue, 1, &submitInfo, fence);
    // Continue with other work, check fence later
} else {
    // Queue work or wait for available queue
}
```
3. Check completion asynchronously:
```cpp
// Later, when you need the result or resources back
if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
    // Transfer complete, can reuse staging buffer
    ReleaseTransferQueue(tq);
}
```


# Godot uses a single transfer queue. Is having 2 transfer queues is overengineering? 1 is enough? With lock on submit. So pool per thread, and when they finish record - lock submit.

You're absolutely right to question this! For most applications, 1 transfer queue with proper pooling is likely sufficient and much simpler.
Why single queue often works well:
1. Transfer bandwidth is the bottleneck, not queue contention:
cpp// Transfer speed is limited by memory bandwidth, not queue submissions
// 2 queues don't help if you're already saturating the bus
2. Transfers are fast - lock contention is minimal:
cpp// Thread 1: Record commands → brief lock → submit → release lock
// Thread 2: Record commands → brief lock → submit → release lock
// Lock held for microseconds, not milliseconds
3. Simpler resource management:
```cpp
```
```cpp
struct TransferContext {
    VkCommandPool commandPool;     // Per-thread
    VkCommandBuffer commandBuffer;
    // No need to track "which queue am I using?"
};
// Global
VkQueue transferQueue;             // Single queue
std::mutex transferMutex;          // Simple lock
Godot's approach (simplified):
cpp// Each thread has its own command pool/buffer
thread_local TransferContext ctx;

void SubmitTransfer() {
    // Record commands (parallel, no lock needed)
    vkBeginCommandBuffer(ctx.commandBuffer, &beginInfo);
    vkCmdCopyBuffer(ctx.commandBuffer, src, dst, 1, &copyRegion);
    vkEndCommandBuffer(ctx.commandBuffer);
    
    // Submit (brief serialization)
    {
        std::lock_guard lock(transferMutex);
        vkQueueSubmit(transferQueue, 1, &submitInfo, fence);
    }
}
```

When multiple transfer queues help:

Very high transfer frequency (hundreds per frame)
Hardware has multiple DMA engines (rare)
Large team already built the complexity

Your lock-free approach has value IF:

You're already hitting transfer queue contention (measure first!)
You're building a high-performance engine from scratch
The complexity cost is acceptable

Recommendation: Start with Godot's simpler approach. Profile transfer queue contention. Only add multiple queues if you can measure the bottleneck. The NVIDIA guide emphasizes batching over multiple queues anyway.
