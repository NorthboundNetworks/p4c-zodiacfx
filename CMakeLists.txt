# Makefile for the ZODAICFX P4-16 back-end.
# To be included in the main P4C compiler makefile

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set (P4C_ZODIACFX_SOURCES
  p4c-zodiacfx.cpp
  zodiacfx-Backend.cpp
  zodiacfx-Program.cpp
  zodiacfx-Table.cpp
  zodiacfx-Control.cpp
  zodiacfx-Parser.cpp
  zodiacfx-Target.cpp
  zodiacfx-Type.cpp
  zodiacfx-CodeGen.cpp
  zodiacfx-Model.cpp
  zodiacfx-Midend.cpp
  zodiacfx-Lower.cpp
  )

set (P4C_ZODIACFX_HEADERS
  zodiacfx-CodeGen.h
  zodiacfx-Backend.h
  zodiacfx-Control.h
  zodiacfx-Model.h
  zodiacfx-Object.h
  zodiacfx-Program.h
  zodiacfx-Options.h
  zodiacfx-Parser.h
  zodiacfx-Table.h
  zodiacfx-Type.h
  zodiacfx-Midend.h
  zodiacfx-Target.h
  zodiacfx-Lower.h
  )

set (P4C_ZODIACFX_DIST_HEADERS p4include/zodiacfx_model.p4)

add_cpplint_files(${CMAKE_CURRENT_SOURCE_DIR} "$(P4C_ZODIACFX_SOURCES)")

#build_unified(P4C_ZODIACFX_SOURCES ALL)
add_executable(p4c-zodiacfx ${P4C_ZODIACFX_SOURCES})
target_link_libraries (p4c-zodiacfx ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
#add_dependencies(p4c-zodiacfx genIR frontend)

install (TARGETS p4c-zodiacfx
        RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/p4include
        DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})


add_custom_target(linkp4czodiacfx
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/p4c-zodiacfx ${P4C_BINARY_DIR}/p4c-zodiacfx
        COMMAND ${CMAKE_COMMAND} -E make_directory ${P4C_BINARY_DIR}/p4include &&
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${P4C_ZODIACFX_DIST_HEADERS} ${P4C_BINARY_DIR}/p4include
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${P4C_BINARY_DIR}/p4include ${CMAKE_CURRENT_BINARY_DIR}/p4include
        )

add_dependencies(p4c_driver linkp4czodiacfx)
