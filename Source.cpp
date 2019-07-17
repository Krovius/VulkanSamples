#include <windows.h>
#include <vulkan.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

bool find_heap(VkPhysicalDevice hardware, const VkMemoryRequirements* requirements, VkMemoryPropertyFlags flags, uint32_t* heap)
{
	VkPhysicalDeviceMemoryProperties prop;
	vkGetPhysicalDeviceMemoryProperties(hardware, &prop);

	for (uint32_t i = 0; i < prop.memoryTypeCount; ++i)
	{
		if ((requirements->memoryTypeBits >> i) & 1)
		{
			if ((prop.memoryTypes[i].propertyFlags & flags) == flags)
			{
				if (heap)
				{
					*heap = i;
				}
				return true;
			}
		}
	}
	return false;
}

int main(int argc, const char** argv)
{
	static const char appName[] = "Test app";
	static const char engineName[] = "Test engine";
	
	const uint32_t hardware_id = 0;
	const VkDeviceSize transfer_size = 16 << 20; // 16MB

	VkApplicationInfo app_info = {
		VK_STRUCTURE_TYPE_APPLICATION_INFO,		// sType
		nullptr,								// pNext
		appName,								// pApplicationName
		VK_MAKE_VERSION(1, 0, 0),				// applicationVersion
		engineName,								// pEngineName
		VK_MAKE_VERSION(1, 0, 0),				// engineVersion
		VK_API_VERSION_1_0,						// apiVersion
	};

	VkInstanceCreateInfo instance_info = {
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,					// sType
		nullptr,												// pNext <-- &debug_info
		0,														// flags
		&app_info,												// pApplicationInfo
		0,														// enabledLayerCount
		nullptr,												// ppEnabledLayerNames
		0,														// enabledExtensionCount
		nullptr,													// ppEnabledExtensionNames
	};

	VkInstance instance;
	assert(vkCreateInstance(&instance_info, nullptr, &instance) == VK_SUCCESS);

	uint32_t hardware_count = 0;	
	assert(vkEnumeratePhysicalDevices(instance, &hardware_count, nullptr) == VK_SUCCESS);
	VkPhysicalDevice* hardware = static_cast<VkPhysicalDevice*>(malloc(sizeof(VkPhysicalDevice) * hardware_count + 1));
	assert(vkEnumeratePhysicalDevices(instance, &hardware_count, hardware) == VK_SUCCESS);
	assert(hardware_id < hardware_count && "Hardware index not available");

	uint32_t family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(hardware[hardware_id], &family_count, nullptr);
	VkQueueFamilyProperties* family_prop = static_cast<VkQueueFamilyProperties*>(malloc(sizeof(VkQueueFamilyProperties) * family_count + 1));
	vkGetPhysicalDeviceQueueFamilyProperties(hardware[hardware_id], &family_count, family_prop);

	uint32_t compute_family = family_count;
	uint32_t transfer_family = family_count;

	const float priority[] = { 1.f };
	VkDeviceQueueCreateInfo* queue_info = static_cast<VkDeviceQueueCreateInfo*>(malloc(sizeof(VkDeviceQueueCreateInfo) * family_count + 1));
	for (uint32_t i = 0; i < family_count; i++)
	{
		if (family_prop[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
		{
			if (i < compute_family)
			{
				compute_family = i;
			}
		}
		else if (family_prop[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			transfer_family = i;
		}

		queue_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info[i].pNext = nullptr;
		queue_info[i].flags = 0;
		queue_info[i].queueFamilyIndex = i;
		queue_info[i].queueCount = 1;
		queue_info[i].pQueuePriorities = priority;
	}

	assert(compute_family < family_count && "No compute queue family");
	assert(transfer_family < family_count && "No transfer-only queue family");

	VkDeviceCreateInfo device_info = {
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		nullptr,
		0,
		family_count,
		queue_info,
		0,
		nullptr,
		0,
		nullptr,
		nullptr,
	};

	VkDevice device;
	assert(vkCreateDevice(hardware[hardware_id], &device_info, nullptr, &device) == VK_SUCCESS);
	
	VkCommandPool* cmd_pool = static_cast<VkCommandPool*>(malloc(sizeof(VkCommandPool) * family_count + 1));
	VkCommandBuffer* cmd_buf = static_cast<VkCommandBuffer*>(malloc(sizeof(VkCommandBuffer) * family_count + 1));
	VkQueryPool* query_pool = static_cast<VkQueryPool*>(malloc(sizeof(VkQueryPool) * family_count + 1));
	for (uint32_t i = 0; i < family_count; i++)
	{
		VkCommandPoolCreateInfo cpi = {
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			i,
		};

		assert(vkCreateCommandPool(device, &cpi, nullptr, cmd_pool + i) == VK_SUCCESS);

		VkCommandBufferAllocateInfo cbi = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			cmd_pool[i],
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1,
		};

		assert(vkAllocateCommandBuffers(device, &cbi, &cmd_buf[i]) == VK_SUCCESS);

		VkQueryPoolCreateInfo query_info = {
			VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
			nullptr,
			0,
			VK_QUERY_TYPE_TIMESTAMP,
			2,
			0,
		};
		assert(vkCreateQueryPool(device, &query_info, nullptr, &query_pool[i]) == VK_SUCCESS);
	}
		
	VkBufferCreateInfo buffer_info[] = {
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			nullptr,
			0,
			transfer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
		},
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			nullptr,
			0,
			transfer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
		}
	};

	VkBuffer buf[2];
	assert(vkCreateBuffer(device, &buffer_info[0], nullptr, &buf[0]) == VK_SUCCESS);
	assert(vkCreateBuffer(device, &buffer_info[1], nullptr, &buf[1]) == VK_SUCCESS);

	VkMemoryRequirements req[2];
	vkGetBufferMemoryRequirements(device, buf[0], &req[0]);
	vkGetBufferMemoryRequirements(device, buf[1], &req[1]);

	uint32_t host_visible;
	assert(find_heap(hardware[hardware_id], &req[0], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &host_visible));
	uint32_t device_local;
	assert(find_heap(hardware[hardware_id], &req[1], VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &device_local));

	VkMemoryAllocateInfo alloc_info[] = {
		{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			transfer_size,
			host_visible,
		},
		{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			nullptr,
			transfer_size,
			device_local,
		}
	};
	
	VkDeviceMemory mem[2];
	assert(vkAllocateMemory(device, &alloc_info[0], nullptr, &mem[0]) == VK_SUCCESS);
	assert(vkBindBufferMemory(device, buf[0], mem[0], 0) == VK_SUCCESS);

	assert(vkAllocateMemory(device, &alloc_info[1], nullptr, &mem[1]) == VK_SUCCESS);
	assert(vkBindBufferMemory(device, buf[1], mem[1], 0) == VK_SUCCESS);

	const uint32_t families[2] = { compute_family, transfer_family };
	VkQueue queues[2];

	for (uint32_t i = 0; i < 2; i++)
	{		
		VkCommandBuffer cb = cmd_buf[families[i]];

		VkCommandBufferBeginInfo begin_info = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			nullptr,
		};
		assert(vkBeginCommandBuffer(cb, &begin_info) == VK_SUCCESS);

		vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool[families[i]], 0);

		VkBufferCopy buf_copy = {
			0,
			0,
			transfer_size,
		};
		vkCmdCopyBuffer(cb, buf[0], buf[1], 1, &buf_copy);

		vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool[families[i]], 1);

		assert(vkEndCommandBuffer(cb) == VK_SUCCESS);

		vkGetDeviceQueue(device, families[i], 0, &queues[i]);
	}

	VkSubmitInfo submit_info[] = {
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0,
			nullptr,
			nullptr,
			1,
			&cmd_buf[families[0]],
			0,
			nullptr,
		},
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0,
			nullptr,
			nullptr,
			1,
			&cmd_buf[families[1]],
			0,
			nullptr,
		},
	};

	VkQueue queue[2];

	assert(vkQueueSubmit(queues[0], 1, &submit_info[0], VK_NULL_HANDLE) == VK_SUCCESS);
	assert(vkQueueWaitIdle(queues[0]) == VK_SUCCESS);

	assert(vkQueueSubmit(queues[1], 1, &submit_info[1], VK_NULL_HANDLE) == VK_SUCCESS);
	assert(vkQueueWaitIdle(queues[1]) == VK_SUCCESS);

	size_t timings[4];
	assert(vkGetQueryPoolResults(device, query_pool[families[0]], 0, 2, sizeof(size_t) * 2, timings + 0, sizeof(size_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) == VK_SUCCESS);
	assert(vkGetQueryPoolResults(device, query_pool[families[1]], 0, 2, sizeof(size_t) * 2, timings + 2, sizeof(size_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) == VK_SUCCESS);

	for (uint32_t i = 0; i < 4; i++)
	{
		if ((i & 1) == 0) {
			printf("Queue family %u\n", families[i >> 1]);
		}
		printf("  %llu = %llu\n", i, timings[i]);
	}
	
	getchar();
	return 0;
}