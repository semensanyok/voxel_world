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


# so... broad phase culling -> dispatch transfer vertex / index -> frustum culling on GPU via compute shader. Or what is the best practice?

You're on the right track! Here's the modern best practice pipeline for large-scale rendering:
Optimal Culling & Rendering Pipeline
Frame N Timeline:
CPU Thread 1 (Main):     CPU Thread 2 (Async):    GPU:
├─ Broad Phase Cull      ├─ Upload Frame N+1       ├─ Frustum Cull (Compute)
├─ Prepare Upload        │  - New Vertices         ├─ Render Frame N-1  
├─ Submit GPU Work       │  - Updated Transforms   │
└─ Begin Frame N+1       └─ Stage Next Frame       └─ PresentCPU Thread 1 (Main):     CPU Thread 2 (Async):    GPU:
├─ Broad Phase Cull      ├─ Upload Frame N+1       ├─ Frustum Cull (Compute)
├─ Prepare Upload        │  - New Vertices         ├─ Render Frame N-1  
├─ Submit GPU Work       │  - Updated Transforms   │
└─ Begin Frame N+1       └─ Stage Next Frame       └─ Present

Recommended Hybrid Approach:
For Most Applications:
cvoid hybridPipeline() {
    // 1. CPU broad phase (coarse, fast)
    //    - Spatial queries (octree/BVH)  
    //    - Rough distance/angle checks
    //    - Material grouping
    broadPhaseCull();                    // ~1000s → ~100s objects
    
    // 2. Upload only broad-phase survivors
    asyncUploadVisible();                // Transfer reduced set
    
    // 3. GPU fine culling (accurate, parallel)
    //    - Per-pixel accurate frustum tests
    //    - Sub-pixel culling
    //    - Occlusion queries
    gpuFineCulling();                    // ~100s → ~50s objects
    
    // 4. Render survivors
    renderIndirect();
}


Detailed Pipeline:
1. Broad Phase Culling (CPU)
```c
c// Thread 1: Octree/spatial queries
void broadPhaseCull() {
    visibleObjects.clear();
    
    // Your octree traversal
    octree.query(cameraFrustum, [&](ObjectID obj) {
        // Coarse rejection - just AABB vs frustum planes
        if (coarseIntersect(objects[obj].worldAABB, frustum)) {
            visibleObjects.push_back(obj);
        }
    });
    
    // Sort by material for batching
    std::sort(visibleObjects.begin(), visibleObjects.end(), 
             [](const ObjectID& a, const ObjectID& b) {
                 return objects[a].materialID < objects[b].materialID;
             });
}
```
2. Async Transfer (CPU Thread 2)
```c
// Thread 2: Upload new/updated data
void asyncUpload() {
    // Upload new geometry discovered in broad phase
    for (ObjectID obj : newlyVisibleObjects) {
        if (!isGeometryUploaded(obj)) {
            stageGeometryUpload(obj);  // Queue for transfer
        }
    }
    
    // Update dynamic transforms
    updateObjectTransforms();
    
    // Submit transfer commands
    vkQueueSubmit(transferQueue, 1, &transferSubmit, transferFence);
}
```
3. GPU Fine Culling (Compute Shader)
```c
void gpuCulling() {
    // Wait for transfers to complete
    vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
    
    vkBeginCommandBuffer(computeCmd, &beginInfo);
    
    // Per-material culling passes
    for (const MaterialBatch& material : materialBatches) {
        // Reset count for this material
        vkCmdFillBuffer(computeCmd, countBuffer, 
                       material.countOffset * 4, 4, 0);
        
        // Push constants: material ID + output offset
        vkCmdPushConstants(computeCmd, cullPipelineLayout,
                          VK_SHADER_STAGE_COMPUTE_BIT, 0,
                          sizeof(CullPushConstants), &material.cullParams);
        
        // Dispatch fine culling
        vkCmdDispatch(computeCmd, (material.objectCount + 63) / 64, 1, 1);
        
        // Barrier between materials
        vkCmdPipelineBarrier(computeCmd, /* ... */);
    }
    
    vkEndCommandBuffer(computeCmd);
    vkQueueSubmit(computeQueue, 1, &computeSubmit, cullFence);
}
```