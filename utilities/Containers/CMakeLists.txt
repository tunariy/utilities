file(GLOB CONTAINER_CPP
    "src/*.cpp"
)

add_library(Containers STATIC ${CONTAINER_CPP})

target_include_directories(Containers
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(Containers PUBLIC)

target_compile_features(Containers PRIVATE cxx_std_20)