cmake_minimum_required (VERSION 2.6)
project (sqtd)
set (sqtd_VERSION_MAJOR   0)
set (sqtd_VERSION_MINOR   1)
set (sqtd_VERSION_RELEASE 3)





#Set path to package components
set(
  BIN_DIR      "/usr/sbin" 
  CACHE  PATH  "The directory to put executable files"
)  

set(
MAN_DIR        "/usr/share/man"
  CACHE  PATH  "The directory to put man files"
)
set(
  DOC_DIR      "/usr/share/doc"
  CACHE  PATH  "The directory to put man files"
)

set(
LOCALE_DIR     "/usr/share/locale"
  CACHE  PATH  "The directory to put localization   mo files (gettext)"
)

set(
ETC_DIR        "/etc"
  CACHE  PATH  "The directory to put init scripts and configuretion files"
)

set(
VAR_LIB_DIR    "/var/lib"
  CACHE  PATH  "The directory to put temporary  socket and pid files"
)



set(sqtd_VERSION "${sqtd_VERSION_MAJOR}.${sqtd_VERSION_MINOR}.${sqtd_VERSION_RELEASE}")

set(CMAKE_EXE_LINKER_FLAGS -lpthread)



#Generate config.h file from template? fnd add build dir  to include path
configure_file (
"${PROJECT_SOURCE_DIR}/config.h.in"
"${PROJECT_BINARY_DIR}/config.h"
)
include_directories("${PROJECT_BINARY_DIR}")



#Make target for executable files, install it into BIN_DIR (/usr/sbin)  
add_executable(sqtd src/sqtd.cpp)
add_executable(sqtc src/sqtc.cpp)
install( TARGETS sqtd sqtc 
       RUNTIME DESTINATION ${BIN_DIR}
)


#Generate localization  files for gettext from source files

file(GLOB SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/*)
file(GLOB PO_FILES ${PROJECT_SOURCE_DIR}/po/*)
find_program(XGETTEXT xgettext)
if(NOT XGETTEXT)
    message( " Note: xgettext coud not be found. Can not generate sqtd.pot file  "  )
else(NOT XGETTEXT)
#generate pot template from source code
  add_custom_command(
     OUTPUT  ${PROJECT_SOURCE_DIR}/po/sqtd.pot
      COMMAND ${XGETTEXT} -k_ --from-code=utf-8 -d sqtd -o ${PROJECT_SOURCE_DIR}/po/sqtd.pot ${SOURCE_FILES}
     DEPENDS ${SOURCE_FILES}
  )
endif(NOT XGETTEXT)

#update existing po files for gettext
find_program(MSGMERGE msgmerge) 
if(NOT MSGMERGE)
    message( " Note: msgmerge coud not be found. Can not generate sqtyd.pot file  "  )
else(NOT MSGMERGE)
  file(GLOB PO_FILES ${PROJECT_SOURCE_DIR}/po/*.po)
  foreach( PO_FILE ${PO_FILES})
    add_custom_command(
       OUTPUT  ${PO_FILE}
       COMMAND ${MSGMERGE} -U  ${PO_FILE}   ${PROJECT_SOURCE_DIR}/po/sqtd.pot
       DEPENDS ${PROJECT_SOURCE_DIR}/po/sqtd.pot
    )
  endforeach(PO_FILE ${PO_FILES})
endif(NOT MSGMERGE)

#Compile  locacization mo files install it in LOCALE_DIR  
find_program(MSGFMT msgfmt)
if(NOT MSGFMT)
else(NOT MSGFMT)
  add_custom_target(localization ALL)
  foreach( PO_FILE ${PO_FILES})
    get_filename_component (LANG  ${PO_FILE} NAME_WE)  
    add_custom_command(
      TARGET  localization
      COMMAND ${MSGFMT} -c -v -o ${PROJECT_BINARY_DIR}/${LANG}.mo  ${PO_FILE}
      DEPENDS ${PROJECT_SOURCE_DIR}/po/ru.po
    )
    install( FILES ${PROJECT_BINARY_DIR}/${LANG}.mo  
              DESTINATION ${LOCALE_DIR}/${LANG}/LC_MESSAGES/sqtd.mo)
  endforeach( PO_FILE ${PO_FILES})
endif(NOT MSGFMT)

#This is the end of localization



#generate man pages
find_program(BZIP2_EXECUTABLE bzip2)
file(GLOB MAN_FILES ${PROJECT_SOURCE_DIR}/man/*)
IF(NOT BZIP2_EXECUTABLE)
   message( " Note: bzip2 coud not be found. Can not compress man. Man can not be installed.")
ELSE(NOT BZIP2_EXECUTABLE)
   add_custom_target(man_pages ALL  DEPENDS ${MAN_FILES} )
   foreach( MAN_FILE ${MAN_FILES} )
      get_filename_component (CURRENT_MAN  ${MAN_FILE} NAME)
      string(FIND ${CURRENT_MAN} "." POS)
      string(SUBSTRING ${CURRENT_MAN} 0 ${POS} MAN_NAME) 
      string(REPLACE "${MAN_NAME}." "" CURRENT_MAN ${CURRENT_MAN})
      string(FIND ${CURRENT_MAN} "." POS)
      string(SUBSTRING ${CURRENT_MAN} 0 ${POS} MAN_LANG)
      string(REPLACE "${MAN_LANG}." "" CURRENT_MAN ${CURRENT_MAN})
      string(FIND ${CURRENT_MAN} "." POS)
      string(SUBSTRING ${CURRENT_MAN} 0 ${POS} MAN_SECTION)
      set (INSTALL_PATH ${MAN_DIR}/${MAN_LANG}/man${MAN_SECTION})  
      
      get_filename_component (CURRENT_MAN  ${MAN_FILE} NAME)
      add_custom_command(
	TARGET man_pages
	COMMAND  ${CMAKE_COMMAND} -E remove  ${PROJECT_BINARY_DIR}/man/${CURRENT_MAN}.bz2
	COMMAND  ${CMAKE_COMMAND} -E copy  ${MAN_FILE} ${PROJECT_BINARY_DIR}/man/${CURRENT_MAN}
	COMMAND  ${BZIP2_EXECUTABLE}   ${PROJECT_BINARY_DIR}/man/${CURRENT_MAN}
	DEPENDS ${MAN_FILE}
      )
     
     install( FILES ${PROJECT_BINARY_DIR}/man/${CURRENT_MAN}.bz2  
              DESTINATION ${INSTALL_PATH}
	      RENAME ${MAN_NAME}.${MAN_SECTION}.bz2
     )
   endforeach( MAN_FILE ${MAN_FILES} )
ENDIF(NOT BZIP2_EXECUTABLE)

#The end of generation man pages


#Installing some other source files 
set(DOCINSTALLDIR=${DOC_DIR}/sqtd-${sqtd_VERSION})
install( FILES ${PROJECT_SOURCE_DIR}/doc/sqtd.conf.example  
         DESTINATION ${DOC_DIR}/sqtd-${sqtd_VERSION}  )



install( DIRECTORY ${PROJECT_SOURCE_DIR}/etc/conf.d ${PROJECT_SOURCE_DIR}/etc/init.d    
         DESTINATION ${ETC_DIR} )

install( FILES ${PROJECT_SOURCE_DIR}/doc/sqtd.conf.example  
         DESTINATION ${ETC_DIR}/sqtd/ )

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/var/sqtd)

install ( DIRECTORY ${PROJECT_BINARY_DIR}/var/sqtd
          DESTINATION ${VAR_LIB_DIR} 
)

#Adding unininstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)
add_custom_target(uninstall  
                  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)


