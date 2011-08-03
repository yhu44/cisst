/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Balazs Vagvolgyi, Simon DiMaio, Anton Deguet
  Created on: 2008-05-23

  (C) Copyright 2008-2009 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstOSAbstraction/osaThreadedLogFile.h>
#include <cisstOSAbstraction/osaSleep.h>
#include <cisstMultiTask/mtsTaskManager.h>
#include <cisstDevices/devSensableHD.h>
#include <cisstDevices/devKeyboard.h>
#include <cisstCommon.h>
#include <cisstStereoVision.h>

#include <SimpleBehavior.h>
#include <ImageViewer.h>
#include <RegistrationBehavior.h>

int main()
{
    // log configuration
    cmnLogger::SetLoD(CMN_LOG_LOD_VERY_VERBOSE);
    cmnLogger::GetMultiplexer()->AddChannel(std::cout, CMN_LOG_LOD_VERY_VERBOSE);
    // add a log per thread
    osaThreadedLogFile threadedLog("example1-");
    cmnLogger::GetMultiplexer()->AddChannel(threadedLog, CMN_LOG_LOD_VERY_VERBOSE);
    // specify a higher, more verbose log level for these classes
    cmnClassRegister::SetLoD("ui3BehaviorBase", CMN_LOG_LOD_VERY_VERBOSE);
    cmnClassRegister::SetLoD("ui3Manager", CMN_LOG_LOD_VERY_VERBOSE);
    cmnClassRegister::SetLoD("mtsTaskInterface", CMN_LOG_LOD_VERY_VERBOSE);
    cmnClassRegister::SetLoD("mtsTaskManager", CMN_LOG_LOD_VERY_VERBOSE);
    cmnClassRegister::SetLoD("RegistrationBehavior", CMN_LOG_LOD_VERY_VERBOSE);

    mtsTaskManager * taskManager = mtsTaskManager::GetInstance();

    devSensableHD * sensable = new devSensableHD("Omni", "Omni1", "Omni2" /* name in driver, see Preferences in Sensable Driver */);
    taskManager->AddTask(sensable);


    ui3Manager guiManager;

    SimpleBehavior simpleBehavior("SimpleBehavior1");
    guiManager.AddBehavior(&simpleBehavior, // behavior reference
                           0,               // position in the menu bar: default
                           "circle.png");   // icon file: no texture

    ImageViewer imageViewer("ImageViewer1");
    guiManager.AddBehavior(&imageViewer,
                           1,
                           "square.png");
    /*
    RegistrationBehavior registrationBehavior("RegistrationBehavior1");
    guiManager.AddBehavior(&registrationBehavior,  // behavior reference
                           2,                      // position in the menu bar: default
                           "cube.png");            // icon file: no texture
*/
    svlCameraGeometry camera_geometry;
    camera_geometry.SetPerspective(400.0, 2);
    camera_geometry.RotateWorldAboutY(180.0);

    guiManager.AddRenderer(400, 300,           // render width & height
                           1.0,                // virtual camera zoom
                           false,              // borderless flag
                           0, 0,               // window position
                           camera_geometry, 2, // camera parameters
                           "ThirdEyeView");    // name of renderer

    vctFrm3 transform;
    transform.Translation().Assign(+30.0, 0.0, +150.0); // recenter Omni's depth (right)
    ui3MasterArm * rightMaster = new ui3MasterArm("Omni1");
    guiManager.AddMasterArm(rightMaster);
    rightMaster->SetInput(sensable, "Omni1",
                          sensable, "Omni1Button1",
                          sensable, "Omni1Button2",
                          ui3MasterArm::PRIMARY);
    rightMaster->SetTransformation(transform, 0.5 /* scale factor */);
    ui3CursorBase * rightCursor = new ui3CursorSphere();
    rightCursor->SetAnchor(ui3CursorBase::CENTER_RIGHT);
    rightMaster->SetCursor(rightCursor);

    transform.Translation().Assign(+30.0, 0.0, -150.0); // recenter Omni's depth (right)
    ui3MasterArm * leftMaster = new ui3MasterArm("Omni2");
    guiManager.AddMasterArm(leftMaster);
    leftMaster->SetInput(sensable, "Omni2",
                         sensable, "Omni2Button1",
                         sensable, "Omni2Button2",
                         ui3MasterArm::SECONDARY);
    leftMaster->SetTransformation(transform, 0.5 /* scale factor */);
    ui3CursorBase * leftCursor = new ui3CursorSphere();
    leftCursor->SetAnchor(ui3CursorBase::CENTER_LEFT);
    leftMaster->SetCursor(leftCursor);


    // connect keyboard to ui3 and Omnis
    devKeyboard * keyboard = new devKeyboard();

    taskManager->AddTask(keyboard);
    keyboard->SetQuitKey('q');

    // MaM
    keyboard->AddKeyButtonEvent('m', "MaM", true);
    guiManager.SetupMaM(keyboard, "MaM");

    guiManager.ConnectAll();

    // following should be replaced by a utility function or method of ui3Manager 
    taskManager->CreateAll();
    osaSleep(3.0 * cmn_s);
    taskManager->StartAll();

    osaSleep(1.0 * cmn_s);

    do {
        osaSleep(10.0 * cmn_ms);
    } while (!keyboard->Done());

    taskManager->KillAll();

    guiManager.SaveConfiguration("config.xml");

    taskManager->Cleanup();
    return 0;
}
