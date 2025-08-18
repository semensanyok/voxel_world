#include <threadn>
#include <atomic>
#include <vulkan/vulkan_core.h>

// A single Vulkan device object.
extern VkDevice dev;

// Three independent Vulkan queues from VkDevice <dev>.
extern VkQueue queue1;
extern VkQueue queue2;
extern VkQueue queue3;
extern VkQueue queue4;

// Swapchain data
extern VkSwapchainKHR swapchain;
extern uint32_t swapchainImageIndex;

// One timeline semaphore object.
VkSemaphore timeline;

// One binary semaphore object for vkQueuePresent()
VkSemaphore binary;

// Track signal submission count.
std::atomic_uint64_t submitCount(0);

static void thread1()
{
  const uint64_t waitValue1 = 0; // No-op wait. Value is always >= 0.
  const uint64_t signalValue1 = 5; // Unblock thread2's CPU work.

  VkTimelineSemaphoreSubmitInfo timelineInfo1;
  timelineInfo1.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  timelineInfo1.pNext = NULL;
  timelineInfo1.waitSemaphoreValueCount = 1;
  timelineInfo1.pWaitSemaphoreValues = &waitValue1;
  timelineInfo1.signalSemaphoreValueCount = 1;
  timelineInfo1.pSignalSemaphoreValues = &signalValue1;

  VkSubmitInfo info1;
  info1.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  info1.pNext = &timelineInfo1;
  info1.waitSemaphoreCount = 1;
  info1.pWaitSemaphores = &timeline;
  info1.signalSemaphoreCount  = 1;
  info1.pSignalSemaphores = &timeline;
  // ... Enqueue initial device work here.
  info1.commandBufferCount = 0;
  info1.pCommandBuffers = 0;

  vkQueueSubmit(queue1, 1, &info1, VK_NULL_HANDLE);
  submitCount++;
}

static void thread2()
{
  // Wait for thread1's device work to complete.
  const uint64_t waitValue2 = 4;

  VkSemaphoreWaitInfo waitInfo;
  waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
  waitInfo.pNext = NULL;
  waitInfo.flags = 0;
  waitInfo.semaphoreCount = 1;
  waitInfo.pSemaphores = &timeline;
  waitInfo.pValues = &waitValue2;

  vkWaitSemaphores(dev, &waitInfo, UINT64_MAX);

  // ... Perform some CPU work dependent on thread1's device work here.

  // Unblock thread3's device work.
  VkSemaphoreSignalInfo signalInfo;
  signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
  signalInfo.pNext = NULL;
  signalInfo.semaphore = timeline;
  signalInfo.value = 7;

  vkSignalSemaphore(dev, &signalInfo);
  submitCount++;
}

static void thread3()
{
  const uint64_t waitValue3 = 7; // Wait for thread2's CPU work to complete.
  const uint64_t signalValue3 = 8; // Signal completion of pre-present work.

  const uint64_t signalSemaphoreValues[2] = {
     signalValue3, // value for "timeline"
     0 // ignored, binary semaphore.
  };

  VkTimelineSemaphoreSubmitInfo timelineInfo3;
  timelineInfo3.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  timelineInfo3.pNext = NULL;
  timelineInfo3.waitSemaphoreValueCount = 1;
  timelineInfo3.pWaitSemaphoreValues = &waitValue3;
  timelineInfo3.signalSemaphoreValueCount = 2;
  timelineInfo3.pSignalSemaphoreValues = signalSemaphoreValues;

 const VkSemaphore signalSemaphores[2] = {
    timeline, // Track timeline semaphore work completion
    binary// Unblock presentation
  };

  VkSubmitInfo info3;
  info3.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  info3.pNext = &timelineInfo3;
  info3.waitSemaphoreCount = 1;
  info3.pWaitSemaphores = &timeline;
  info3.signalSemaphoreCount  = 2;
  info3.pSignalSemaphores = signalSemaphores;
  // ... Enqueue device work dependent on thread2's CPU work here.
  info3.commandBufferCount = 0;
  info3.pCommandBuffers = 0;

  vkQueueSubmit(queue3, 1, &info3, VK_NULL_HANDLE);
  submitCount++;
}

int main()
{
  // Create the timeline semaphore object
  VkSemaphoreTypeCreateInfo timelineCreateInfo;
  timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
  timelineCreateInfo.pNext = NULL;
  timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
  timelineCreateInfo.initialValue = 0;

  VkSemaphoreCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  createInfo.pNext = &timelineCreateInfo;
  createInfo.flags = 0;

  vkCreateSemaphore(dev, &createInfo, NULL, &timeline);

  // Create the binary semaphore object
  VkSemaphoreCreateInfo binaryCreateInfo;
  binaryCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  binaryCreateInfo.pNext = NULL;
  binaryCreateInfo.flags = 0;

  vkCreateSemaphore(dev, &binaryCreateInfo, NULL, &binary);

  // Spawn three free-running CPU threads using the timeline semaphore
  std::thread t1(thread1);
  std::thread t2(thread2);
  std::thread t3(thread3);

  // Wait for submission of all dependencies of the binary semaphore
  while (submitCount < 3);

  // Present results
  VkPresentInfoKHR presentInfo;
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = NULL;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &binary;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &swapchainImageIndex;
  presentInfo.pResults = NULL;
  vkQueuePresentKHR(queue4, &presentInfo);

  // Wait for the device and CPU work using the timeline semaphore to idle
  const uint64_t waitValue = 8;

  VkSemaphoreWaitInfo waitInfo;
  waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
  waitInfo.pNext = NULL;
  waitInfo.flags = 0;
  waitInfo.semaphoreCount = 1;
  waitInfo.pSemaphores = &timeline;
  waitInfo.pValues = &waitValue;

  vkWaitSemaphores(dev, &waitInfo, UINT64_MAX);

  // Destroy the timeline semaphore object
  vkDestroySemaphore(dev, timeline, NULL);

  // Clean up the CPU threads.
  t3.join();
  t2.join();
  t1.join();

  return 0;
}
