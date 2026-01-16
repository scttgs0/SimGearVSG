
function(simgear_component name includePath sources headers)
    target_sources(SimGearCore PRIVATE ${sources} ${headers})
    install (FILES ${headers} DESTINATION include/simgear/${includePath})
endfunction()

function(simgear_scene_component name includePath sources headers)
    if (TARGET SimGearScene)
        target_sources(SimGearScene PRIVATE ${sources} ${headers})
        install (FILES ${headers} DESTINATION include/simgear/${includePath})
    endif()
endfunction()
