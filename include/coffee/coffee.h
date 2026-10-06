#pragma once

#include <coffee/config.h>
#include <coffee/application.h>
#include <coffee/asset_locator.h>
#include <coffee/buffer.h>
#include <coffee/clock.h>
#include <coffee/error.h>
#include <coffee/event.h>
#include <coffee/file.h>
#include <coffee/framebuffer.h>
#include <coffee/input.h>
#include <coffee/image.h>
#include <coffee/log.h>
#include <coffee/math.h>
#include <coffee/mesh.h>
#include <coffee/orthographic_camera.h>
#include <coffee/perspective_camera.h>
#include <coffee/physics_world_2d.h>
#include <coffee/render_types.h>
#include <coffee/renderer.h>
#include <coffee/resource_cache.h>
#include <coffee/shader.h>
#include <coffee/shader_preprocessor.h>
#include <coffee/sprite_batch.h>
#include <coffee/texture.h>
#include <coffee/version.h>
#include <coffee/vertex_array.h>
#include <coffee/window.h>

#if COFFEE_HAS_VULKAN_MODULE
#include <coffee/vulkan_context.h>
#endif
