###########################################################################
## $Id$
## Authors: griffin.milsap@gmail.com
## Description: Contains a macro for creating a commandline application

MACRO( BCI2000_ADD_TOOLS_CMDLINE ) # NAME SOURCES [HEADERS] REQUESTQT
  BCI2000_PARSE_ARGS( "NAME;SOURCES" ${ARGV} )
  LIST( REVERSE SOURCES )
  LIST( GET SOURCES 0 USEQT )
  LIST( REMOVE_AT SOURCES 0 )

  MESSAGE( "-- Adding Commandline Project: " ${NAME} )
  
  SET( SOURCES
    ${SOURCES}
    ${BCI2000_SRC_DIR}/core/Tools/cmdline/bci_tool.cpp
    ${BCI2000_SRC_DIR}/shared/bcistream/BCIStream_tool.cpp
  )

  INCLUDE( ${BCI2000_CMAKE_DIR}/frameworks/Core.cmake )
  
  SET_OUTPUT_DIRECTORY( "${BCI2000_ROOT_DIR}/tools/cmdline" )
  BCI2000_ADD_TARGET( EXECUTABLE ${NAME} ${SOURCES} )
  
  IF( ${USEQT} )
    MESSAGE( "-- (NB: ${NAME} is using Qt)" )
    SET_PROPERTY( TARGET ${NAME} APPEND PROPERTY COMPILE_FLAGS "-DUSE_QT" )
    TARGET_LINK_LIBRARIES( ${NAME} ${QT_LIBRARIES} )
  ENDIF()
  BCI2000_ADD_BCITEST( ${NAME} )

ENDMACRO()

################################################################################################

MACRO( BCI2000_ADD_CMDLINE_CONVERTER NAME )

  SET( SOURCES
    ${NAME}.cpp
    ${BCI2000_SRC_DIR}/core/Tools/cmdline/bci_tool.cpp
  )
  BCI2000_ADD_TOOLS_CMDLINE( ${NAME} ${SOURCES} FALSE )
  
ENDMACRO( BCI2000_ADD_CMDLINE_CONVERTER NAME )

################################################################################################

MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})    
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})            
    SET(larg_names ${arg_names})    
    LIST(FIND larg_names "${arg}" is_arg_name)                   
    IF (is_arg_name GREATER -1)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name GREATER -1)
      SET(loption_names ${option_names})    
      LIST(FIND loption_names "${arg}" is_option)            
      IF (is_option GREATER -1)
       SET(${prefix}_${arg} TRUE)
      ELSE (is_option GREATER -1)
       SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option GREATER -1)
    ENDIF (is_arg_name GREATER -1)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PARSE_ARGUMENTS)

################################################################################################

MACRO( BCI2000_ADD_CMDLINE_FILTER )

  PARSE_ARGUMENTS(
    CMDLINEFILTER
    "FROM;EXTRA_SOURCES;EXTRA_HEADERS;USING;INCLUDING"
    ""
    ${ARGN}
  )
  LIST( GET CMDLINEFILTER_DEFAULT_ARGS 0 NAME )
  LIST( REMOVE_AT CMDLINEFILTER_DEFAULT_ARGS 0 )
  LIST( LENGTH CMDLINEFILTER_DEFAULT_ARGS NARGS )
  IF( ${NARGS} GREATER 0 )
    MESSAGE( "- WARNING: BCI2000_ADD_CMDLINE_FILTER is ignoring extraneous arguments: " ${CMDLINEFILTER_DEFAULT_ARGS} )
  ENDIF( ${NARGS} GREATER 0 )
    
  SET( MAINSTEM ${NAME} )
  IF( NOT ${CMDLINEFILTER_FROM} STREQUAL "" )
    SET( MAINSTEM ${CMDLINEFILTER_FROM}/${MAINSTEM} )
  ENDIF( NOT ${CMDLINEFILTER_FROM} STREQUAL "" )
  #MESSAGE( "want to build command-line filter ${NAME} from ${CMDLINEFILTER_FROM} based on ${MAINSTEM}.cpp" )
  
  SET( SOURCES
    ${MAINSTEM}.cpp
    ${BCI2000_SRC_DIR}/core/Tools/cmdline/bci_tool.cpp
    ${BCI2000_SRC_DIR}/core/Tools/cmdline/bci_filtertool.cpp
    ${CMDLINEFILTER_EXTRA_SOURCES}
    ${CMDLINEFILTER_EXTRA_HEADERS}
  )

  SET( USEQT FALSE )
  FOREACH( DEPENDENCY ${CMDLINEFILTER_USING} ${CMDLINEFILTER_INCLUDING} )
    IF( ${DEPENDENCY} STREQUAL QT )
      SET( USEQT TRUE )
    ELSE( ${DEPENDENCY} STREQUAL QT )
      BCI2000_INCLUDE( ${DEPENDENCY} )
    ENDIF( ${DEPENDENCY} STREQUAL QT )
  ENDFOREACH( DEPENDENCY )
  
  BCI2000_ADD_TOOLS_CMDLINE( ${NAME} ${SOURCES} ${USEQT} )

ENDMACRO( BCI2000_ADD_CMDLINE_FILTER )

################################################################################################
