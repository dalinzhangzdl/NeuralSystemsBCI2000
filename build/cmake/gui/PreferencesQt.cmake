###########################################################################
## $Id$
## Authors: juergen.mellinger@uni-tuebingen.de
## Description: Sets up CMAKE variables for importing PreferencesQt
## SETS:
##       SRC_GUI - Required source files for the PreferencesQt
##       HDR_GUI - Required header files for the PreferencesQt
##       INC_GUI - Include directory for the PreferencesQt
##       MOC_GUI - Files which require MOCing
##       UI_GUI  - UI Files for wrapping

# Define the source files
SET( SRC_GUI
  ${BCI2000_SRC_DIR}/core/Operator/OperatorQt/Preferences.cpp
)

# Define the headers
SET( HDR_GUI
  ${BCI2000_SRC_DIR}/core/Operator/OperatorQt/Preferences.h
)

# Wrap MOCable sources for this GUI
SET( MOC_GUI
)

# Set UI Files which need wrapping
SET( UI_GUI
)

# Define the include directory
SET( INC_GUI 
  ${BCI2000_SRC_DIR}/core/Operator/OperatorQt
)

# Set success
SET( GUI_OK TRUE )
