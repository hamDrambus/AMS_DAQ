FIND_PROGRAM(CLANG_TIDY NAMES "clang-tidy-8" "clang-tidy")
# Enable clang tidy only if using a clang compiler
IF(CLANG_TIDY) #AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # If debug build enabled do automatic clang tidy
    IF(CMAKE_BUILD_TYPE MATCHES Debug)
        SET(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY} "-header-filter='${CMAKE_SOURCE_DIR}'")
    ENDIF()

    # Enable checking and formatting through run-clang-tidy if available
    # FIXME Make finding this program more portable
    GET_FILENAME_COMPONENT(CLANG_TIDY ${CLANG_TIDY} REALPATH)
    GET_FILENAME_COMPONENT(CLANG_DIR ${CLANG_TIDY} DIRECTORY)
    FIND_PROGRAM(RUN_CLANG_TIDY NAMES "run-clang-tidy.py" "run-clang-tidy-4.0.py" HINTS ${CMAKE_SOURCE_DIR}/cmake ${CLANG_DIR}/../share/clang/ /usr/bin/)
    IF(RUN_CLANG_TIDY)
        MESSAGE(STATUS "Found ${CLANG_TIDY}, adding linting targets")
	# write a .clang-tidy file in the binary dir to disable checks for created files
	FILE(WRITE ${CMAKE_BINARY_DIR}/.clang-tidy "\n---\nChecks: '-*,llvm-twine-local'\n...\n")

        # Set export commands on
        SET (CMAKE_EXPORT_COMPILE_COMMANDS ON)

        # Get amount of processors to speed up linting
        INCLUDE(ProcessorCount)
        ProcessorCount(NPROC)
        IF(NPROC EQUAL 0)
            SET(NPROC 1)
        ENDIF()

        ADD_CUSTOM_TARGET(
            lint COMMAND
            ${RUN_CLANG_TIDY} -fix -format -header-filter=${CMAKE_SOURCE_DIR} -extra-arg=-Wno-unknown-warning-option -j${NPROC}
            COMMENT "Auto fixing problems in all source files"
        )

        ADD_CUSTOM_TARGET(
            check-lint COMMAND
            ${RUN_CLANG_TIDY} -header-filter=${CMAKE_SOURCE_DIR} -extra-arg=-Wno-unknown-warning-option -j${NPROC}
            | tee ${CMAKE_BINARY_DIR}/check_lint_file.txt
            # WARNING: fix to stop with error if there are problems
            COMMAND ! grep -c ": error: " ${CMAKE_BINARY_DIR}/check_lint_file.txt > /dev/null
            COMMENT "Checking for problems in source files"
        )
    ELSE()
        MESSAGE(STATUS "Could NOT find run-clang-tidy script")
    ENDIF()
ELSE()
    IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        MESSAGE(STATUS "Could NOT find clang-tidy")
    ELSE()
        MESSAGE(STATUS "Could NOT check for clang-tidy, wrong compiler: ${CMAKE_CXX_COMPILER_ID}")
    ENDIF()
ENDIF()


