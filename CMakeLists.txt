# CMakeList.txt: Üst düzey CMake proje dosyası, genel yapılandırma gerçekleştirin
# ve alt projeleri buraya ekleyin.
#
cmake_minimum_required (VERSION 3.8)

project ("MyCompiler")

# Alt projeleri ekleyin.
add_subdirectory ("MyCompiler")
