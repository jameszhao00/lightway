#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/multi_array.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>

#include <cstdio>
#include <memory>
#include <algorithm>
#include <list>
#include <thread>
#include <mutex>
#include <iostream>
#include <vector>
#include <chrono>
#include <functional>


#define GLFW_CDECL
#include <gl/glfw.h>

#include <glm/glm.hpp>

#include <AntTweakBar.h>


#include <assimp/assimp.hpp>  
#include <assimp/aiScene.h>   
#include <assimp/aiPostProcess.h> 