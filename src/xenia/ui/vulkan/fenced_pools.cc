/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2016 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/ui/vulkan/fenced_pools.h"

#include "xenia/base/assert.h"
#include "xenia/base/math.h"
#include "xenia/ui/vulkan/vulkan_util.h"

namespace xe {
namespace ui {
namespace vulkan {

using xe::ui::vulkan::CheckResult;

CommandBufferPool::CommandBufferPool(VkDevice device,
                                     uint32_t queue_family_index,
                                     VkCommandBufferLevel level)
    : BaseFencedPool(device), level_(level) {
  // Create the pool used for allocating buffers.
  // They are marked as transient (short-lived) and cycled frequently.
  VkCommandPoolCreateInfo cmd_pool_info;
  cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmd_pool_info.pNext = nullptr;
  cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cmd_pool_info.queueFamilyIndex = queue_family_index;
  auto err =
      vkCreateCommandPool(device_, &cmd_pool_info, nullptr, &command_pool_);
  CheckResult(err, "vkCreateCommandPool");

  // Allocate a bunch of command buffers to start.
  constexpr uint32_t kDefaultCount = 32;
  VkCommandBufferAllocateInfo command_buffer_info;
  command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_info.pNext = nullptr;
  command_buffer_info.commandPool = command_pool_;
  command_buffer_info.level = level;
  command_buffer_info.commandBufferCount = kDefaultCount;
  VkCommandBuffer command_buffers[kDefaultCount];
  err =
      vkAllocateCommandBuffers(device_, &command_buffer_info, command_buffers);
  CheckResult(err, "vkCreateCommandBuffer");
  for (size_t i = 0; i < xe::countof(command_buffers); ++i) {
    PushEntry(command_buffers[i]);
  }
}

CommandBufferPool::~CommandBufferPool() {
  FreeAllEntries();
  vkDestroyCommandPool(device_, command_pool_, nullptr);
  command_pool_ = nullptr;
}

VkCommandBuffer CommandBufferPool::AllocateEntry() {
  // TODO(benvanik): allocate a bunch at once?
  VkCommandBufferAllocateInfo command_buffer_info;
  command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_info.pNext = nullptr;
  command_buffer_info.commandPool = command_pool_;
  command_buffer_info.level = level_;
  command_buffer_info.commandBufferCount = 1;
  VkCommandBuffer command_buffer;
  auto err =
      vkAllocateCommandBuffers(device_, &command_buffer_info, &command_buffer);
  CheckResult(err, "vkCreateCommandBuffer");
  return command_buffer;
}

void CommandBufferPool::FreeEntry(VkCommandBuffer handle) {
  vkFreeCommandBuffers(device_, command_pool_, 1, &handle);
}

}  // namespace vulkan
}  // namespace ui
}  // namespace xe
