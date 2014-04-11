/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*

  Author(s):  Ankur Kapoor, Peter Kazanzides
  Created on: 2004-04-30

  (C) Copyright 2004-2007 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstMultiTask/mtsInterfaceOutput.h>


mtsInterfaceOutput::mtsInterfaceOutput(const std::string & name, mtsComponent * component):
    BaseType(name, component)
{
}


mtsInterfaceOutput::~mtsInterfaceOutput() {
	CMN_LOG_CLASS_INIT_VERBOSE << "Class mtsInterfaceOutput: Class destructor" << std::endl;
    // ADV: Need to add all cleanup, i.e. make sure all mailboxes are
    // properly deleted.
}
