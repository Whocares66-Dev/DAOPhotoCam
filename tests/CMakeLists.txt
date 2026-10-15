find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

# Over photocam_core only: the flight, the session and the tools' JSON, with no game.
add_executable(
    photocam_tests
    quiet_crt.cpp
    test_flycamera.cpp
    test_session.cpp
    test_tools.cpp
)
target_link_libraries(
    photocam_tests
    PRIVATE photocam_core Catch2::Catch2WithMain
)
dao_apply_warnings(photocam_tests)
catch_discover_tests(photocam_tests)
