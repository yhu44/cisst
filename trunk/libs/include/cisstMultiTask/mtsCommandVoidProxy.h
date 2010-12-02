/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Min Yang Jung
  Created on: 2009-04-28

  (C) Copyright 2009-2010 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


/*!
  \file
  \brief Defines a command proxy class with no argument
 */

#ifndef _mtsCommandVoidProxy_h
#define _mtsCommandVoidProxy_h

#include <cisstMultiTask/mtsCommandVoid.h>
#include <cisstMultiTask/mtsCommandProxyBase.h>

/*!
  \ingroup cisstMultiTask

  mtsCommandVoidProxy is a proxy for mtsCommandVoid. When Execute() method is
  called, CommandID is sent to a connected required interface proxy across a
  network without payload.
*/
class mtsCommandVoidProxy: public mtsCommandVoid, public mtsCommandProxyBase
{
public:
    typedef mtsCommandVoid BaseType;

public:
    /*! Constructor. Command proxy is disabled by default and is enabled when
        command id and network proxy are set. */
    mtsCommandVoidProxy(const std::string & commandName): BaseType(0, commandName) {
        Disable();
    }

    /*! Destructor */
    ~mtsCommandVoidProxy() {}

    /*! Execute void command */
    mtsExecutionResult Execute(mtsBlockingType blocking) {
        if (IsDisabled()) return mtsExecutionResult::COMMAND_DISABLED;

        if (NetworkProxyServer) {
            // Command void execution: client (request) -> server (execution)
            if (!NetworkProxyServer->SendExecuteCommandVoid(ClientID, CommandID, blocking)) {
                return mtsExecutionResult::NETWORK_ERROR;
            }
        } else {
            // Event void execution: server (event generator) -> client (event handler)
            if (!NetworkProxyClient->SendExecuteEventVoid(CommandID)) {
                return mtsExecutionResult::NETWORK_ERROR;
            }
        }

        return mtsExecutionResult::COMMAND_SUCCEEDED;
    }

    /*! Generate human readable description of this object */
    void ToStream(std::ostream & outputStream) const {
        ToStreamBase("mtsCommandVoidProxy", Name, CommandID, IsEnabled(), outputStream);
    }
};

#endif // _mtsCommandVoidProxy_h

