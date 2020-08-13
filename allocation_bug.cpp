#include <Windows.h>
#include <vulkan.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int, const char**)
{
	VkInstanceCreateInfo header1 = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

	VkInstance instance;
	assert(vkCreateInstance(&header1, nullptr, &instance) == VK_SUCCESS);

	uint32_t device_count = 0;
	assert(vkEnumeratePhysicalDevices(instance, &device_count, nullptr) == VK_SUCCESS);
	assert(device_count > 0 && "Must have at least one device.");
	
	VkPhysicalDevice* device_list = static_cast<VkPhysicalDevice*>(malloc(sizeof(VkPhysicalDevice) * device_count));
	assert(vkEnumeratePhysicalDevices(instance, &device_count, device_list) == VK_SUCCESS);


	const float priority[1] = { 1.0f };
	VkDeviceQueueCreateInfo header2 = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	header2.queueCount = 1;
	header2.pQueuePriorities = priority;

	VkDeviceCreateInfo header3 = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	header3.queueCreateInfoCount = 1;
	header3.pQueueCreateInfos = &header2;

	VkDevice device;
	assert(vkCreateDevice(device_list[0], &header3, nullptr, &device) == VK_SUCCESS);

	const VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1 },
	};

	VkDescriptorPoolCreateInfo header4 = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	header4.maxSets = 16;
	header4.poolSizeCount = 1;
	header4.pPoolSizes = poolSizes;

	VkDescriptorPool dpool;
	assert(vkCreateDescriptorPool(device, &header4, nullptr, &dpool) == VK_SUCCESS);

	const VkDescriptorSetLayoutBinding bindings[] = {
		{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT }
	};

	VkDescriptorSetLayoutCreateInfo header5 = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	header5.bindingCount = 1;
	header5.pBindings = bindings;
	
	VkDescriptorSetLayout layout;
	assert(vkCreateDescriptorSetLayout(device, &header5, nullptr, &layout) == VK_SUCCESS);

	VkDescriptorSetAllocateInfo header6 = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	header6.descriptorPool = dpool;
	header6.descriptorSetCount = 1;
	header6.pSetLayouts = &layout;

	VkDescriptorSet sets[1] = {};
	assert(vkAllocateDescriptorSets(device, &header6, sets) == VK_SUCCESS);

	printf("set[0] = %p\n", sets[0]);

	return 0;
}
