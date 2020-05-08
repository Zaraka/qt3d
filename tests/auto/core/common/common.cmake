function(qt3d_setup_common_test target)
    set(commonDir ${PROJECT_SOURCE_DIR}/tests/auto/core/common)
    target_sources(${target} PRIVATE ${commonDir}/qbackendnodetester.h ${commonDir}/qbackendnodetester.cpp)
    target_include_directories(${target} PRIVATE ${commonDir})

#    qt_extend_target(${target} CONDITION QT_FEATURE_private_tests
#        SOURCES
#            ${commonDir}/qbackendnodetester.cpp ${commonDir}/qbackendnodetester.h
#            ${commonDir}/testarbiter.h
#    )
endfunction()
