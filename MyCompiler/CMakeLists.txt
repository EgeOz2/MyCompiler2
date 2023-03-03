# CMakeList.txt: MyCompiler için CMake projesi, kaynağı ekleyin ve tanımlayın
# projeye özgü mantık burada bulunur.
#

# Kaynağı bu projenin yürütülebilir dosyasına ekleyin.
add_executable (MyCompiler "ParserAST.cpp" "Lexer.hpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET MyCompiler PROPERTY CXX_STANDARD 20)
endif()

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})



# TODO: Gerekirse testleri ve yükleme hedeflerini ekleyin.