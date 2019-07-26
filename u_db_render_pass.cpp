#include "u_db_render_pass.hpp"

u_vk_render_pass* u_db_render_pass_implementation::create_instance(int index)
{
    static const VkAttachmentDescription attachments[2] = {
        {
            0,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        },
        {
            0,
            VK_FORMAT_D16_UNORM,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        },
    };

    static const VkSubpassDependency dependencies[1] = {
        {
            VK_SUBPASS_EXTERNAL,
            0,
            0,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            0
        },
    };

    static const VkAttachmentReference color_ref[1] = {
        { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
    };

    static const VkAttachmentReference depth_ref[1] = {
        { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
    };

    static const VkSubpassDescription passes[1] = {
        {
            0,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            0,
            nullptr,
            1,
            color_ref + 0,
            nullptr,
            depth_ref + 0,
            0,
            nullptr,
        },
    };

    static const VkRenderPassCreateInfo header[1] = {
        {
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            nullptr,
            0,
            2,
            attachments + 0,
            1,
            passes + 0,
            1,
            dependencies + 0,
        },
    };

    VkRenderPass handle;
    assert(device->CreateRenderPass(device->get_handle(), header + index, nullptr, &handle) == VK_SUCCESS);

    device->grab();
    u_vk_render_pass_implementation* obj = new u_vk_render_pass_implementation;
    obj->handle = handle;
    obj->device = device;
    return obj;
}

u_db_render_pass* u_db_render_pass::create(u_vk_device* device)
{
    assert(device->CreateComputePipelines);
    assert(device->CmdBindPipeline);
    assert(device->DestroyPipeline);

    device->grab();
    u_db_render_pass_implementation* obj = new u_db_render_pass_implementation;
    obj->device = device;
    obj->items.resize(1);
    obj->items.zero();
    obj->db.insert(0xf0a2d8d14769f172llu, 0); // renderpass/rp1
    return obj;
}
