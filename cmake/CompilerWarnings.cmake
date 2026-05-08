# Strict warnings. We want the compiler yelling at us early — most "weird" bugs
# in low-level C++ are caught by -Wconversion, -Wsign-conversion and -Wshadow.
function(llte_set_warnings target visibility)
    target_compile_options(${target} ${visibility}
        $<$<CXX_COMPILER_ID:GNU,Clang>:
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
            -Wimplicit-fallthrough
        >
    )
endfunction()
