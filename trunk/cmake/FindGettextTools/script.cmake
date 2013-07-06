# ----- Set up variables.

# Read variables from the generated config.
include(${CMAKE_ARGV3})

# Transform keywords in to flags.
set(keywordArgs "")
foreach(keyword ${keywords})
  list(APPEND keywordArgs "--keyword=${keyword}")
endforeach()

# ----- Make the pot file.

message("Creating translation template...")

file(MAKE_DIRECTORY ${langDir})

set(potFile "${langDir}/${domain}.pot")

execute_process(COMMAND ${XGETTEXT_EXECUTABLE}
  "--output=${potFile}"
  "--omit-header" "--add-comments"
  ${keywordArgs}
  ${sourceFiles}
  WORKING_DIRECTORY ${sourcePrefix})

message(" '${domain}.pot' done.")

# ----- Copy and merge across the po files that come with the source.

message("Copying and updating stock translations...")

file(GLOB poFiles "${stockDir}/*.po")

foreach(file ${poFiles})
  # Get the language name, like en_US or zh_CN from the name of the po file, so
  # 'en_US.po' or 'zh_CN.po' become 'en_US' or 'zh_CN.po'
  get_filename_component(langName ${file} NAME_WE)
  
  set(newFile "${langDir}/${langName}.po")
  
  if(NOT EXISTS ${newFile})
    execute_process(COMMAND ${MSGMERGE_EXECUTABLE}
      "--output-file" ${newFile} ${file} ${potFile}
      OUTPUT_QUIET ERROR_VARIABLE error RESULT_VARIABLE ret)
    
    if(ret) # Have to do this hack as msgmerge prints to stderr.
      message(SEND_ERROR "${error}")
    endif()
    
    message(" '${langName}' copied.")
  elseif(${file} IS_NEWER_THAN ${newFile})
     execute_process(COMMAND ${MSGMERGE_EXECUTABLE}
       "--update" ${newFile} ${file}
       OUTPUT_QUIET ERROR_VARIABLE error RESULT_VARIABLE ret)
     
     if(ret) # Have to do this hack as msgmerge prints to stderr.
       message(SEND_ERROR "${error}")
     endif()
     
     message(" '${langName}' merged.")
  endif()
endforeach()

# ----- Process the files in to mo files.

message("Compiling translations...")

file(GLOB localPoFiles "${langDir}/*.po")

foreach(file ${localPoFiles})
  execute_process(COMMAND ${MSGMERGE_EXECUTABLE}
    "--update" ${file} ${potFile}
    OUTPUT_QUIET ERROR_VARIABLE error RESULT_VARIABLE ret)
  
  if(ret) # Have to do this hack as msgmerge prints to stderr.
    message(SEND_ERROR "${error}")
  endif()
  
  get_filename_component(langName ${file} NAME_WE)
  
  set(binaryFile "${hierarchy}")
  string(REPLACE "{1}" "${outDir}"   binaryFile "${binaryFile}")
  string(REPLACE "{2}" "${langName}" binaryFile "${binaryFile}")
  string(REPLACE "{3}" "LC_MESSAGES" binaryFile "${binaryFile}")
  string(REPLACE "{4}" "${domain}"   binaryFile "${binaryFile}")
  
  if(${file} IS_NEWER_THAN ${binaryFile})
    get_filename_component(binaryDir ${binaryFile} PATH)
    
    file(MAKE_DIRECTORY ${binaryDir})
    
    execute_process(COMMAND ${MSGFMT_EXECUTABLE}
      ${file} "--output-file" ${binaryFile}
      OUTPUT_QUIET ERROR_VARIABLE error RESULT_VARIABLE ret)
    
    if(ret) # Have to do this hack as msgfmt prints to stderr.
      message(SEND_ERROR "${error}")
    endif()
    
    message(" '${langName}' done.")
  endif()
endforeach()
