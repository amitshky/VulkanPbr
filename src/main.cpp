#include "core/core.h"
#include "engine/engine.h"

int main()
{
	Logger::Init();

	Engine* engine = Engine::Create("Vulkan PBR", 400, 400);
	engine->Run();
	delete engine;
}
