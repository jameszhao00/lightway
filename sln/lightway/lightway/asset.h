#pragma once
#include <memory>
#include "scene.h"

unique_ptr<StaticScene> load_scene(string path, float3 translation = float3(0, 0, 0), float scale = 1.f);