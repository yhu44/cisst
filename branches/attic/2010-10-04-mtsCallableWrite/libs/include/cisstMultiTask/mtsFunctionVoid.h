/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Peter Kazanzides, Anton Deguet

  (C) Copyright 2007-2008 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/



/*!
  \file
  \brief Defines a function object to use a void command (mtsCommandVoid)
*/

#ifndef _mtsFunctionVoid_h
#define _mtsFunctionVoid_h

#include <cisstMultiTask/mtsCommandBase.h>
#include <cisstMultiTask/mtsFunctionBase.h>
#include <cisstMultiTask/mtsForwardDeclarations.h>

// Always include last
#include <cisstMultiTask/mtsExport.h>

class CISST_EXPORT mtsFunctionVoid: public mtsFunctionBase {
 protected:
    typedef mtsCommandVoid CommandType;
    CommandType * Command;

 public:
    /*! Default constructor.  Does nothing, use Instantiate before
      using. */
    mtsFunctionVoid(void): Command(0) {}

    /*! Destructor. */
    ~mtsFunctionVoid();

    // documented in base class
    bool Detach(void);

    // documented in base class
    bool IsValid(void) const;

    /*! Bind using a command pointer.  This allows to avoid
      querying by name from an interface.
      \param command Pointer on an existing command
      \result Boolean value, true if the command pointer is not null.
    */
    bool Bind(CommandType * command);

    /*! Overloaded operator to enable more intuitive syntax
      e.g., Command() instead of Command->Execute(). */
    mtsExecutionResult operator()(void) const;

    /*! Blocking call */
    mtsExecutionResult ExecuteBlocking(void) const;

    /*! Access to underlying command object. */
    mtsCommandVoid * GetCommand(void) const;

    // documented in base class
    void ToStream(std::ostream & outputStream) const;
};


#endif // _mtsFunctionVoid_h
