cmake_minimum_required(VERSION 3.22)

project(AutoEagleEye VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 强制适配 1.5.97 (runtime 1_5_97)
add_compile_definitions(SKYRIM_SUPPORT_AE=OFF)

# 下载并引入 CommonLibSSE
include(FetchContent)
FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com
    GIT_TAG        master
)
FetchContent_MakeAvailable(CommonLibSSE)

add_library(${PROJECT_NAME} SHARED src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibSSE::CommonLibSSE)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# 输出重命名为常规 SKSE 插件
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
