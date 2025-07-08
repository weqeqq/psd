
add_executable(tests)
target_sources(tests PRIVATE
    sources/llapi/structure/header_test.cc
    sources/llapi/stream_test.cc
)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

target_link_libraries(tests
    PRIVATE
        psd::psd
        GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(tests)
