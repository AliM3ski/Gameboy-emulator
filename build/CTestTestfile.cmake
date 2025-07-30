# CMake generated Testfile for 
# Source directory: C:/Users/AliMe/VsCodeProjects/Gameboy emulator
# Build directory: C:/Users/AliMe/VsCodeProjects/Gameboy emulator/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(check_gbe "C:/Users/AliMe/VsCodeProjects/Gameboy emulator/build/tests/check_gbe.exe")
set_tests_properties(check_gbe PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/AliMe/VsCodeProjects/Gameboy emulator/CMakeLists.txt;96;add_test;C:/Users/AliMe/VsCodeProjects/Gameboy emulator/CMakeLists.txt;0;")
subdirs("lib")
subdirs("gbemu")
subdirs("tests")
