cmake_minimum_required(VERSION 3.15)
project(final_project LANGUAGES CXX)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)
set(SOURCE_PATH ${CMAKE_SOURCE_DIR}/src)

file(COPY media/ DESTINATION media)

add_subdirectory(${THIRD_PARTY_LIBRARY_PATH}/glfw)
add_subdirectory(${THIRD_PARTY_LIBRARY_PATH}/glad)
add_subdirectory(${THIRD_PARTY_LIBRARY_PATH}/glm)
add_subdirectory(${THIRD_PARTY_LIBRARY_PATH}/imgui)
add_subdirectory(${THIRD_PARTY_LIBRARY_PATH}/stb)

file(GLOB BASE_HDR    ${SOURCE_PATH}/base/*.h)
file(GLOB BASE_SRC    ${SOURCE_PATH}/base/*.cpp)
file(GLOB SCENE_HDR   ${SOURCE_PATH}/scene/*.h)
file(GLOB SCENE_SRC   ${SOURCE_PATH}/scene/*.cpp)
file(GLOB OBJ_HDR     ${SOURCE_PATH}/obj_loader/*.h)
file(GLOB OBJ_SRC     ${SOURCE_PATH}/obj_loader/*.cpp)
file(GLOB MESH_HDR    ${SOURCE_PATH}/mesh/*.h)
file(GLOB MESH_SRC    ${SOURCE_PATH}/mesh/*.cpp)
file(GLOB PRIM_HDR    ${SOURCE_PATH}/primitives/*.h)
file(GLOB PRIM_SRC    ${SOURCE_PATH}/primitives/*.cpp)
file(GLOB CAM_HDR     ${SOURCE_PATH}/camera/*.h)
file(GLOB RENDER_HDR  ${SOURCE_PATH}/render_util/*.h)
file(GLOB ORBIT_HDR   ${SOURCE_PATH}/orbit_control/*.h)
file(GLOB NURBS_HDR   ${SOURCE_PATH}/nurbs/*.h)
file(GLOB ANIM_HDR    ${SOURCE_PATH}/animation/*.h)
file(GLOB ANIM_SRC    ${SOURCE_PATH}/animation/*.cpp)
file(GLOB MC_HDR      ${SOURCE_PATH}/game/minecraft_game.h)
file(GLOB MC_SRC      ${SOURCE_PATH}/game/minecraft_game.cpp)

add_library(engine STATIC
    ${BASE_HDR} ${BASE_SRC}
    ${SCENE_HDR} ${SCENE_SRC}
    ${OBJ_HDR} ${OBJ_SRC}
    ${MESH_HDR} ${MESH_SRC}
    ${PRIM_HDR} ${PRIM_SRC}
    ${CAM_HDR} ${RENDER_HDR} ${ORBIT_HDR} ${NURBS_HDR} ${ANIM_HDR} ${ANIM_SRC}
    ${MC_HDR} ${MC_SRC}
)

target_include_directories(engine PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/base
    ${THIRD_PARTY_LIBRARY_PATH}/glfw/include
    ${THIRD_PARTY_LIBRARY_PATH}/glad/include
    ${THIRD_PARTY_LIBRARY_PATH}/glm
    ${THIRD_PARTY_LIBRARY_PATH}/imgui
    ${THIRD_PARTY_LIBRARY_PATH}/stb
)

set_target_properties(engine PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_definitions(engine PRIVATE _USE_MATH_DEFINES)
if(MSVC)
    target_compile_options(engine PRIVATE /utf-8)
endif()
target_link_libraries(engine PUBLIC glfw glad glm imgui stb)

function(add_example NAME)
    add_executable(${NAME} examples/${NAME}.cpp)
    target_link_libraries(${NAME} PRIVATE engine)
    if(WIN32)
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    endif()
    if(MSVC)
        target_compile_options(${NAME} PRIVATE /utf-8)
    endif()
endfunction()

add_example(01_primitives)
add_example(02_obj_loader)
add_example(03_materials)
add_example(04_transform)
add_example(05_lighting)
add_example(06_navigation)
add_example(07_animation)
add_example(08_nurbs)
add_example(09_skeletal)
add_example(09_bone_test)
add_example(09_humanoid_test)

file(GLOB GAME_HDR ${SOURCE_PATH}/game/*.h)
file(GLOB GAME_SRC ${SOURCE_PATH}/game/*.cpp)
file(GLOB MAIN_SRC ${SOURCE_PATH}/main.cpp)

add_executable(final_project ${MAIN_SRC} ${GAME_HDR} ${GAME_SRC})
target_link_libraries(final_project PRIVATE engine)
add_custom_command(TARGET final_project POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/media"
    "$<TARGET_FILE_DIR:final_project>/media"
)
set_target_properties(final_project PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(WIN32)
    set_target_properties(final_project PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
set_target_properties(final_project PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
if(MSVC)
    target_compile_options(final_project PRIVATE /utf-8)
endif()
