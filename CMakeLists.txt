# Dung duoc cho: Visual Studio (File > Open > Folder), VS Code (extension CMake Tools),
# ca MSVC lan MinGW-gcc.
cmake_minimum_required(VERSION 3.10)
project(Caro C)

set(CMAKE_C_STANDARD 99)
add_executable(caro main.c control.c model.c storage.c sound.c view.c)
target_link_libraries(caro winmm)   # am thanh (PlaySound)

if(MSVC)
  target_compile_options(caro PRIVATE /W3)
  target_compile_definitions(caro PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
