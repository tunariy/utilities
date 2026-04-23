file(GLOB CONTAINER_CPP
    "src/*.cpp"
)

add_library(Thread STATIC ${CONTAINER_CPP})

target_include_directories(Thread
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(Thread PUBLIC)

target_compile_features(Thread PRIVATE cxx_std_20)