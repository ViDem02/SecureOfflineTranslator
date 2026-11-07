if (NOT EXISTS "${PROJECT_SOURCE_DIR}/src")
    message(FATAL_ERROR "Project source directory is invalid")
endif()

file(READ "${PROJECT_SOURCE_DIR}/entitlements/SecureTranslator.entitlements" ENTITLEMENTS)
if (NOT ENTITLEMENTS MATCHES "com.apple.security.app-sandbox")
    message(FATAL_ERROR "App Sandbox entitlement is missing")
endif()
if (ENTITLEMENTS MATCHES "network")
    message(FATAL_ERROR "Network entitlement is present")
endif()

file(GLOB_RECURSE APP_SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.h")
foreach (SOURCE_FILE IN LISTS APP_SOURCES)
    file(READ "${SOURCE_FILE}" CONTENT)
    if (CONTENT MATCHES "QProcess|system[ \t]*\\(|popen[ \t]*\\(")
        message(FATAL_ERROR "External process API found in ${SOURCE_FILE}")
    endif()
    if (CONTENT MATCHES "QNetworkAccessManager|QTcpSocket|QUdpSocket|QNetworkRequest")
        message(FATAL_ERROR "Network API found in ${SOURCE_FILE}")
    endif()
endforeach()

if (APPLE AND EXISTS "${APP_BUNDLE}")
    execute_process(
        COMMAND codesign --verify --deep --strict "${APP_BUNDLE}"
        RESULT_VARIABLE SIGN_RESULT
        OUTPUT_QUIET
        ERROR_VARIABLE SIGN_ERROR
    )
    if (NOT SIGN_RESULT EQUAL 0)
        message(FATAL_ERROR "Bundle signature verification failed: ${SIGN_ERROR}")
    endif()
endif()

file(READ "${PROJECT_SOURCE_DIR}/src/engine/LlamaEngine.cpp" ENGINE_FILE)
if (NOT ENGINE_FILE MATCHES "n_gpu_layers = -1")
    message(FATAL_ERROR "Full GPU offload is not configured")
endif()

message(STATUS "Security hardening checks passed")
