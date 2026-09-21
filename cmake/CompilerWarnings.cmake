# recrayon_set_warnings(<target>)
#
# Applies the project-wide warning level to a target. Honors RECRAYON_WARNINGS_AS_ERRORS.
function(recrayon_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /utf-8
            $<$<BOOL:${RECRAYON_WARNINGS_AS_ERRORS}>:/WX>)
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            $<$<BOOL:${RECRAYON_WARNINGS_AS_ERRORS}>:-Werror>)
    endif()
endfunction()
