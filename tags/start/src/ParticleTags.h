//////////////////////////////////////////////////////////////////
// (c) Copyright 2003- by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//   Tel:    217-244-6319 (NCSA) 217-333-3324 (MCC)
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//   Department of Physics, Ohio State University
//   Ohio Supercomputer Center
//////////////////////////////////////////////////////////////////
// -*- C++ -*-

/**@file ParticleTags.h
 *@brief Declaration of the particle-related tags
 */
#ifndef OHMMS_PARTICLETAGS_H
#define OHMMS_PARTICLETAGS_H

#include <string>

/** define the tags for the data types and attributes of particle classes
 *
 *The tags are grouped into
 *- the data type of an attribute (type name)
 *- the name of an attribute (object name)
 *- xml node and attribute names
 */
struct ParticleTags {

  /** enumeraton for datatypes for particle attributes 
   *
   *Each attribute of type T is stored in a container ParticleAttrib<T>.
   *ParticleBase::AttribTypeMap is used to map the string and the enumeration.
   */
  enum {PA_IndexType=0, /*!< index (int) type */
	PA_ScalarType,  /*!< scalar (float|double) type */
	PA_StringType,  /*!< string type */
	PA_PositionType,/*!< position (D-dim vector) type */
	PA_TensorType /*!< tensor type */
  };

  ///the tag for uninitialized datatype and name of attributes
  static std::string null_tag;

  /** @defgroup datatype_tags tags for particle attribute types
   *@{*/
  ///the datatype tag for the index-type attribute
  static std::string indextype_tag;

  ///the datatype tag for the scalar-type attribute
  static std::string scalartype_tag;

  ///the datatype tag for the string-type attribute
  static std::string stringtype_tag;

  ///the datatype tag for the pos-type attribute
  static std::string postype_tag;

  ///the datatype tag for the tensor-type attribute
  static std::string tensortype_tag;

  ///the datatype tag for the xmol-type attribute
  static std::string xmoltype_tag;
  /*@}*/

  /** @defgroup name_tags tags for particle attribute names
    *@{
    */
  ///the name for position attribute
  static std::string position_tag;

  ///the name for id attribute
  static std::string id_tag;

  ///the name for ionid attribute
  static std::string ionid_tag;

  ///the name for trajectory attribute
  static std::string trajectory_tag;

  ///the name for force attribute
  static std::string force_tag;

  ///the name for velocity attribute
  static std::string velocity_tag;

  ///the name for energy attribute
  static std::string energy_tag;

  ///the name for sumbc (accumulative boundary crossings) attribute
  static std::string sumbc_tag;
  /*@}*/

  /** @defgroup xml_tags xml tags for particle and its attributes
    *@{
    */
  ///the name of particle node
  static std::string root_tag;    
  ///the name of particle attribute node
  static std::string attrib_tag;    
  ///the attribute name of name
  static std::string name_tag;    
  ///the attribute name of datatype
  static std::string datatype_tag;    
  ///the attribute name of condition
  static std::string condition_tag;
  ///the attribute name of size
  static std::string size_tag;
  ///the attribute name of format
  static std::string format_tag;
  ///the attribute name of role
  static std::string role_tag;
  /*@}*/
};
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
