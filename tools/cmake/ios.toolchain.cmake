include("${CMAKE_CURRENT_LIST_DIR}/Utilities.cmake")
include(GNUInstallDirs)

check_generator("Xcode")

fix_xcode_try_compile()

set_common_xcode_attributes()

set(REALM_SKIP_SHARED_LIB ON)
set(CPACK_SYSTEM_NAME "ios")

set(CMAKE_OSX_SYSROOT "iphoneos")

set(CMAKE_XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos iphonesimulator")
set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "8.0")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")

set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "YES")
set(CMAKE_XCODE_ATTRIBUTE_REALM_ALIGN_FLAG "")
set(CMAKE_XCODE_ATTRIBUTE_REALM_ALIGN_FLAG[arch=armv7] "-fno-aligned-new")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $(REALM_ALIGN_FLAG)")

set_bitcode_attributes()

install_arch_slices_for_platform("iphone")
