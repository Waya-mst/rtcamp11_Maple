if(NOT DEFINED INPUT)
  message(FATAL_ERROR "need INPUT")
endif()
if(NOT DEFINED OUTPUT)
  message(FATAL_ERROR "need OUTPUT")
endif()
if(NOT DEFINED VAR)
  message(FATAL_ERROR "need VAR")
endif()

file(READ "${INPUT}" data HEX)

string(REGEX MATCHALL "[0-9a-fA-F][0-9a-fA-F]" bytes "${data}")

file(WRITE "${OUTPUT}" "// generated from ${INPUT}\n")
file(APPEND "${OUTPUT}" "static const unsigned char ${VAR}[] = {\n  ")

foreach(b ${bytes})
  file(APPEND "${OUTPUT}" "0x${b}, ")
endforeach()

file(APPEND "${OUTPUT}" "\n};\n")
file(APPEND "${OUTPUT}" "static const size_t ${VAR}_size = sizeof(${VAR});\n")