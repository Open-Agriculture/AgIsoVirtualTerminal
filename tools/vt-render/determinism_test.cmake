# Checks that vt-render's output depends only on its input.
#
#   cmake -DVT_RENDER=<vt-render> -DCOLLECTION=<AgIsoObjectPoolCollection checkout> -DWORK_DIR=<scratch folder>
#         -P determinism_test.cmake
#
# 1. Renders the whole collection twice with --all: every file must be byte for byte the same.
# 2. Renders every pool on its own with --pool, in a fresh process: every file must match what --all wrote,
#    where other pools had been rendered before it. This catches state carried from one render to the next.
# 3. Renders a pool that cannot be parsed: exit code 2, and a manifest.json that reports the error.

cmake_minimum_required(VERSION 3.19)

foreach(variable VT_RENDER COLLECTION WORK_DIR)
  if(NOT ${variable})
    message(FATAL_ERROR "${variable} is not set")
  endif()
endforeach()

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

# Fails unless both folders hold the same files with the same content
function(compare_folders expected actual)
  file(GLOB_RECURSE expectedFiles RELATIVE "${expected}" "${expected}/*")
  file(GLOB_RECURSE actualFiles RELATIVE "${actual}" "${actual}/*")
  list(SORT expectedFiles)
  list(SORT actualFiles)
  if(NOT expectedFiles STREQUAL actualFiles)
    message(FATAL_ERROR "${expected} and ${actual} hold different files")
  endif()
  foreach(file IN LISTS expectedFiles)
    file(SHA256 "${expected}/${file}" expectedHash)
    file(SHA256 "${actual}/${file}" actualHash)
    if(NOT expectedHash STREQUAL actualHash)
      message(FATAL_ERROR "${file} differs between ${expected} and ${actual}")
    endif()
  endforeach()
endfunction()

function(render_all output resultVariable)
  execute_process(
    COMMAND "${VT_RENDER}" --all "${COLLECTION}" --out "${output}"
    RESULT_VARIABLE result
    OUTPUT_QUIET)
  if(NOT result MATCHES "^[012]$")
    message(FATAL_ERROR "vt-render --all exited with ${result}")
  endif()
  set(${resultVariable} ${result} PARENT_SCOPE)
endfunction()

message(STATUS "Rendering the collection, first run")
render_all("${WORK_DIR}/run1" firstResult)
message(STATUS "Rendering the collection, second run")
render_all("${WORK_DIR}/run2" secondResult)
if(NOT firstResult EQUAL secondResult)
  message(FATAL_ERROR "The exit code changed between runs: ${firstResult}, then ${secondResult}")
endif()

file(GLOB_RECURSE images "${WORK_DIR}/run1/*.png")
list(LENGTH images imageCount)
if(imageCount EQUAL 0)
  message(FATAL_ERROR "The collection rendered no images")
endif()
compare_folders("${WORK_DIR}/run1" "${WORK_DIR}/run2")
message(STATUS "${imageCount} images are identical in both runs")

file(GLOB poolFolders LIST_DIRECTORIES true "${COLLECTION}/pools/*/*")
list(SORT poolFolders)
set(index 0)
foreach(poolFolder IN LISTS poolFolders)
  if(NOT EXISTS "${poolFolder}/pool.iop")
    continue()
  endif()
  math(EXPR index "${index} + 1")
  set(output "${WORK_DIR}/single/${index}")
  execute_process(
    COMMAND "${VT_RENDER}" --pool "${poolFolder}/pool.iop" --meta "${poolFolder}/meta.yaml" --out "${output}"
    RESULT_VARIABLE result
    OUTPUT_QUIET)
  if(NOT result MATCHES "^[012]$")
    message(FATAL_ERROR "vt-render --pool ${poolFolder} exited with ${result}")
  endif()
  file(READ "${output}/manifest.json" manifest)
  string(JSON poolId GET "${manifest}" pool_id)
  compare_folders("${WORK_DIR}/run1/${poolId}" "${output}")
endforeach()
message(STATUS "${index} pools rendered on their own match the collection run")

set(brokenPool "${WORK_DIR}/broken")
file(MAKE_DIRECTORY "${brokenPool}")
file(WRITE "${brokenPool}/pool.iop" "this is not an object pool")
file(WRITE "${brokenPool}/meta.yaml" "id: \"0badf00d\"\nname: broken\nrender:\n  data_mask_size: TODO\n")
execute_process(
  COMMAND "${VT_RENDER}" --pool "${brokenPool}/pool.iop" --meta "${brokenPool}/meta.yaml" --out "${brokenPool}/out"
  RESULT_VARIABLE result
  OUTPUT_QUIET)
if(NOT result EQUAL 2)
  message(FATAL_ERROR "A pool that cannot be parsed exited with ${result}, not 2")
endif()
file(READ "${brokenPool}/out/manifest.json" manifest)
string(JSON loaded GET "${manifest}" load ok)
string(JSON loadError GET "${manifest}" load error)
if(loaded OR NOT loadError)
  message(FATAL_ERROR "The manifest of a pool that cannot be parsed does not report the error")
endif()
message(STATUS "A pool that cannot be parsed exits with 2 and reports: ${loadError}")
