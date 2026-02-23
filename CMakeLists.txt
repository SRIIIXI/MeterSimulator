cmake_minimum_required(VERSION 3.10)

set(Project PosixDlms)
project(${Project})

if(UNIX AND NOT APPLE)
set(LINUX TRUE)
endif()

if(UNIX AND APPLE)
set(DARWIN TRUE)
endif()

#add_subdirectory(Dlms)
#add_subdirectory(DlmsClient)
add_subdirectory(DlmsServer)
