/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Ankur Kapoor, Peter Kazanzides, Anton Deguet
  Created on: 2005-05-02

  (C) Copyright 2005-2007 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


#include <cisstMultiTask/mtsCommandQueuedVoidBase.h>


void mtsCommandQueuedVoidBase::ToStream(std::ostream & outputStream) const {
    outputStream << "mtsCommandQueuedVoid: Mailbox \"";
    if (this->MailBox) {
        outputStream << this->MailBox->GetName();
    } else {
        outputStream << "Undefined";
    }
    outputStream << "\" for command " << *(this->ActualCommand)
                 << " currently " << (this->IsEnabled() ? "enabled" : "disabled");
}

#if 0    
mtsCommandBase::ReturnType mtsCommandQueuedVoidBase::Execute(void) {
    if (this->MailBox->Write(this))
        return mtsCommandBase::DEV_OK;
    else {
        CMN_LOG(5) << "mtsCommandQueuedVoidBase: Unable to write to mail box." << std::endl;
        return mtsCommandBase::MAILBOX_FULL;
    }
}
#endif
