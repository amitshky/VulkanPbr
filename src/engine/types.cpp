#include "engine/types.h"

#ifdef NDEBUG // release mode
const bool Config::enableValidationLayers = false;
#else // debug mode
const bool Config::enableValidationLayers = true;
#endif
const uint32_t Config::maxFramesInFlight = 2;
const std::array<const char*, 1> Config::deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const std::array<const char*, 1> Config::validationLayers{ "VK_LAYER_KHRONOS_validation" };
