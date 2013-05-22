###########################################################################
## $Id$
## Authors: griffin.milsap@gmail.com
## Description: Sets up CMAKE variables for importing EditMatrixQt
## SETS:
##       SRC_GUI - Required source files for the EditMatrixQt
##       HDR_GUI - Required header files for the EditMatrixQt
##       INC_GUI - Include directory for the EditMatrixQt
##       MOC_GUI - Files which require MOCing
##       UI_GUI  - UI Files for wrapping

# Define the source files
SET( SRC_GUI
  ${BCI2000_SRC_DIR}/core/Operator/OperatorQt/EditMatrix.cpp
)

# Define the headers
SET( HDR_GUI
  ${BCI2000_SRC_DIR}/core/Operator/OperatorQt/EditMatrix.h
)

# Wrap MOCable sources for this GUI
SET( MOC_GUI
  ${BCI2000_SRC_DIR}/core/Operator/OperatorQt/EditMatrix.h
)

# Set UI Files which need wrapping
SET( UI_GUI
  ${BCI2000_SRC_DIR}/core/Operator/OperatorQt/EditMatrix.ui
)

# Define the include directory
SET( INC_GUI 
  ${BCI2000_SRC_DIR}/core/Operator/OperatorQt
  ${BCI2000_SRC_DIR}/shared/gui/                      
)

# Set success
SET( GUI_OK TRUE )
