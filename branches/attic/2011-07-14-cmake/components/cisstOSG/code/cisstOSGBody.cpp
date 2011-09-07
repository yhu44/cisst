/*

  Author(s): Simon Leonard
  Created on: Nov 11 2009

  (C) Copyright 2008 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <osgDB/ReadFile> 
#include <osg/PolygonMode>
#include <osg/Material>

#include <cisstOSG/cisstOSGBody.h>

// This is called at each update traversal
void cisstOSGBody::TransformCallback::operator()( osg::Node* node, 
						  osg::NodeVisitor* nv ){
  osg::Referenced* data = node->getUserData();
  cisstOSGBody::UserData* userdata;
  userdata = dynamic_cast<cisstOSGBody::UserData*>( data );

  // change the transform 
  if( userdata != NULL )
    { userdata->GetBody()->UpdateTransform(); }
  traverse( node, nv );

}   

// This is called at each update traversal
void cisstOSGBody::SwitchCallback::operator()( osg::Node* node, 
					       osg::NodeVisitor* nv ){

  osg::Referenced* data = node->getUserData();
  cisstOSGBody::UserData* userdata;
  userdata = dynamic_cast<cisstOSGBody::UserData*>( data );

  // change the switch
  if( userdata != NULL )
    { userdata->GetBody()->UpdateSwitch(); }
  traverse( node, nv );

}   


cisstOSGBody::cisstOSGBody( const std::string& model, 
			    const vctFrame4x4<double>& Rt ) :
  transform( Rt ),
  onoff( SWITCH_ON ){
  
  Initialize();
  ReadModel( model );

}

cisstOSGBody::cisstOSGBody( const std::string& model, 
			    cisstOSGWorld* world,
			    const vctFrame4x4<double>& Rt ) :
  transform( Rt ),
  onoff( SWITCH_ON ){
  
  Initialize();
  ReadModel( model );

  // Once this is done add the body to the world
  if( world != NULL )
    { world->addChild( this ); }

}

cisstOSGBody::cisstOSGBody( const std::string& model, 
			    cisstOSGWorld* world,
			    const vctFrm3& Rt ):
  onoff( SWITCH_ON ){

  // Hack to avoid non-normalized rotations!
  const vctMatrixRotation3<double>& R = Rt.Rotation();
  vctQuaternionRotation3<double> q( R, VCT_NORMALIZE );
  transform = vctFrame4x4<double>( q, Rt.Translation() );

  Initialize();
  ReadModel( model );

  // Once this is done add the body to the world
  if( world != NULL )
    { world->addChild( this ); }

}

cisstOSGBody::cisstOSGBody( const vctDynamicMatrix<double>& pointcloud,
			    cisstOSGWorld* world,
			    const vctFrm3& Rt,
			    const vctFixedSizeVector<unsigned char,3>& rgb ) :
  onoff( SWITCH_ON ){

  // Hack to avoid non-normalized rotations!
  const vctMatrixRotation3<double>& R = Rt.Rotation();
  vctQuaternionRotation3<double> q( R, VCT_NORMALIZE );
  transform = vctFrame4x4<double>( q, Rt.Translation() );

  Initialize();
  Read3DData( pointcloud, rgb );

  if( world != NULL )
    { world->addChild( this ); }
 
}


cisstOSGBody::~cisstOSGBody(){}

void cisstOSGBody::Initialize(){

  // always moving  
  setDataVariance( osg::Object::DYNAMIC );
  
  // Setup the user data for this body. This can be used to recover the body
  // from callbacks
  userdata = new cisstOSGBody::UserData( this );
  this->setUserData( userdata );

  // Create and configure the transform node
  osgtransform = new osg::MatrixTransform;
  osgtransform->setUserData( userdata );

  // Add an update callback to the transform
  transformcallback = new cisstOSGBody::TransformCallback;
  osgtransform->setUpdateCallback( transformcallback );

  // Create and configure the switch node
  osgswitch = new osg::Switch();
  osgswitch->setUserData( userdata );

  // Add an update callback to the switch
  switchcallback = new cisstOSGBody::SwitchCallback;
  osgswitch->setUpdateCallback( switchcallback );

  // add the transform as the child of the switch
  osgswitch->addChild( osgtransform );
  
  // add the switch as the child of the main node
  this->addChild( osgswitch );
  
  SetTransform( transform );

}

void cisstOSGBody::ReadModel( const std::string& model ){

  osg::ref_ptr< osgDB::ReaderWriter::Options > options;
  // "noRotation" is to cancel the default -X in .obj files
  //options = new osgDB::ReaderWriter::Options("noRotation");
  options = new osgDB::ReaderWriter::Options();

  std::string path;
  size_t found;
#if (CISST_OS == CISST_WINDOWS)
  found = model.rfind( '/' );
#else
  found = model.rfind( '/' );
#endif    

  if( found != std::string::npos )
    { path.assign( model, 0, found ); }
  options->setDatabasePath( path );

  osg::ref_ptr<osg::Node> node = osgDB::readNodeFile( model, options );

  if( node != NULL ){
    // Add the node to the transformation node
    osgtransform->addChild( node );
  }
  else{
    CMN_LOG_RUN_ERROR << "Failed to create node for: " << model << std::endl;
  }

}

void cisstOSGBody::Read3DData( const vctDynamicMatrix<double>& pc,
			       const vctFixedSizeVector<unsigned char,3>& RGB ){
  
  size_t npoints= 0;
  if( pc.rows() == 3 )
    { npoints = pc.cols(); }
  else{
    // invalid data
  }
  
  // First, create a geode
  osg::ref_ptr<osg::Geode> geode;
  try{ geode = new osg::Geode; }
  catch( std::bad_alloc& ){ 
    CMN_LOG_RUN_ERROR << CMN_LOG_DETAILS
		      << "Failed to create a geode." 
		      << std::endl;
  }
  osgtransform->addChild( geode );

  // then create a geometry
  osg::ref_ptr<osg::Geometry> pointsGeom;
  try{ pointsGeom = new osg::Geometry; }
  catch( std::bad_alloc& ){ 
    CMN_LOG_RUN_ERROR << CMN_LOG_DETAILS
		      << "Failed to create a points geometry." 
		      << std::endl;
  }
  // add the geometry to the geode
  geode->addDrawable( pointsGeom );

  // Create an array primitive set 
  osg::ref_ptr<osg::DrawArrays> drawArrayPoints;
  try{ drawArrayPoints = new osg::DrawArrays( osg::PrimitiveSet::POINTS ); }
  catch( std::bad_alloc& ){ 
    CMN_LOG_RUN_ERROR << CMN_LOG_DETAILS
		      << "Failed to create array of points."
		      << std::endl;
  }
  // add the set to the geometry
  pointsGeom->addPrimitiveSet( drawArrayPoints );

  // add a vector of vertices
  osg::ref_ptr<osg::Vec3Array> vertexData;
  try{ vertexData = new osg::Vec3Array; }
  catch( std::bad_alloc& ){ 
    CMN_LOG_RUN_ERROR << CMN_LOG_DETAILS
		      << "Failed to create vertices array." 
		      << std::endl;
  }
  pointsGeom->setVertexArray( vertexData );
  
  for( size_t i=0; i<npoints; i++ )
    { vertexData->push_back( osg::Vec3( pc[0][i], pc[1][i], pc[2][i] ) ); }

  osg::ref_ptr<osg::Vec4Array> colorArray = new osg::Vec4Array;
  colorArray->push_back( osg::Vec4( RGB[0], RGB[1], RGB[2], 1.0f ) );
  
  osg::ref_ptr< osg::TemplateIndexArray< unsigned int, 
					 osg::Array::UIntArrayType,
					 4, 1 > > colorIndexArray;
  colorIndexArray = new osg::TemplateIndexArray< unsigned int, 
						 osg::Array::UIntArrayType,
						 4, 1>;
  colorIndexArray->push_back(0);

  pointsGeom->setColorArray( colorArray );
  pointsGeom->setColorIndices(colorIndexArray);
  pointsGeom->setColorBinding(osg::Geometry::BIND_OVERALL);

  drawArrayPoints->setFirst( 0 );
  drawArrayPoints->setCount( vertexData->size() );

}

// This is called from the body's callback
// This reads a transformation if the body is connected to an interface
void cisstOSGBody::UpdateTransform(){

  vctFrame4x4<double> Rt( transform );
  osgtransform->setMatrix( osg::Matrix ( Rt[0][0], Rt[1][0], Rt[2][0], 0.0,
					 Rt[0][1], Rt[1][1], Rt[2][1], 0.0,
					 Rt[0][2], Rt[1][2], Rt[2][2], 0.0,
					 Rt[0][3], Rt[1][3], Rt[2][3], 1.0 ) );
}

void cisstOSGBody::UpdateSwitch(){}

void cisstOSGBody::SetTransform( const vctFrame4x4<double>& Rt )
{ transform = Rt; }

void cisstOSGBody::SetTransform( const vctFrm3& Rt ){
  // Hack to avoid non-normalized rotations!
  const vctMatrixRotation3<double>& R = Rt.Rotation();
  vctQuaternionRotation3<double> q( R, VCT_NORMALIZE );
  SetTransform( vctFrame4x4<double>( q, Rt.Translation() ) );
}

//! Set the switch of the body
void cisstOSGBody::SwitchOn()
{ onoff = SWITCH_ON; }

void cisstOSGBody::SwitchOff()
{ onoff = SWITCH_OFF; }
  
void cisstOSGBody::SetModeFill(){
  osg::ref_ptr<osg::StateSet> state = getOrCreateStateSet();
  osg::ref_ptr<osg::PolygonMode> pm;
  pm = dynamic_cast<osg::PolygonMode*>
    ( state->getAttribute( osg::StateAttribute::POLYGONMODE ));
  if( !pm ){
    pm = new osg::PolygonMode;
    state->setAttribute( pm );
  }
  pm->setMode( osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL );
}

void cisstOSGBody::SetModeLine(){
  osg::ref_ptr<osg::StateSet> state = getOrCreateStateSet();
  osg::ref_ptr<osg::PolygonMode> pm;
  pm = dynamic_cast<osg::PolygonMode*>
    ( state->getAttribute( osg::StateAttribute::POLYGONMODE ));
  if( !pm ){
    pm = new osg::PolygonMode;
    state->setAttribute( pm );
  }
  pm->setMode( osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE );
}

void cisstOSGBody::SetModePoint(){
  osg::ref_ptr<osg::StateSet> state = getOrCreateStateSet();
  osg::ref_ptr<osg::PolygonMode> pm;
  pm = dynamic_cast<osg::PolygonMode*>
    ( state->getAttribute( osg::StateAttribute::POLYGONMODE ));
  if( !pm ){
    pm = new osg::PolygonMode;
    state->setAttribute( pm );
  }
  pm->setMode( osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::POINT );
}

