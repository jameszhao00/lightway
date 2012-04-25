#pragma once
#include <memory>
#include "scene.h"

unique_ptr<StaticScene> load_scene(string path, vec3 translation = vec3(0, 0, 0), float scale = 1.f);