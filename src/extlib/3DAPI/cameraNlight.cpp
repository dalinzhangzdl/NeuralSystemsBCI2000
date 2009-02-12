///////////////////////////////////////////////////////////////////////////////////////
// $Id$
// Authors: shzeng, schalk@wadsworth.org
// Description: This "cameraNLight" class encapsulate the camera and the light in the
//   3D environment.
//
// (C) 2000-2009, BCI2000 Project
// http://www.bci2000.org
///////////////////////////////////////////////////////////////////////////////////////
#include "PCHIncludes.h"
#pragma hdrstop

#include "cameraNLight.h"
#include "component.h"
#include <algorithm>

using namespace std;

perspectiveProj::perspectiveProj( GLfloat fieldOfView, GLfloat aspect, GLfloat depth )
{
  float zNear = 1,
        f = 1.0 / ::tan( fieldOfView * M_PI / 180.0 / 2.0 );
  mMatrix[ 0 ] = f / aspect;
  mMatrix[ 5 ] = f;
  mMatrix[ 10 ] = ( zNear + depth ) / ( zNear - depth );
  mMatrix[ 11 ] = -1;
  mMatrix[ 14 ] = 2 * zNear * depth / ( zNear - depth );
}

flatProj::flatProj( GLfloat height, GLfloat width, GLfloat depth )
{
  float zNear = -1;
  mMatrix[ 0 ] = 2 / width;
  mMatrix[ 5 ] = 2 / height;
  mMatrix[ 10 ] = 2 / ( zNear - depth );
  mMatrix[ 14 ] = ( zNear + depth ) / ( zNear - depth );
  mMatrix[ 15 ] = 1;
}

void
cameraNLight::setBoundingBox( const cuboid& inCuboid )
{
  sceneDimensions = inCuboid;
  sceneDimensions.setXAxis( sceneDimensions.getXAxis() * inCuboid.getWidth() );
  sceneDimensions.setYAxis( sceneDimensions.getYAxis() * inCuboid.getHeight() );
  sceneDimensions.setZAxis( sceneDimensions.getZAxis() * inCuboid.getDepth() );
}

void
cameraNLight::apply() const
{
  //set up the light
  if( quality > 0 )
  {
    ::glEnable( GL_DEPTH_TEST );
    ::glDepthFunc( GL_LEQUAL );
    ::glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
    ::glShadeModel( GL_SMOOTH );

    GLfloat ambBri = clamp( ambLightBri );
    GLfloat ambLight[] =
    {
      ambBri * lightColorR,
      ambBri * lightColorG,
      ambBri * lightColorB,
      1
    };
    glLightfv( GL_LIGHT0, GL_AMBIENT, ambLight ); // Setup The Ambient Light

    GLfloat ligBri = clamp( lightBri );
    GLfloat spec[] =
    {
      ligBri * lightColorR,
      ligBri * lightColorG,
      ligBri * lightColorB,
      1
    };

    glLightfv( GL_LIGHT0, GL_DIFFUSE, spec );  // Setup The Specular Light
    glLightfv( GL_LIGHT0, GL_SPECULAR, spec ); // Setup The Specular Light

    GLfloat lightPos[] =
    {
      lightX,
      lightY,
      lightZ,
      0
    };
    glLightfv( GL_LIGHT0, GL_POSITION, lightPos ); // Position The Light

    // shade
    //glMaterialfv( GL_FRONT, GL_DIFFUSE, spec );
    glMaterialfv( GL_FRONT, GL_SPECULAR, spec );
    glMaterialf( GL_FRONT, GL_SHININESS, 127 );

    glMaterialfv( GL_FRONT, GL_AMBIENT, ambLight );
    glColorMaterial( GL_FRONT, GL_AMBIENT );//_AND_DIFFUSE );

    glEnable( GL_LIGHT0 );                  // Enable light zero
    glEnable( GL_LIGHTING );
    glEnable( GL_COLOR_MATERIAL );
  }
  
  glMatrixMode( GL_COLOR );
  glLoadIdentity();

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  gluLookAt(
    camViewX, camViewY, camViewZ,
    camAimX, camAimY, camAimZ,
    camUpX, camUpY, camUpZ
  );

  glMatrixMode( GL_PROJECTION );
  // estimate a reasonable value for the maximum scene depth
  CVector3 camPos = { camViewX, camViewY, camViewZ },
           camDir = { camAimX - camViewX, camAimY - camViewY, camAimZ - camViewZ },
           dirProj = { sceneDimensions.getXAxis() * camDir,
                       sceneDimensions.getYAxis() * camDir,
                       sceneDimensions.getZAxis() * camDir },
           sceneDir = sceneDimensions.getOrigin() - camPos;
  GLfloat sceneDepth = 0;
  if( Length( camDir ) > 0 )
    sceneDepth = ( dirProj * camDir ) / ( camDir * camDir )
               + 1/Length( camDir ) * camDir * sceneDir;
  // determine the window's aspect ratio
  int viewport[] = { -1, -1, -1, -1 };
  glGetIntegerv( GL_VIEWPORT, viewport );
  GLfloat aspect = GLfloat( viewport[2] ) / GLfloat( viewport[3] );

  if( fieldOfView == 0 )
  {
    GLfloat maxDim = max( Length( sceneDimensions.getXAxis() ), Length( sceneDimensions.getYAxis() ) );
    glLoadMatrixf( projection::flat( maxDim, aspect * maxDim, sceneDepth ).getMatrix() );
  }
  else
  {
    glLoadMatrixf( projection::perspective( fieldOfView, aspect, sceneDepth ).getMatrix() );
  }
}

