find_package(PkgConfig REQUIRED)
pkg_search_module(ZSTD REQUIRED IMPORTED_TARGET libzstd)

find_package(SZ3 REQUIRED)

target_sources(Core PRIVATE src/ZipSZ3.cxx)
target_link_libraries(Core PRIVATE SZ3::SZ3)
target_include_directories(Core PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
   $<BUILD_INTERFACE:${SZ3_INCLUDE_DIRS}>
)
