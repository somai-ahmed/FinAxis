cmake_minimum_required(VERSION 3.16)
project(FinAxis_ae C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- sources du moteur (dossiers du depot, majuscules comprises) ---
set(FINAXIS_SOURCES
    Src/Core/api.c
    Src/Core/errors.c
    Src/Core/session.c
    Src/Utils/dates.c
    Src/Utils/identifiants.c
    Src/Utils/math_utils.c
    Src/Utils/monnaie.c
    Src/Utils/monnaie_convert.c
    Src/Utils/string_fincore.c
    Src/Utils/validation.c
    Src/comptabilite/comptes.c
    Src/comptabilite/ecritures.c
    Src/comptabilite/periodes.c
    Src/comptabilite/compte_resultat.c
    Src/Rapports/balance.c
    Src/Rapports/grand_livre.c
    Src/Rapports/bilan.c
    Src/Rapports/rapports_session.c
    Src/Detection/detection.c
    Src/Detection/benford.c
    Src/Detection/doublons.c
    Src/Detection/montants_ronds.c
    Src/Detection/valeurs_aberrantes.c
    Src/Persistence/session_io.c
    third_party/cJSON/cJSON.c
)

# FinAxis_ae.dll (Windows) / FinAxis_ae.so (Linux : developpement et tests)
add_library(FinAxis_ae SHARED ${FINCORE_SOURCES})
target_include_directories(FinAxis_ae PUBLIC include PRIVATE third_party/cJSON)

# Sous Windows : exporte toutes les fonctions publiques sans macro __declspec
# Copie aussi dans CMAKE_BINARY_DIR pour que ctest trouve la DLL (hors Native/build).
set_target_properties(FinAxis_ae PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)

add_custom_command(TARGET FinAxis_ae POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FinAxis_ae>
        ${CMAKE_BINARY_DIR}/$<TARGET_FILE_NAME:FinAxis_ae>
    COMMENT "Copie FinAxis_ae a cote des tests")

if(NOT MSVC)
    target_compile_options(FinAxis_ae PRIVATE -Wall -Wextra)
    target_link_libraries(FinAxis_ae PRIVATE m)
endif()

# --- tests natifs (cmake -DFINCORE_BUILD_TESTS=ON) ---
option(FINCORE_BUILD_TESTS "Compiler les tests natifs C" OFF)
if(FINCORE_BUILD_TESTS)
    enable_testing()
    foreach(nom test_utils test_comptes test_ecritures test_balance test_detection test_persistence)
        add_executable(${nom} ../../tests/native/${nom}.c)
        target_link_libraries(${nom} PRIVATE FinAxis_ae)
        if(NOT MSVC)
            target_link_libraries(${nom} PRIVATE m)
        endif()
        add_test(NAME ${nom} COMMAND ${nom})
        set_property(TEST ${nom} PROPERTY ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${CMAKE_CURRENT_SOURCE_DIR}/build")
        set_tests_properties(${nom} PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endforeach()
endif()
