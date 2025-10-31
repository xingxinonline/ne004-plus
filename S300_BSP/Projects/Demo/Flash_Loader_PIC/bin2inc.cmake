# bin2inc.cmake
# CMake script to convert binary file to C include file
# Usage: cmake -DBIN_FILE=input.bin -DINC_FILE=output.inc -P bin2inc.cmake

if(NOT DEFINED BIN_FILE OR NOT DEFINED INC_FILE)
    message(FATAL_ERROR "Usage: cmake -DBIN_FILE=input.bin -DINC_FILE=output.inc -P bin2inc.cmake")
endif()

if(NOT EXISTS ${BIN_FILE})
    message(FATAL_ERROR "Binary file not found: ${BIN_FILE}")
endif()

# Read binary file
file(READ ${BIN_FILE} BIN_DATA HEX)

# Get file size
file(SIZE ${BIN_FILE} BIN_SIZE)

# Convert hex string to byte array format
string(LENGTH ${BIN_DATA} HEX_LEN)
set(BYTE_ARRAY "")
set(BYTE_COUNT 0)

math(EXPR NUM_BYTES "${HEX_LEN} / 2")

# Process each byte
foreach(i RANGE 0 ${NUM_BYTES})
    math(EXPR POS "${i} * 2")

    if(POS LESS ${HEX_LEN})
        string(SUBSTRING ${BIN_DATA} ${POS} 2 BYTE_HEX)

        # Add newline every 16 bytes
        math(EXPR MOD "${BYTE_COUNT} % 16")

        if(MOD EQUAL 0 AND BYTE_COUNT GREATER 0)
            string(APPEND BYTE_ARRAY "\n    ")
        endif()

        string(APPEND BYTE_ARRAY "0x${BYTE_HEX}")

        # Add comma if not last byte
        math(EXPR NEXT_I "${i} + 1")

        if(NEXT_I LESS ${NUM_BYTES})
            string(APPEND BYTE_ARRAY ", ")
        endif()

        math(EXPR BYTE_COUNT "${BYTE_COUNT} + 1")
    endif()
endforeach()

# Generate C include file
file(WRITE ${INC_FILE} "/* Auto-generated from ${BIN_FILE} */\n")
file(APPEND ${INC_FILE} "/* Size: ${BIN_SIZE} bytes */\n\n")
file(APPEND ${INC_FILE} "#include <stdint.h>\n\n")
file(APPEND ${INC_FILE} "const uint8_t flash_ops_code[] = {\n    ")
file(APPEND ${INC_FILE} "${BYTE_ARRAY}\n")
file(APPEND ${INC_FILE} "};\n\n")
file(APPEND ${INC_FILE} "const uint32_t flash_ops_code_size = ${BIN_SIZE}U;\n")

message(STATUS "Generated ${INC_FILE} from ${BIN_FILE} (${BIN_SIZE} bytes)")
