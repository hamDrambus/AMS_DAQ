# Adding clang-format check and formatter if found
find_program(_clang_format NAMES "clang-format-6.0" "clang-format-5.0" "clang-format-4.0" "clang-format")
if(_clang_format)
    exec_program(${_clang_format} ${CMAKE_CURRENT_SOURCE_DIR} ARGS --version OUTPUT_VARIABLE _clang_version)
    string(REGEX REPLACE ".*([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" _clang_major_version ${_clang_version})

    if((${_clang_major_version} GREATER "4") OR (${_clang_major_version} EQUAL "4"))
        message(STATUS "Found ${_clang_format}, adding formatting targets")
        add_custom_target(
            format
            COMMAND
            ${_clang_format}
            -i
            -style=file
            ${FORMAT_CXX_SOURCE_FILES}
            COMMENT "Auto formatting of all source files"
        )

        add_custom_target(
            check-format
            COMMAND
            ${_clang_format}
            -style=file
            -output-replacements-xml
            ${FORMAT_CXX_SOURCE_FILES}
            # print output
            | tee ${CMAKE_BINARY_DIR}/check_format_file.txt | grep -c "replacement " |
            tr -d "[:cntrl:]" && echo " replacements necessary"
            # WARNING: fix to stop with error if there are problems
            COMMAND ! grep -c "replacement "
            ${CMAKE_BINARY_DIR}/check_format_file.txt > /dev/null
            COMMENT "Checking format compliance"
        )

    else()
        message(STATUS "Could only find version ${_clang_major_version} or clang-format, but version >= 4 is required.")
    endif()
else()
    message(STATUS "Could NOT find clang-format")
endif()
