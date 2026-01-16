
function(add_simgear_test _name _sources)
    add_executable(${_name} ${_sources})
    target_link_libraries(${_name} SimGearCore Threads::Threads)

    # for simgear_config.h
    target_include_directories(${_name} PRIVATE ${PROJECT_BINARY_DIR}/simgear)
endfunction()


function(add_simgear_autotest _name _sources)    
    add_executable(${_name} ${_sources})
    target_link_libraries(${_name} SimGearCore Threads::Threads)

    # for simgear_config.h
    target_include_directories(${_name} PRIVATE ${PROJECT_BINARY_DIR}/simgear)

    add_test(${_name} ${EXECUTABLE_OUTPUT_PATH}/${_name})
    set_tests_properties(${_name} PROPERTIES TIMEOUT 30) 
endfunction()


function(add_simgear_scene_autotest _name _sources)
    add_executable(${_name} ${_sources})
    target_link_libraries(${_name} SimGearScene Threads::Threads)

    # for simgear_config.h
    target_include_directories(${_name} PRIVATE ${PROJECT_BINARY_DIR}/simgear)

    add_test(${_name} ${EXECUTABLE_OUTPUT_PATH}/${_name})
endfunction()
