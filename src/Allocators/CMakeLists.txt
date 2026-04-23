find_package(TBB REQUIRED COMPONENTS tbb)

file(GLOB ALLOCATOR_CPP
    "src/*.cpp"
)

add_library(Allocators STATIC ${ALLOCATOR_CPP})

target_include_directories(Allocators
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(Allocators PRIVATE cxx_std_20)

target_link_libraries(Allocators PUBLIC TBB::tbb)