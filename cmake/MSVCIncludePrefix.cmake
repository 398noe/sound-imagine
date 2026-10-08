# Probe compilation with UTF-8 output. Automatic codepage conversion can corrupt
# Japanese /showIncludes prefixes, leaving Ninja with zero dependencies.
if(CMAKE_HOST_WIN32 AND CMAKE_GENERATOR MATCHES "Ninja")
    file(WRITE "${CMAKE_BINARY_DIR}/msvc-include-probe.cpp" "#include <stddef.h>\n")
    execute_process(COMMAND cl /nologo /showIncludes /utf-8 /c
        "/Fo${CMAKE_BINARY_DIR}/msvc-include-probe.obj"
        "${CMAKE_BINARY_DIR}/msvc-include-probe.cpp"
        OUTPUT_VARIABLE noe_probe ERROR_VARIABLE noe_probe_error ENCODING UTF-8)
    string(CONCAT noe_probe "${noe_probe_error}" "${noe_probe}")
    string(REGEX MATCH "[^\r\n]*: +[A-Za-z]:[/\\\\]" noe_prefix "${noe_probe}")
    if(noe_prefix)
        string(REGEX REPLACE "[A-Za-z]:[/\\\\]$" "" CMAKE_CL_SHOWINCLUDES_PREFIX "${noe_prefix}")
        set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "${CMAKE_CL_SHOWINCLUDES_PREFIX}")
        set(CMAKE_C_CL_SHOWINCLUDES_PREFIX "${CMAKE_CL_SHOWINCLUDES_PREFIX}")
    endif()
endif()
