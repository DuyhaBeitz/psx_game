target_compile_definitions(game PUBLIC
    PLATFORM_WEB
)

target_link_options(game PUBLIC
    "-sUSE_GLFW=3"
    "-sASSERTIONS=1"
    "-sASYNCIFY"
    "-sMAX_WEBGL_VERSION=2"
    "--preload-file"
    "${CMAKE_SOURCE_DIR}/assets@assets"
    "--shell-file"
    "${CMAKE_SOURCE_DIR}/web/shell.html"

    "-sINITIAL_MEMORY=134217728"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sMAXIMUM_MEMORY=1073741824"
    
    "SHELL:-s EXPORTED_RUNTIME_METHODS=[\"HEAP8\",\"HEAPU8\",\"HEAP16\",\"HEAPU16\",\"HEAP32\",\"HEAPU32\",\"HEAPF32\",\"HEAPF64\"]"
)

set_target_properties(game PROPERTIES
    OUTPUT_NAME "index"
    SUFFIX ".html"
)



target_compile_definitions(editor PUBLIC
    PLATFORM_WEB
)

target_link_options(editor PUBLIC
    "-sUSE_GLFW=3"
    "-sASSERTIONS=1"
    "-sASYNCIFY"
    "-sMAX_WEBGL_VERSION=2"
    "--preload-file"
    "${CMAKE_SOURCE_DIR}/assets@assets"
    "--shell-file"
    "${CMAKE_SOURCE_DIR}/web/shell.html"

    "-sINITIAL_MEMORY=134217728"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sMAXIMUM_MEMORY=1073741824"
    "-sEXPORT_NAME=EditorModule"
    "-sMODULARIZE=1"

    "SHELL:-s EXPORTED_RUNTIME_METHODS=[\"HEAP8\",\"HEAPU8\",\"HEAP16\",\"HEAPU16\",\"HEAP32\",\"HEAPU32\",\"HEAPF32\",\"HEAPF64\",\"ccall\",\"cwrap\"]"
    
    "SHELL:-s EXPORTED_FUNCTIONS=[\"_malloc\",\"_free\",\"_main\"]"
)

set_target_properties(editor PROPERTIES
    OUTPUT_NAME "editor_index"
    SUFFIX ".html"
)
