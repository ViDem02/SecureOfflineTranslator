cmake_minimum_required(VERSION 3.24)
project(SecureTranslator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Test Widgets)
include(CTest)
option(SECURE_TRANSLATOR_SIGN "Sign the macOS app bundle after building" ON)
set(SECURE_TRANSLATOR_SIGNING_IDENTITY "-" CACHE STRING
    "codesign identity; use '-' for ad-hoc development signing")
file(GLOB SECURE_TRANSLATOR_MODEL_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/model/*.gguf")
list(LENGTH SECURE_TRANSLATOR_MODEL_FILES SECURE_TRANSLATOR_MODEL_COUNT)
if (NOT SECURE_TRANSLATOR_MODEL_COUNT EQUAL 1)
    message(FATAL_ERROR
        "The model/ folder must contain exactly one .gguf model file; found "
        "${SECURE_TRANSLATOR_MODEL_COUNT}.")
endif()
list(GET SECURE_TRANSLATOR_MODEL_FILES 0 SECURE_TRANSLATOR_MODEL_PATH)
get_filename_component(SECURE_TRANSLATOR_MODEL_FILE
    "${SECURE_TRANSLATOR_MODEL_PATH}" NAME)
file(GLOB SECURE_TRANSLATOR_GLOSSARY_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/glossary/*.yaml")
list(LENGTH SECURE_TRANSLATOR_GLOSSARY_FILES SECURE_TRANSLATOR_GLOSSARY_COUNT)
if (SECURE_TRANSLATOR_GLOSSARY_COUNT GREATER 1)
    message(FATAL_ERROR "The glossary/ folder may contain at most one .yaml file")
endif()
set(SECURE_TRANSLATOR_GLOSSARY_FILE "")
if (SECURE_TRANSLATOR_GLOSSARY_COUNT EQUAL 1)
    list(GET SECURE_TRANSLATOR_GLOSSARY_FILES 0 SECURE_TRANSLATOR_GLOSSARY_PATH)
    get_filename_component(SECURE_TRANSLATOR_GLOSSARY_FILE
        "${SECURE_TRANSLATOR_GLOSSARY_PATH}" NAME)
endif()

set(LLAMA_BUILD_COMMON OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(LLAMA_METAL ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
if (EXISTS "${CMAKE_SOURCE_DIR}/third_party/llama.cpp/CMakeLists.txt")
    add_subdirectory(third_party/llama.cpp EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "Vendored llama.cpp is missing: third_party/llama.cpp")
endif()

add_executable(SecureTranslator
    src/main.cpp
    src/app/MainWindow.cpp
    src/app/MainWindow.h
    src/core/TranslationController.cpp
    src/core/TranslationController.h
    src/core/TranslationWorker.cpp
    src/core/TranslationWorker.h
    src/engine/InferenceEngine.h
    src/engine/LlamaEngine.cpp
    src/engine/LlamaEngine.h
    src/security/ModelVerifier.cpp
    src/security/ModelVerifier.h
    src/security/SecurityStatus.cpp
    src/security/SecurityStatus.h
    src/storage/FileAccess.cpp
    src/storage/FileAccess.h
    src/system/ResourceMonitor.cpp
    src/system/ResourceMonitor.h
    resources.qrc
)

target_link_libraries(SecureTranslator PRIVATE Qt6::Widgets)
target_link_libraries(SecureTranslator PRIVATE llama)
if (APPLE)
    target_link_libraries(SecureTranslator PRIVATE "-framework Security")
endif()
target_include_directories(SecureTranslator PRIVATE src)
target_compile_definitions(SecureTranslator PRIVATE
    SECURE_TRANSLATOR_BUNDLED_MODEL_FILE="${SECURE_TRANSLATOR_MODEL_FILE}"
    SECURE_TRANSLATOR_BUNDLED_GLOSSARY_FILE="${SECURE_TRANSLATOR_GLOSSARY_FILE}"
)

set_target_properties(SecureTranslator PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "local.securetranslator.app"
    MACOSX_BUNDLE_BUNDLE_NAME "SecureTranslator"
    BUILD_RPATH ""
    INSTALL_RPATH ""
    SKIP_BUILD_RPATH TRUE
    SKIP_INSTALL_RPATH TRUE
)

if (APPLE AND SECURE_TRANSLATOR_SIGN)
    find_program(QT_MACDEPLOYQT macdeployqt
        HINTS
            /opt/homebrew/opt/qtbase/bin
            /opt/homebrew/bin
    )
    if (NOT QT_MACDEPLOYQT)
        message(FATAL_ERROR "macdeployqt is required to package the Qt application bundle")
    endif()

    add_custom_command(TARGET SecureTranslator POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_DIR:SecureTranslator>/Contents/Resources"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${SECURE_TRANSLATOR_MODEL_PATH}"
            "$<TARGET_BUNDLE_DIR:SecureTranslator>/Contents/Resources/${SECURE_TRANSLATOR_MODEL_FILE}"
        COMMAND "${QT_MACDEPLOYQT}" "$<TARGET_BUNDLE_DIR:SecureTranslator>"
            -always-overwrite
        COMMAND codesign --force --deep --sign "${SECURE_TRANSLATOR_SIGNING_IDENTITY}"
            --entitlements "${CMAKE_SOURCE_DIR}/entitlements/SecureTranslator.entitlements"
            "$<TARGET_BUNDLE_DIR:SecureTranslator>"
        COMMENT "Deploying Qt frameworks/plugins and signing the application bundle"
    )
    if (SECURE_TRANSLATOR_GLOSSARY_COUNT EQUAL 1)
        add_custom_command(TARGET SecureTranslator POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${SECURE_TRANSLATOR_GLOSSARY_PATH}"
                "$<TARGET_BUNDLE_DIR:SecureTranslator>/Contents/Resources/${SECURE_TRANSLATOR_GLOSSARY_FILE}"
            COMMENT "Bundling glossary"
        )
    endif()
endif()

install(TARGETS SecureTranslator BUNDLE DESTINATION .)

if (BUILD_TESTING)
    add_executable(SecureTranslatorSecurityTests
        tests/ModelVerifierTests.cpp
        src/security/ModelVerifier.cpp
        src/security/ModelVerifier.h
    )
    target_link_libraries(SecureTranslatorSecurityTests PRIVATE Qt6::Test)
    target_include_directories(SecureTranslatorSecurityTests PRIVATE src)
    add_test(NAME SecureTranslatorSecurityTests COMMAND SecureTranslatorSecurityTests)

    add_test(
        NAME SecureTranslatorHardeningChecks
        COMMAND "${CMAKE_COMMAND}"
            -DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DAPP_BUNDLE=$<TARGET_BUNDLE_DIR:SecureTranslator>
            -P "${CMAKE_SOURCE_DIR}/tests/hardening_checks.cmake"
    )
endif()
