cmake_minimum_required(VERSION 3.5)


set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 20)
set(PROJECT_VERSION_PATCH 5)

project(LiveAmp
	LANGUAGES CXX
	VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX "." CACHE PATH "..." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)	

set(CMAKE_INSTALL_BINDIR ${PROJECT_NAME})
set(CMAKE_INSTALL_LIBDIR ${PROJECT_NAME})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(lsl_DIR ${CMAKE_CURRENT_LIST_DIR}/LSL/lib/cmake/LSL)

find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(Threads REQUIRED)
find_package(lsl REQUIRED
	HINTS ${LSL_INSTALL_ROOT}
	"${CMAKE_CURRENT_LIST_DIR}/../../LSL/liblsl/build/"
	"${CMAKE_CURRENT_LIST_DIR}/../../LSL/liblsl/build/install"
	"${CMAKE_CURRENT_LIST_DIR}/../../LSL/liblsl/out/build/x64-Release"
	"${CMAKE_CURRENT_LIST_DIR}/../../LSL/liblsl/out/install/x64-Release"
	PATH_SUFFIXES share/LSL)
	
set_target_properties(${target} PROPERTIES AUTORCC_OPTIONS "--compress;9")

if(CMAKE_BUILD_TYPE_UPPER MATCHES "^(DEBUG|RELWITHDEBINFO)$")
    set(CPACK_STRIP_FILES FALSE)
else()
    set(CPACK_STRIP_FILES TRUE)
endif()

set(CMAKE_MODULE_PATH ".")
if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "CMAKE_BUILD_TYPE was default initialized to Release")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()	

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(platform 64)
	set(LiveAmpLib2 ${CMAKE_CURRENT_LIST_DIR}/LiveAmpLib2/x64/Release/LiveAmpLib2.lib)
	set(LiveAmpLib2Dll ${CMAKE_CURRENT_LIST_DIR}/LiveAmpLib2/x64/Release/LiveAmpLib2.dll)
else()
	set(platform 32)
	set(LiveAmpLib2 ${CMAKE_CURRENT_LIST_DIR}/LiveAmpLib2/Win32/Release/LiveAmpLib2.lib)
	set(LiveAmpLib2Dll ${CMAKE_CURRENT_LIST_DIR}/LiveAmpLib2/Win32/Release/LiveAmpLib2.dll)
endif()

add_executable(${PROJECT_NAME} WIN32
	LiveAmp.cpp
	LiveAmp.h
	main.cpp
	mainwindow.cpp
	mainwindow.h
	mainwindow.ui
	mainwindow.rc
	LiveAmpLib2/Amplifier_LIB.h 
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
	Qt5::Widgets
	Threads::Threads
	LSL::lsl
	${LiveAmpLib2}
)	

include_directories(
	LiveAmpLib2
)

include (InstallRequiredSystemLibraries)
set(CMAKE_INSTALL_MFC_LIBRARIES)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP false)

install(TARGETS ${PROJECT_NAME}
	COMPONENT "${PROJECT_NAME}"
	BUNDLE DESTINATION bin
	RUNTIME DESTINATION bin)
	
install(FILES $<TARGET_FILE:LSL::lsl>
	DESTINATION bin
	COMPONENT "${PROJECT_NAME}"
)

install(FILES ${PROJECT_NAME}.cfg
	DESTINATION bin
	COMPONENT "${PROJECT_NAME}"
)

install(FILES LICENSE.txt
	DESTINATION bin
	COMPONENT "${PROJECT_NAME}"
)

install(FILES ${LiveAmpLib2Dll}
	DESTINATION bin
	COMPONENT "${PROJECT_NAME}"
)

install(FILES explanation_of_trigger_marker_types.pdf
	DESTINATION bin
	COMPONENT "${PROJECT_NAME}"
)

get_target_property(qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
mark_as_advanced(WINDEPLOYQT_EXECUTABLE)

function(windeployqt target)
	# Bundle Library Files
    if(CMAKE_BUILD_TYPE_UPPER STREQUAL "DEBUG")
        set(WINDEPLOYQT_ARGS --debug)
    else()
        set(WINDEPLOYQT_ARGS --release)
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
                       COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/winqt/"
                       COMMAND "${CMAKE_COMMAND}" -E
                               env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
                               ${WINDEPLOYQT_ARGS}
                               --verbose 0
                               --no-compiler-runtime
                               --no-angle
							   --no-svg
                               --no-opengl-sw
                               --dir "${CMAKE_CURRENT_BINARY_DIR}/winqt/"
                               $<TARGET_FILE:${target}>
                       COMMENT "Deploying Qt..."
    )
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/winqt/" DESTINATION bin)
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    #include(InstallRequiredSystemLibraries)
endfunction()

add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${WINDEPLOYQT_EXECUTABLE} --qmldir
		${CMAKE_SOURCE_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different  
		$<TARGET_FILE:LSL::lsl>
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)
add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_SOURCE_DIR}//${PROJECT_NAME}.cfg
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)
add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${LiveAmpLib2Dll}
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)
add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_SOURCE_DIR}/release_notes.txt
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)
add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_SOURCE_DIR}/LICENSE.txt
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)
add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_SOURCE_DIR}/explanation_of_trigger_marker_types.pdf
		$<TARGET_FILE_DIR:${PROJECT_NAME}>)

include(${CMAKE_CURRENT_LIST_DIR}/Packaging.cmake)
