// Copyright 2019 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_DAWN_NATIVE_VULKAN_EXTERNAL_SEMAPHORE_SEMAPHORESERVICE_H_
#define SRC_DAWN_NATIVE_VULKAN_EXTERNAL_SEMAPHORE_SEMAPHORESERVICE_H_

#include <memory>

#include "dawn/native/Error.h"
#include "dawn/native/vulkan/ExternalHandle.h"

namespace dawn::native::vulkan {
class Device;
struct VulkanDeviceInfo;
struct VulkanFunctions;
}  // namespace dawn::native::vulkan

namespace dawn::native::vulkan::external_semaphore {
class ServiceImplementation;

class Service {
  public:
    explicit Service(Device* device);
    ~Service();

    static bool CheckSupport(const VulkanDeviceInfo& deviceInfo,
                             VkPhysicalDevice physicalDevice,
                             const VulkanFunctions& fn);
    void CloseHandle(ExternalSemaphoreHandle handle);

    // True if the device reports it supports this feature
    bool Supported();

    // Given an external handle, import it into a VkSemaphore
    ResultOrError<VkSemaphore> ImportSemaphore(ExternalSemaphoreHandle handle);

    // Create a VkSemaphore that is exportable into an external handle later
    ResultOrError<VkSemaphore> CreateExportableSemaphore();

    // Export a VkSemaphore into an external handle
    ResultOrError<ExternalSemaphoreHandle> ExportSemaphore(VkSemaphore semaphore);

    // Duplicate a new external handle from the given one.
    ExternalSemaphoreHandle DuplicateHandle(ExternalSemaphoreHandle handle);

  private:
    // Linux, ChromeOS and Android use FD semaphore and Fuchia uses ZirconHandle.
    std::unique_ptr<ServiceImplementation> mServiceImpl;
};

}  // namespace dawn::native::vulkan::external_semaphore

#endif  // SRC_DAWN_NATIVE_VULKAN_EXTERNAL_SEMAPHORE_SEMAPHORESERVICE_H_
