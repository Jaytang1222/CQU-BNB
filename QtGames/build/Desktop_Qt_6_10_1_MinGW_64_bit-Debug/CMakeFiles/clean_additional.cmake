# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\QtGames_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\QtGames_autogen.dir\\ParseCache.txt"
  "QtGames_autogen"
  )
endif()
