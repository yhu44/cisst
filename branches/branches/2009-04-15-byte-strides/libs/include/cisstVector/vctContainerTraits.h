/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$
  
  Author(s):	Anton Deguet
  Created on:	2004-11-11

  (C) Copyright 2004-2007 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


/*! 
  \file 
  \brief Basic traits for the cisstVector containers
  \ingroup cisstVector
 */


#ifndef _vctContainerTraits_h
#define _vctContainerTraits_h

/*! Macro used to define multiple types based on the type of elements.
  This will define size_type, index_type, difference_type,
  stride_type, value_type, reference, const_reference, pointer,
  const_pointer, NormType (double) and AngleType (double).  Most of
  these types are introduced and named for STL compatibility.

  \param type Type of element, e.g. double, float, char.
 */
#define VCT_CONTAINER_TRAITS_TYPEDEFS(type) \
    typedef unsigned int size_type; \
    typedef unsigned int index_type; \
    typedef int difference_type; \
    typedef int stride_type; \
    typedef type value_type; \
    typedef value_type & reference; \
    typedef const value_type & const_reference; \
    typedef value_type * pointer; \
    typedef const value_type * const_pointer; \
    typedef double NormType; \
    typedef double AngleType; \
    typedef char byte_type; \
    typedef byte_type * byte_pointer


/*! Macro used to define nArray-specific types based on the type of
  elements.  This will define nsize_type, nstride_type, and
  dimension_type.
 */
#define VCT_NARRAY_TRAITS_TYPEDEFS(dimension) \
    typedef vctFixedSizeVector<size_type, dimension> nsize_type; \
    typedef vctFixedSizeVector<stride_type, dimension> nstride_type; \
    typedef vctFixedSizeVector<index_type, dimension> nindex_type; \
    typedef unsigned int dimension_type; \
    typedef vctFixedSizeVector<dimension_type, dimension> ndimension_type


#endif  // _vctContainerTraits_h

