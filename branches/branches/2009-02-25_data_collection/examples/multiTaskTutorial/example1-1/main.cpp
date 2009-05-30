/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */
/* $Id: main.cpp 188 2009-03-20 17:07:32Z mjung5 $ */

#include <cisstCommon.h>
#include <cisstOSAbstraction.h>
#include <cisstMultiTask.h>

#include "sineTask.h"
#include "displayTask.h"
#include "displayUI.h"

using namespace std;

int main(void)
{
    // log configuration
    cmnLogger::SetLoD(10);
    cmnLogger::GetMultiplexer()->AddChannel(cout, 10);
    // add a log per thread
    osaThreadedLogFile threadedLog("example1-");
    cmnLogger::GetMultiplexer()->AddChannel(threadedLog, 10);
    // specify a higher, more verbose log level for these classes
    cmnClassRegister::SetLoD("sineTask", 10);
    cmnClassRegister::SetLoD("displayTask", 10);
    cmnClassRegister::SetLoD("mtsTaskInterface", 10);
    cmnClassRegister::SetLoD("mtsTaskManager", 10);

    // Create user tasks
    const double PeriodSine = 1 * cmn_ms; // in milliseconds
    const double PeriodDisplay = 50 * cmn_ms; // in milliseconds

    sineTask * sineTaskObject = new sineTask("SIN", PeriodSine);
    displayTask * displayTaskObject = new displayTask("DISP", PeriodDisplay);    
    displayTaskObject->Configure();

    // add the tasks to the task manager
    mtsTaskManager * taskManager = mtsTaskManager::GetInstance();
    taskManager->AddTask(sineTaskObject);
    taskManager->AddTask(displayTaskObject);

    // data collector setup
    // 1) Plain text (ASCII) format
    //mtsCollectorState * Collector = new mtsCollectorState(sineTaskObject->GetName());
    // 2) Binary format using CISST serialization and deserialization (see commonTutorialSerializationRead/Write)
    mtsCollectorState * Collector = new mtsCollectorState(sineTaskObject->GetName(), mtsCollectorBase::COLLECTOR_LOG_FORMAT_BINARY);

    bool AddSignalFlag = true;
    try {        
        // Example A. Selectively choose signals of which data is to be collected.
        const string signalName = "SineData";  // refer to sineTask constructor
        AddSignalFlag &= Collector->AddSignal(signalName + "01");
        //AddSignalFlag &= Collector->AddSignal(signalName + "02");
        //AddSignalFlag &= Collector->AddSignal(signalName + "03");
        //AddSignalFlag &= Collector->AddSignal(signalName + "04");
        //AddSignalFlag &= Collector->AddSignal(signalName + "05");
        //AddSignalFlag &= Collector->AddSignal(signalName + "06");
        //AddSignalFlag &= Collector->AddSignal(signalName + "07");
        //AddSignalFlag &= Collector->AddSignal(signalName + "08");
        //AddSignalFlag &= Collector->AddSignal(signalName + "09");
        //AddSignalFlag &= Collector->AddSignal(signalName + "10");

        // Example B. Register ALL signals to the data collector.
        //AddSignalFlag = Collector->AddSignal();
    } catch (mtsCollectorBase::mtsCollectorBaseException e) {
        cout << "ERROR: Adding a signal failed." << endl;
    }

    if (!AddSignalFlag) {
        cout << "WARNING: Data Collector disabled." << endl;
    } else {
        taskManager->AddTask(Collector);
    }

    // connect the tasks, task.RequiresInterface -> task.ProvidesInterface
    taskManager->Connect("DISP", "DataGenerator", "SIN", "MainInterface");

    // generate a nice tasks diagram
    std::ofstream dotFile("example1.dot"); 
    taskManager->ToStreamDot(dotFile);
    dotFile.close();

    // create the tasks, i.e. find the commands
    taskManager->CreateAll();
    // start the periodic Run
    taskManager->StartAll();

    // Start immediately
    //Collector->SetSamplingInterval(4);
    Collector->Start(0);
    // Start some time later (5 seconds in the following case)
    //Collector->Start(5);

    // wait until the close button of the UI is pressed
    while (1) {
        osaSleep(10.0 * cmn_ms); // sleep to save CPU
        if (displayTaskObject->GetExitFlag()) {
            break;
        }        
    }
    // cleanup
    taskManager->KillAll();

    osaSleep(PeriodDisplay * 2);
    while (!sineTaskObject->IsTerminated()) osaSleep(PeriodDisplay);
    while (!displayTaskObject->IsTerminated()) osaSleep(PeriodDisplay);

    // Convert a binary log file to ASCII log.
    if (!Collector->ConvertBinaryLogFileIntoPlainText(
        Collector->GetLogFileName(), Collector->GetLogFileName() + ".converted.txt" )) 
    {
        cout << " Conversion failed." << std::endl;
        return 1;
    }

    return 0;
}

/*
  Author(s):  Ankur Kapoor, Peter Kazanzides, Anton Deguet, Min Yang Jung
  Created on: 2004-04-30

  (C) Copyright 2004-2009 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/
