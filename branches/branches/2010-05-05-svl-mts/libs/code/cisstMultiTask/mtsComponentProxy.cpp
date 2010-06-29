/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s):  Min Yang Jung
  Created on: 2009-12-18

  (C) Copyright 2009-2010 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstMultiTask/mtsComponentProxy.h>

#include <cisstCommon/cmnUnits.h>
#include <cisstOSAbstraction/osaSleep.h>
#include <cisstMultiTask/mtsManagerLocal.h>
#include <cisstMultiTask/mtsTaskInterface.h>
#include <cisstMultiTask/mtsInterfaceRequired.h>
#include <cisstMultiTask/mtsFunctionReadOrWriteProxy.h>
#include <cisstMultiTask/mtsFunctionQualifiedReadOrWriteProxy.h>

mtsComponentProxy::mtsComponentProxy(const std::string & componentProxyName)
: mtsDevice(componentProxyName), InterfaceProvidedProxyInstanceID(0)
{
}

mtsComponentProxy::~mtsComponentProxy()
{
    // Clean up all internal resources

    // Stop all provided interface proxies running.
    InterfaceProvidedNetworkProxyMapType::MapType::const_iterator itProvidedProxy = InterfaceProvidedNetworkProxies.begin();
    const InterfaceProvidedNetworkProxyMapType::MapType::const_iterator itProvidedProxyEnd = InterfaceProvidedNetworkProxies.end();
    for (; itProvidedProxy != itProvidedProxyEnd; ++itProvidedProxy) {
        itProvidedProxy->second->Stop();
    }
    InterfaceProvidedNetworkProxies.DeleteAll();

    // Stop all required interface proxies running.
    InterfaceRequiredNetworkProxyMapType::MapType::const_iterator itRequiredProxy = InterfaceRequiredNetworkProxies.begin();
    const InterfaceRequiredNetworkProxyMapType::MapType::const_iterator itRequiredProxyEnd = InterfaceRequiredNetworkProxies.end();
    for (; itRequiredProxy != itRequiredProxyEnd; ++itRequiredProxy) {
        itRequiredProxy->second->Stop();
    }
    InterfaceRequiredNetworkProxies.DeleteAll();


    InterfaceProvidedProxyInstanceMapType::const_iterator itInstance = InterfaceProvidedProxyInstanceMap.begin();
    const InterfaceProvidedProxyInstanceMapType::const_iterator itInstanceEnd = InterfaceProvidedProxyInstanceMap.end();
    for (; itInstance != itInstanceEnd; ++itInstance) {
        delete itInstance->second;
    }

    FunctionProxyAndEventHandlerProxyMap.DeleteAll();
}

//-----------------------------------------------------------------------------
//  Methods for Server Components
//-----------------------------------------------------------------------------
bool mtsComponentProxy::CreateInterfaceRequiredProxy(const InterfaceRequiredDescription & requiredInterfaceDescription)
{
    const std::string requiredInterfaceName = requiredInterfaceDescription.InterfaceRequiredName;

    // Check if the interface name is unique
    mtsInterfaceRequired * requiredInterface = GetInterfaceRequired(requiredInterfaceName);
    if (requiredInterface) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: can't create required interface proxy: "
            << "duplicate name: " << requiredInterfaceName << std::endl;
        return false;
    }

    // Create a required interface proxy
    mtsInterfaceRequired * requiredInterfaceProxy = new mtsInterfaceRequired(requiredInterfaceName);

    // Store function proxy pointers and event handler proxy pointers to assign
    // command proxies' id at server side.
    FunctionProxyAndEventHandlerProxyMapElement * mapElement = new FunctionProxyAndEventHandlerProxyMapElement;

    // Populate the new required interface
    mtsFunctionVoid * functionVoidProxy;
    mtsFunctionWrite * functionWriteProxy;
    mtsFunctionRead * functionReadProxy;
    mtsFunctionQualifiedRead * functionQualifiedReadProxy;

    bool success;

    // Create void function proxies
    const std::vector<std::string> namesOfFunctionVoid = requiredInterfaceDescription.FunctionVoidNames;
    for (unsigned int i = 0; i < namesOfFunctionVoid.size(); ++i) {
        functionVoidProxy = new mtsFunctionVoid();
        success = requiredInterfaceProxy->AddFunction(namesOfFunctionVoid[i], *functionVoidProxy);
        success &= mapElement->FunctionVoidProxyMap.AddItem(namesOfFunctionVoid[i], functionVoidProxy);
        if (!success) {
            delete requiredInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: failed to add void function proxy: " << namesOfFunctionVoid[i] << std::endl;
            return false;
        }
    }

    // Create write function proxies
    const std::vector<std::string> namesOfFunctionWrite = requiredInterfaceDescription.FunctionWriteNames;
    for (unsigned int i = 0; i < namesOfFunctionWrite.size(); ++i) {
        functionWriteProxy = new mtsFunctionWriteProxy();
        success = requiredInterfaceProxy->AddFunction(namesOfFunctionWrite[i], *functionWriteProxy);
        success &= mapElement->FunctionWriteProxyMap.AddItem(namesOfFunctionWrite[i], functionWriteProxy);
        if (!success) {
            delete requiredInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: failed to add write function proxy: " << namesOfFunctionWrite[i] << std::endl;
            return false;
        }
    }

    // Create read function proxies
    const std::vector<std::string> namesOfFunctionRead = requiredInterfaceDescription.FunctionReadNames;
    for (unsigned int i = 0; i < namesOfFunctionRead.size(); ++i) {
        functionReadProxy = new mtsFunctionReadProxy();
        success = requiredInterfaceProxy->AddFunction(namesOfFunctionRead[i], *functionReadProxy);
        success &= mapElement->FunctionReadProxyMap.AddItem(namesOfFunctionRead[i], functionReadProxy);
        if (!success) {
            delete requiredInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: failed to add read function proxy: " << namesOfFunctionRead[i] << std::endl;
            return false;
        }
    }

    // Create QualifiedRead function proxies
    const std::vector<std::string> namesOfFunctionQualifiedRead = requiredInterfaceDescription.FunctionQualifiedReadNames;
    for (unsigned int i = 0; i < namesOfFunctionQualifiedRead.size(); ++i) {
        functionQualifiedReadProxy = new mtsFunctionQualifiedReadProxy();
        success = requiredInterfaceProxy->AddFunction(namesOfFunctionQualifiedRead[i], *functionQualifiedReadProxy);
        success &= mapElement->FunctionQualifiedReadProxyMap.AddItem(namesOfFunctionQualifiedRead[i], functionQualifiedReadProxy);
        if (!success) {
            delete requiredInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: failed to add qualified read function proxy: " << namesOfFunctionQualifiedRead[i] << std::endl;
            return false;
        }
    }

    // Create event handler proxies
    std::string eventName;

    // Create event handler proxies with CommandIDs set to zero and disable them
    // by default. It will be updated and enabled later by UpdateEventHandlerID(),

    // Create void event handler proxy
    mtsCommandVoidProxy * newEventVoidHandlerProxy = NULL;
    for (unsigned int i = 0; i < requiredInterfaceDescription.EventHandlersVoid.size(); ++i) {
        eventName = requiredInterfaceDescription.EventHandlersVoid[i].Name;
        newEventVoidHandlerProxy = new mtsCommandVoidProxy(eventName);
        if (!requiredInterfaceProxy->EventHandlersVoid.AddItem(eventName, newEventVoidHandlerProxy)) {
            delete newEventVoidHandlerProxy;
            delete requiredInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: failed to add void event handler proxy: " << eventName << std::endl;
            return false;
        }
    }

    // Create write event handler proxies
    std::stringstream streamBuffer;
    cmnDeSerializer deserializer(streamBuffer);

    mtsCommandWriteProxy * newEventWriteHandlerProxy = NULL;
    mtsGenericObject * argumentPrototype = NULL;
    for (unsigned int i = 0; i < requiredInterfaceDescription.EventHandlersWrite.size(); ++i) {
        eventName = requiredInterfaceDescription.EventHandlersWrite[i].Name;
        newEventWriteHandlerProxy = new mtsCommandWriteProxy(eventName);
        if (!requiredInterfaceProxy->EventHandlersWrite.AddItem(eventName, newEventWriteHandlerProxy)) {
            delete newEventWriteHandlerProxy;
            delete requiredInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: failed to add write event handler proxy: " << eventName << std::endl;
            return false;
        }

        // argument deserialization
        streamBuffer.str("");
        streamBuffer << requiredInterfaceDescription.EventHandlersWrite[i].ArgumentPrototypeSerialized;
        try {
            argumentPrototype = dynamic_cast<mtsGenericObject *>(deserializer.DeSerialize());
        } catch (std::exception e) {
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: write command argument deserialization failed: " << e.what() << std::endl;
            argumentPrototype = NULL;
        }

        if (!argumentPrototype) {
            delete requiredInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: failed to create event write handler proxy: " << eventName << std::endl;
            return false;
        }
        newEventWriteHandlerProxy->SetArgumentPrototype(argumentPrototype);
    }

    // Add the required interface proxy to the component
    if (!AddInterfaceRequired(requiredInterfaceName, requiredInterfaceProxy)) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: failed to add required interface proxy: " << requiredInterfaceName << std::endl;
        delete requiredInterfaceProxy;
        return false;
    }

    // Add to function proxy and event handler proxy map
    if (!FunctionProxyAndEventHandlerProxyMap.AddItem(requiredInterfaceName, mapElement)) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceRequiredProxy: failed to add proxy map: " << requiredInterfaceName << std::endl;
        return false;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "CreateInterfaceRequiredProxy: added required interface proxy: " << requiredInterfaceName << std::endl;

    return true;
}

bool mtsComponentProxy::RemoveInterfaceRequiredProxy(const std::string & requiredInterfaceProxyName)
{
    // Get network objects to remove
    mtsComponentInterfaceProxyClient * clientProxy = InterfaceRequiredNetworkProxies.GetItem(requiredInterfaceProxyName);
    if (!clientProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveInterfaceRequiredProxy: cannot find proxy client: " << requiredInterfaceProxyName << std::endl;
        return false;
    } else {
        // Network server deactivation and resource clean up
        delete clientProxy;
        InterfaceRequiredNetworkProxies.RemoveItem(requiredInterfaceProxyName);
    }

    // Get logical objects to remove
    if (!InterfacesRequired.FindItem(requiredInterfaceProxyName)) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveInterfaceRequiredProxy: cannot find required interface proxy: " << requiredInterfaceProxyName << std::endl;
        return false;
    }
    mtsInterfaceRequired * requiredInterfaceProxy = InterfacesRequired.GetItem(requiredInterfaceProxyName);
    if (!requiredInterfaceProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveInterfaceRequiredProxy: This should not happen" << std::endl;
        return false;
    } else {
        delete requiredInterfaceProxy;
        InterfacesRequired.RemoveItem(requiredInterfaceProxyName);
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "RemoveInterfaceRequiredProxy: removed required interface proxy: " << requiredInterfaceProxyName << std::endl;

    return true;
}

//-----------------------------------------------------------------------------
//  Methods for Client Components
//-----------------------------------------------------------------------------
bool mtsComponentProxy::CreateInterfaceProvidedProxy(const InterfaceProvidedDescription & providedInterfaceDescription)
{
    const std::string providedInterfaceName = providedInterfaceDescription.InterfaceProvidedName;

    // Create a local provided interface (a provided interface proxy) but it
    // is not immediately added to the component. It is added to this component
    // only after all proxy objects (command proxies and event proxies) are 
    // confirmed to be successfully created.
    mtsInterfaceProvided * providedInterfaceProxy = new mtsInterfaceProvidedOrOutput(providedInterfaceName, this);

    // Create command proxies according to the information about the original
    // provided interface. CommandId is initially set to zero and will be
    // updated later by GetCommandId().

    // Since argument prototypes in the interface description have been serialized,
    // they need to be deserialized.
    std::string commandName;
    mtsGenericObject * argumentPrototype = NULL,
                     * argument1Prototype = NULL,
                     * argument2Prototype = NULL;

    std::stringstream streamBuffer;
    cmnDeSerializer deserializer(streamBuffer);

    // Create void command proxies
    mtsCommandVoidProxy * newCommandVoid = NULL;
    CommandVoidVector::const_iterator itVoid = providedInterfaceDescription.CommandsVoid.begin();
    const CommandVoidVector::const_iterator itVoidEnd = providedInterfaceDescription.CommandsVoid.end();
    for (; itVoid != itVoidEnd; ++itVoid) {
        commandName = itVoid->Name;
        newCommandVoid = new mtsCommandVoidProxy(commandName);
        //if (!providedInterfaceProxy->GetCommandVoidMap().AddItem(commandName, newCommandVoid)) {
        if (!providedInterfaceProxy->AddCommandVoid(newCommandVoid)) {
            delete newCommandVoid;
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to add void command proxy: " << commandName << std::endl;
            return false;
        }
    }

    // Create write command proxies
    mtsCommandWriteProxy * newCommandWrite = NULL;
    CommandWriteVector::const_iterator itWrite = providedInterfaceDescription.CommandsWrite.begin();
    const CommandWriteVector::const_iterator itWriteEnd = providedInterfaceDescription.CommandsWrite.end();
    for (; itWrite != itWriteEnd; ++itWrite) {
        commandName = itWrite->Name;
        newCommandWrite = new mtsCommandWriteProxy(commandName);
        //if (!providedInterfaceProxy->GetCommandWriteMap().AddItem(commandName, newCommandWrite)) {
        if (!providedInterfaceProxy->AddCommandWrite(newCommandWrite)) {
            delete newCommandWrite;
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to add " <<
                (itWrite->Category == 0 ? "write" : "filtered write") << " command proxy: " << commandName << std::endl;
            return false;
        }

        // argument deserialization
        streamBuffer.str("");
        streamBuffer << itWrite->ArgumentPrototypeSerialized;
        try {
            argumentPrototype = dynamic_cast<mtsGenericObject *>(deserializer.DeSerialize());
        } catch (std::exception e) {
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to deserialize " <<
                (itWrite->Category == 0 ? "write" : "filtered write") << " command argument: " << e.what() << std::endl;
            argumentPrototype = NULL;
        }

        if (!argumentPrototype) {
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to create " <<
                (itWrite->Category == 0 ? "write" : "filtered write") << " command proxy: " << commandName << std::endl;
            return false;
        }
        newCommandWrite->SetArgumentPrototype(argumentPrototype);
    }

    // Create read command proxies
    mtsCommandReadProxy * newCommandRead = NULL;
    CommandReadVector::const_iterator itRead = providedInterfaceDescription.CommandsRead.begin();
    const CommandReadVector::const_iterator itReadEnd = providedInterfaceDescription.CommandsRead.end();
    for (; itRead != itReadEnd; ++itRead) {
        commandName = itRead->Name;
        newCommandRead = new mtsCommandReadProxy(commandName);
        //if (!providedInterfaceProxy->GetCommandReadMap().AddItem(commandName, newCommandRead)) {
        if (!providedInterfaceProxy->AddCommandRead(newCommandRead)) {
            delete newCommandRead;
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to add read command proxy: " << commandName << std::endl;
            return false;
        }

        // argument deserialization
        streamBuffer.str("");
        streamBuffer << itRead->ArgumentPrototypeSerialized;
        try {
            argumentPrototype = dynamic_cast<mtsGenericObject *>(deserializer.DeSerialize());
        } catch (std::exception e) {
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: read command argument deserialization failed: " << e.what() << std::endl;
            argumentPrototype = NULL;
        }

        if (!argumentPrototype) {
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to create read command proxy: " << commandName << std::endl;
            return false;
        }
        newCommandRead->SetArgumentPrototype(argumentPrototype);
    }

    // Create qualified read command proxies
    mtsCommandQualifiedReadProxy * newCommandQualifiedRead = NULL;
    CommandQualifiedReadVector::const_iterator itQualifiedRead = providedInterfaceDescription.CommandsQualifiedRead.begin();
    const CommandQualifiedReadVector::const_iterator itQualifiedReadEnd = providedInterfaceDescription.CommandsQualifiedRead.end();
    for (; itQualifiedRead != itQualifiedReadEnd; ++itQualifiedRead) {
        commandName = itQualifiedRead->Name;
        newCommandQualifiedRead = new mtsCommandQualifiedReadProxy(commandName);
        //if (!providedInterfaceProxy->GetCommandQualifiedReadMap().AddItem(commandName, newCommandQualifiedRead)) {
        if (!providedInterfaceProxy->AddCommandQualifiedRead(newCommandQualifiedRead)) {
            delete newCommandQualifiedRead;
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to add qualified read command proxy: " << commandName << std::endl;
            return false;
        }

        // argument1 deserialization
        streamBuffer.str("");
        streamBuffer << itQualifiedRead->Argument1PrototypeSerialized;
        try {
            argument1Prototype = dynamic_cast<mtsGenericObject *>(deserializer.DeSerialize());
        } catch (std::exception e) {
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: qualified read command argument 1 deserialization failed: " << e.what() << std::endl;
            argument1Prototype = NULL;
        }
        // argument2 deserialization
        streamBuffer.str("");
        streamBuffer << itQualifiedRead->Argument2PrototypeSerialized;
        try {
            argument2Prototype = dynamic_cast<mtsGenericObject *>(deserializer.DeSerialize());
        } catch (std::exception e) {
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: qualified read command argument 2 deserialization failed: " << e.what() << std::endl;
            argument2Prototype = NULL;
        }

        if (!argument1Prototype || !argument2Prototype) {
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to create qualified read command proxy: " << commandName << std::endl;
            return false;
        }
        newCommandQualifiedRead->SetArgumentPrototype(argument1Prototype, argument2Prototype);
    }

    // Create event generator proxies
    std::string eventName;

    FunctionProxyAndEventHandlerProxyMapElement * mapElement
        = FunctionProxyAndEventHandlerProxyMap.GetItem(providedInterfaceName);
    if (!mapElement) {
        mapElement = new FunctionProxyAndEventHandlerProxyMapElement;
    }

    // Create void event generator proxies
    mtsFunctionVoid * eventVoidGeneratorProxy = NULL;
    // TODO: Isn't mtsMulticastCommandVoidProxy needed?
    //mtsMulticastCommandVoidProxy eventMulticastCommandVoidProxy;
    EventVoidVector::const_iterator itEventVoid = providedInterfaceDescription.EventsVoid.begin();
    const EventVoidVector::const_iterator itEventVoidEnd = providedInterfaceDescription.EventsVoid.end();
    for (; itEventVoid != itEventVoidEnd; ++itEventVoid) {
        eventName = itEventVoid->Name;
        eventVoidGeneratorProxy = new mtsFunctionVoid();
        if (!mapElement->EventGeneratorVoidProxyMap.AddItem(eventName, eventVoidGeneratorProxy)) {
            delete eventVoidGeneratorProxy;
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to create event generator proxy: " << eventName << std::endl;
            return false;
        }
        if (!eventVoidGeneratorProxy->Bind(providedInterfaceProxy->AddEventVoid(eventName))) {
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to create event generator proxy: " << eventName << std::endl;
            return false;
        }
    }

    // Create write event generator proxies
    mtsFunctionWrite * eventWriteGeneratorProxy;
    mtsMulticastCommandWriteProxy * eventMulticastCommandWriteProxy;
    EventWriteVector::const_iterator itEventWrite = providedInterfaceDescription.EventsWrite.begin();
    const EventWriteVector::const_iterator itEventWriteEnd = providedInterfaceDescription.EventsWrite.end();
    for (; itEventWrite != itEventWriteEnd; ++itEventWrite) {
        eventName = itEventWrite->Name;
        eventWriteGeneratorProxy = new mtsFunctionWrite();//new mtsFunctionWriteProxy();
        if (!mapElement->EventGeneratorWriteProxyMap.AddItem(eventName, eventWriteGeneratorProxy)) {
            delete eventWriteGeneratorProxy;
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to add event write generator proxy pointer: " << eventName << std::endl;
            return false;
        }

        eventMulticastCommandWriteProxy = new mtsMulticastCommandWriteProxy(eventName);

        // event argument deserialization
        streamBuffer.str("");
        streamBuffer << itEventWrite->ArgumentPrototypeSerialized;
        try {
            argumentPrototype = dynamic_cast<mtsGenericObject *>(deserializer.DeSerialize());
        } catch (std::exception e) {
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: argument deserialization for event write generator failed: " << e.what() << std::endl;
            argumentPrototype = NULL;
        }
        if (!argumentPrototype) {
            delete eventMulticastCommandWriteProxy;
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to create write event proxy: " << eventName << std::endl;
            return false;
        }
        eventMulticastCommandWriteProxy->SetArgumentPrototype(argumentPrototype);

        if (!providedInterfaceProxy->AddEvent(eventName, eventMulticastCommandWriteProxy)) {
            delete eventMulticastCommandWriteProxy;
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to add event multicast write command proxy: " << eventName << std::endl;
            return false;
        }

        if (!eventWriteGeneratorProxy->Bind(eventMulticastCommandWriteProxy)) {
            delete eventMulticastCommandWriteProxy;
            delete providedInterfaceProxy;
            CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to bind with event multicast write command proxy: " << eventName << std::endl;
            return false;
        }
    }

    // Add the provided interface proxy to the component
    if (!InterfacesProvided.AddItem(providedInterfaceName, providedInterfaceProxy)) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProvidedProxy: failed to add provided interface proxy: " << providedInterfaceName << std::endl;
        delete providedInterfaceProxy;
        return false;
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "CreateInterfaceProvidedProxy: added provided interface proxy: " << providedInterfaceName << std::endl;

    return true;
}

bool mtsComponentProxy::RemoveInterfaceProvidedProxy(const std::string & providedInterfaceProxyName)
{
    // Get network objects to remove
    mtsComponentInterfaceProxyServer * serverProxy = InterfaceProvidedNetworkProxies.GetItem(providedInterfaceProxyName);
    if (!serverProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveInterfaceProvidedProxy: cannot find proxy server: " << providedInterfaceProxyName << std::endl;
        return false;
    } else {
        // Network server deactivation and resource clean up
        delete serverProxy;
        InterfaceProvidedNetworkProxies.RemoveItem(providedInterfaceProxyName);
    }

    // Get logical objects to remove
    if (!InterfacesProvided.FindItem(providedInterfaceProxyName)) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveInterfaceProvidedProxy: cannot find provided interface proxy: " << providedInterfaceProxyName << std::endl;
        return false;
    }
    mtsInterfaceProvided * providedInterfaceProxy = InterfacesProvided.GetItem(providedInterfaceProxyName);
    if (!providedInterfaceProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "RemoveInterfaceProvidedProxy: cannot find provided interface proxy instance: " << providedInterfaceProxyName << std::endl;
        return false;
    } else {
        delete providedInterfaceProxy;
        InterfacesProvided.RemoveItem(providedInterfaceProxyName);
    }

    CMN_LOG_CLASS_RUN_VERBOSE << "RemoveInterfaceProvidedProxy: removed provided interface proxy: " << providedInterfaceProxyName << std::endl;

    return true;
}

bool mtsComponentProxy::CreateInterfaceProxyServer(const std::string & providedInterfaceProxyName,
                                                   std::string & endpointAccessInfo,
                                                   std::string & communicatorID)
{
    // Generate parameters to initialize server proxy
    std::string adapterName("ComponentInterfaceServerAdapter");
    adapterName += "_";
    adapterName += providedInterfaceProxyName;
    communicatorID = mtsComponentInterfaceProxyServer::GetInterfaceCommunicatorID();

    // Create an instance of mtsComponentInterfaceProxyServer
    mtsComponentInterfaceProxyServer * providedInterfaceProxy =
        new mtsComponentInterfaceProxyServer(adapterName, communicatorID);

    // Add it to provided interface proxy object map
    if (!InterfaceProvidedNetworkProxies.AddItem(providedInterfaceProxyName, providedInterfaceProxy)) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProxyServer: "
            << "Cannot register provided interface proxy: " << providedInterfaceProxyName << std::endl;
        return false;
    }

    // Run provided interface proxy (i.e., component interface proxy server)
    if (!providedInterfaceProxy->Start(this)) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProxyServer: proxy failed to start: " << providedInterfaceProxyName << std::endl;
        return false;
    }

    providedInterfaceProxy->GetLogger()->trace(
        "mtsComponentProxy", "provided interface proxy starts: " + providedInterfaceProxyName);

    // Return this server's endpoint information
    endpointAccessInfo = providedInterfaceProxy->GetEndpointInfo();

    return true;
}

bool mtsComponentProxy::CreateInterfaceProxyClient(const std::string & requiredInterfaceProxyName,
                                                   const std::string & serverEndpointInfo,
                                                   const std::string & CMN_UNUSED(communicatorID),
                                                   const unsigned int providedInterfaceProxyInstanceID)
{
    // Create an instance of mtsComponentInterfaceProxyClient
    mtsComponentInterfaceProxyClient * requiredInterfaceProxy =
        new mtsComponentInterfaceProxyClient(serverEndpointInfo, providedInterfaceProxyInstanceID);

    // Add it to required interface proxy object map
    if (!InterfaceRequiredNetworkProxies.AddItem(requiredInterfaceProxyName, requiredInterfaceProxy)) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProxyClient: "
            << "cannot register required interface proxy: " << requiredInterfaceProxyName << std::endl;
        return false;
    }

    // Run required interface proxy (i.e., component interface proxy client)
    if (!requiredInterfaceProxy->Start(this)) {
        CMN_LOG_CLASS_RUN_ERROR << "CreateInterfaceProxyClient: proxy failed to start: " << requiredInterfaceProxyName << std::endl;
        return false;
    }

    requiredInterfaceProxy->GetLogger()->trace(
        "mtsComponentProxy", "required interface proxy starts: " + requiredInterfaceProxyName);

    return true;
}

bool mtsComponentProxy::IsActiveProxy(const std::string & proxyName, const bool isProxyServer) const
{
    if (isProxyServer) {
        mtsComponentInterfaceProxyServer * providedInterfaceProxy = InterfaceProvidedNetworkProxies.GetItem(proxyName);
        if (!providedInterfaceProxy) {
            CMN_LOG_CLASS_RUN_ERROR << "IsActiveProxy: Cannot find provided interface proxy: " << proxyName << std::endl;
            return false;
        }
        return providedInterfaceProxy->IsActiveProxy();
    } else {
        mtsComponentInterfaceProxyClient * requiredInterfaceProxy = InterfaceRequiredNetworkProxies.GetItem(proxyName);
        if (!requiredInterfaceProxy) {
            CMN_LOG_CLASS_RUN_ERROR << "IsActiveProxy: Cannot find required interface proxy: " << proxyName << std::endl;
            return false;
        }
        return requiredInterfaceProxy->IsActiveProxy();
    }
}

bool mtsComponentProxy::UpdateEventHandlerProxyID(const std::string & clientComponentName, const std::string & requiredInterfaceName)
{
    // Note that this method is only called by a server process.

    // Get network proxy client connected to the required interface proxy
    // of which name is 'requiredInterfaceName.'
    mtsComponentInterfaceProxyClient * interfaceProxyClient = InterfaceRequiredNetworkProxies.GetItem(requiredInterfaceName);
    if (!interfaceProxyClient) {
        CMN_LOG_CLASS_RUN_ERROR << "UpdateEventHandlerProxyID: no interface proxy client found: " << requiredInterfaceName << std::endl;
        return false;
    }

    // Fetch pointers of event generator proxies from the connected provided
    // interface proxy at the client side.
    mtsComponentInterfaceProxy::EventGeneratorProxyPointerSet eventGeneratorProxyPointers;
    if (!interfaceProxyClient->SendFetchEventGeneratorProxyPointers(clientComponentName, requiredInterfaceName, eventGeneratorProxyPointers))
    {
        CMN_LOG_CLASS_RUN_ERROR << "UpdateEventHandlerProxyID: failed to fetch event generator proxy pointers: " << clientComponentName << ":" << requiredInterfaceName << std::endl;
        return false;
    }

    mtsComponentInterfaceProxy::EventGeneratorProxySequence::const_iterator it;
    mtsComponentInterfaceProxy::EventGeneratorProxySequence::const_iterator itEnd;

    mtsInterfaceRequired * requiredInterface = GetInterfaceRequired(requiredInterfaceName);
    if (!requiredInterface) {
        CMN_LOG_CLASS_RUN_ERROR << "UpdateEventHandlerProxyID: no required interface found: " << requiredInterfaceName << std::endl;
        return false;
    }

    mtsCommandVoidProxy * eventHandlerVoid;
    it = eventGeneratorProxyPointers.EventGeneratorVoidProxies.begin();
    itEnd = eventGeneratorProxyPointers.EventGeneratorVoidProxies.end();
    for (; it != itEnd; ++it) {
        // Get event handler proxy of which id is current zero and which is disabled
        eventHandlerVoid = dynamic_cast<mtsCommandVoidProxy*>(requiredInterface->EventHandlersVoid.GetItem(it->Name));
        if (!eventHandlerVoid) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateEventHandlerProxyID: cannot find event void handler proxy: " << it->Name << std::endl;
            return false;
        }
        // Set client ID and network proxy. Note that SetNetworkProxy() should
        // be called before SetCommandID().
        if (!eventHandlerVoid->SetNetworkProxy(interfaceProxyClient)) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateEventHandlerProxyID:: failed to set network proxy: " << it->Name << std::endl;
            return false;
        }
        eventHandlerVoid->SetCommandID(it->EventGeneratorProxyId);
        eventHandlerVoid->Enable();
    }

    mtsCommandWriteProxy * eventHandlerWrite;
    it = eventGeneratorProxyPointers.EventGeneratorWriteProxies.begin();
    itEnd = eventGeneratorProxyPointers.EventGeneratorWriteProxies.end();
    for (; it != itEnd; ++it) {
        // Get event handler proxy which is disabled and of which id is current zero
        eventHandlerWrite = dynamic_cast<mtsCommandWriteProxy*>(requiredInterface->EventHandlersWrite.GetItem(it->Name));
        if (!eventHandlerWrite) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateEventHandlerProxyID: cannot find event Write handler proxy: " << it->Name << std::endl;
            return false;
        }
        // Set client ID and network proxy. Note that SetNetworkProxy() should
        // be called before SetCommandID().
        if (!eventHandlerWrite->SetNetworkProxy(interfaceProxyClient)) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateEventHandlerProxyID:: failed to set network proxy: " << it->Name << std::endl;
            return false;
        }
        eventHandlerWrite->SetCommandID(it->EventGeneratorProxyId);
        eventHandlerWrite->Enable();
    }

    return true;
}

bool mtsComponentProxy::UpdateCommandProxyID(
    const std::string & serverInterfaceProvidedName, const std::string & CMN_UNUSED(clientComponentName),
    const std::string & clientInterfaceRequiredName, const unsigned int providedInterfaceProxyInstanceID)
{
    const unsigned int clientID = providedInterfaceProxyInstanceID;

    // Note that this method is only called by a client process.

    // Get a network proxy server that corresponds to 'serverInterfaceProvidedName'
    mtsComponentInterfaceProxyServer * interfaceProxyServer =
        InterfaceProvidedNetworkProxies.GetItem(serverInterfaceProvidedName);
    if (!interfaceProxyServer) {
        CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: no interface proxy server found: " << serverInterfaceProvidedName << std::endl;
        return false;
    }

    // Fetch function proxy pointers from a connected required interface proxy
    // at server side, which will be used to set command proxies' IDs.
    mtsComponentInterfaceProxy::FunctionProxyPointerSet functionProxyPointers;
    if (!interfaceProxyServer->SendFetchFunctionProxyPointers(
            providedInterfaceProxyInstanceID, clientInterfaceRequiredName, functionProxyPointers))
    {
        CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: failed to fetch function proxy pointers: "
            << clientInterfaceRequiredName << " @ " << providedInterfaceProxyInstanceID << std::endl;
        return false;
    }

    // Get a provided interface proxy instance of which command proxies are updated.
    InterfaceProvidedProxyInstanceMapType::const_iterator it =
        InterfaceProvidedProxyInstanceMap.find(providedInterfaceProxyInstanceID);
    if (it == InterfaceProvidedProxyInstanceMap.end()) {
        CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: failed to fetch provided interface proxy instance: "
            << providedInterfaceProxyInstanceID << std::endl;
        return false;
    }
    mtsInterfaceProvided * instance = it->second;

    // Set command proxy IDs in the provided interface proxy as the
    // function proxies' pointers fetched from server process.

    // Void command
    mtsCommandVoidProxy * commandVoid = NULL;
    mtsComponentInterfaceProxy::FunctionProxySequence::const_iterator itVoid = functionProxyPointers.FunctionVoidProxies.begin();
    const mtsComponentInterfaceProxy::FunctionProxySequence::const_iterator itVoidEnd= functionProxyPointers.FunctionVoidProxies.end();
    for (; itVoid != itVoidEnd; ++itVoid) {
        commandVoid = dynamic_cast<mtsCommandVoidProxy*>(instance->GetCommandVoid(itVoid->Name));
        if (!commandVoid) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: failed to update command void proxy id: " << itVoid->Name << std::endl;
            return false;
        }
        // Set client ID and network proxy. Note that SetNetworkProxy() should
        // be called before SetCommandID().
        if (!commandVoid->SetNetworkProxy(interfaceProxyServer, clientID)) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: failed to set network proxy: " << itVoid->Name << std::endl;
            return false;
        }
        // Set command void proxy's id and enable this command
        commandVoid->SetCommandID(itVoid->FunctionProxyId);
        commandVoid->Enable();
    }

    // Write command
    mtsCommandWriteProxy * commandWrite = NULL;
    mtsComponentInterfaceProxy::FunctionProxySequence::const_iterator itWrite = functionProxyPointers.FunctionWriteProxies.begin();
    const mtsComponentInterfaceProxy::FunctionProxySequence::const_iterator itWriteEnd = functionProxyPointers.FunctionWriteProxies.end();
    for (; itWrite != itWriteEnd; ++itWrite) {
        commandWrite = dynamic_cast<mtsCommandWriteProxy*>(instance->GetCommandWrite(itWrite->Name));
        if (!commandWrite) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: failed to update command write proxy id: " << itWrite->Name << std::endl;
            return false;
        }
        // Set client ID and network proxy
        if (!commandWrite->SetNetworkProxy(interfaceProxyServer, clientID)) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: failed to set network proxy: " << itWrite->Name << std::endl;
            return false;
        }
        // Set command write proxy's id and enable this command
        commandWrite->SetCommandID(itWrite->FunctionProxyId);
        commandWrite->Enable();
    }

    // Read command
    mtsCommandReadProxy * commandRead = NULL;
    mtsComponentInterfaceProxy::FunctionProxySequence::const_iterator itRead = functionProxyPointers.FunctionReadProxies.begin();
    const mtsComponentInterfaceProxy::FunctionProxySequence::const_iterator itReadEnd = functionProxyPointers.FunctionReadProxies.end();
    for (; itRead != itReadEnd; ++itRead) {
        commandRead = dynamic_cast<mtsCommandReadProxy*>(instance->GetCommandRead(itRead->Name));
        if (!commandRead) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: failed to update command read proxy id: " << itRead->Name << std::endl;
            return false;
        }
        // Set client ID and network proxy
        if (!commandRead->SetNetworkProxy(interfaceProxyServer, clientID)) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: failed to set network proxy: " << itRead->Name << std::endl;
            return false;
        }
        // Set command read proxy's id and enable this command
        commandRead->SetCommandID(itRead->FunctionProxyId);
        commandRead->Enable();
    }

    // QualifiedRead command
    mtsCommandQualifiedReadProxy * commandQualifiedRead = NULL;
    mtsComponentInterfaceProxy::FunctionProxySequence::const_iterator itQualifiedRead = functionProxyPointers.FunctionQualifiedReadProxies.begin();
    const mtsComponentInterfaceProxy::FunctionProxySequence::const_iterator itQualifiedReadEnd = functionProxyPointers.FunctionQualifiedReadProxies.end();
    for (; itQualifiedRead != itQualifiedReadEnd; ++itQualifiedRead) {
        commandQualifiedRead = dynamic_cast<mtsCommandQualifiedReadProxy*>(instance->GetCommandQualifiedRead(itQualifiedRead->Name));
        if (!commandQualifiedRead) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: failed to update command qualifiedRead proxy id: " << itQualifiedRead->Name << std::endl;
            return false;
        }
        // Set client ID and network proxy
        if (!commandQualifiedRead->SetNetworkProxy(interfaceProxyServer, clientID)) {
            CMN_LOG_CLASS_RUN_ERROR << "UpdateCommandProxyID: failed to set network proxy: " << itQualifiedRead->Name << std::endl;
            return false;
        }
        // Set command qualified read proxy's id and enable this command
        commandQualifiedRead->SetCommandID(itQualifiedRead->FunctionProxyId);
        commandQualifiedRead->Enable();
    }

    return true;
}

mtsInterfaceProvided * mtsComponentProxy::CreateInterfaceProvidedInstance(
    const mtsInterfaceProvided * providedInterfaceProxy, unsigned int & instanceID)
{
    // Create a new instance of provided interface proxy
    mtsInterfaceProvided * providedInterfaceInstance = 
        new mtsInterfaceProvided(providedInterfaceProxy->GetName(), this);

    // Clone command object proxies
    mtsInterfaceProvided::CommandVoidMapType::const_iterator itVoidBegin =
        providedInterfaceProxy->CommandsVoid.begin();
    mtsInterfaceProvided::CommandVoidMapType::const_iterator itVoidEnd =
        providedInterfaceProxy->CommandsVoid.end();
    providedInterfaceInstance->CommandsVoid.GetMap().insert(itVoidBegin, itVoidEnd);

    mtsInterfaceProvided::CommandWriteMapType::const_iterator itWriteBegin =
        providedInterfaceProxy->CommandsWrite.begin();
    mtsInterfaceProvided::CommandWriteMapType::const_iterator itWriteEnd =
        providedInterfaceProxy->CommandsWrite.end();
    providedInterfaceInstance->CommandsWrite.GetMap().insert(itWriteBegin, itWriteEnd);

    mtsInterfaceProvided::CommandReadMapType::const_iterator itReadBegin =
        providedInterfaceProxy->CommandsRead.begin();
    mtsInterfaceProvided::CommandReadMapType::const_iterator itReadEnd =
        providedInterfaceProxy->CommandsRead.end();
    providedInterfaceInstance->CommandsRead.GetMap().insert(itReadBegin, itReadEnd);

    mtsInterfaceProvided::CommandQualifiedReadMapType::const_iterator itQualifiedReadBegin =
        providedInterfaceProxy->CommandsQualifiedRead.begin();
    mtsInterfaceProvided::CommandQualifiedReadMapType::const_iterator itQualifiedReadEnd =
        providedInterfaceProxy->CommandsQualifiedRead.end();
    providedInterfaceInstance->CommandsQualifiedRead.GetMap().insert(itQualifiedReadBegin, itQualifiedReadEnd);

    mtsInterfaceProvided::EventVoidMapType::const_iterator itEventVoidGeneratorBegin =
        providedInterfaceProxy->EventVoidGenerators.begin();
    mtsInterfaceProvided::EventVoidMapType::const_iterator itEventVoidGeneratorEnd =
        providedInterfaceProxy->EventVoidGenerators.end();
    providedInterfaceInstance->EventVoidGenerators.GetMap().insert(itEventVoidGeneratorBegin, itEventVoidGeneratorEnd);

    mtsInterfaceProvided::EventWriteMapType::const_iterator itEventWriteGeneratorBegin =
        providedInterfaceProxy->EventWriteGenerators.begin();
    mtsInterfaceProvided::EventWriteMapType::const_iterator itEventWriteGeneratorEnd =
        providedInterfaceProxy->EventWriteGenerators.end();
    providedInterfaceInstance->EventWriteGenerators.GetMap().insert(itEventWriteGeneratorBegin, itEventWriteGeneratorEnd);

    // Don't need to clone queued void and queued write commands because 
    // a server component proxy is created as a device which only can have 
    // device-type interface (of type mtsInterfaceProvided).

    // Assign a new provided interface proxy instance id
    instanceID = ++InterfaceProvidedProxyInstanceID;
    InterfaceProvidedProxyInstanceMap.insert(std::make_pair(instanceID, providedInterfaceInstance));

    return providedInterfaceInstance;
}

bool mtsComponentProxy::GetFunctionProxyPointers(const std::string & requiredInterfaceName,
    mtsComponentInterfaceProxy::FunctionProxyPointerSet & functionProxyPointers)
{
    // Get required interface proxy
    mtsInterfaceRequired * requiredInterfaceProxy = GetInterfaceRequired(requiredInterfaceName);
    if (!requiredInterfaceProxy) {
        CMN_LOG_CLASS_RUN_ERROR << "GetFunctionProxyPointers: failed to get required interface proxy: " << requiredInterfaceName << std::endl;
        return false;
    }

    // Get function proxy and event handler proxy map element
    FunctionProxyAndEventHandlerProxyMapElement * mapElement = FunctionProxyAndEventHandlerProxyMap.GetItem(requiredInterfaceName);
    if (!mapElement) {
        CMN_LOG_CLASS_RUN_ERROR << "GetFunctionProxyPointers: failed to get proxy map element: " << requiredInterfaceName << std::endl;
        return false;
    }

    mtsComponentInterfaceProxy::FunctionProxyInfo function;

    // Void function proxy
    FunctionVoidProxyMapType::const_iterator itVoid = mapElement->FunctionVoidProxyMap.begin();
    const FunctionVoidProxyMapType::const_iterator itVoidEnd = mapElement->FunctionVoidProxyMap.end();
    for (; itVoid != itVoidEnd; ++itVoid) {
        function.Name = itVoid->first;
        function.FunctionProxyId = reinterpret_cast<CommandIDType>(itVoid->second);
        functionProxyPointers.FunctionVoidProxies.push_back(function);
    }

    // Write function proxy
    FunctionWriteProxyMapType::const_iterator itWrite = mapElement->FunctionWriteProxyMap.begin();
    const FunctionWriteProxyMapType::const_iterator itWriteEnd = mapElement->FunctionWriteProxyMap.end();
    for (; itWrite != itWriteEnd; ++itWrite) {
        function.Name = itWrite->first;
        function.FunctionProxyId = reinterpret_cast<CommandIDType>(itWrite->second);
        functionProxyPointers.FunctionWriteProxies.push_back(function);
    }

    // Read function proxy
    FunctionReadProxyMapType::const_iterator itRead = mapElement->FunctionReadProxyMap.begin();
    const FunctionReadProxyMapType::const_iterator itReadEnd = mapElement->FunctionReadProxyMap.end();
    for (; itRead != itReadEnd; ++itRead) {
        function.Name = itRead->first;
        function.FunctionProxyId = reinterpret_cast<CommandIDType>(itRead->second);
        functionProxyPointers.FunctionReadProxies.push_back(function);
    }

    // QualifiedRead function proxy
    FunctionQualifiedReadProxyMapType::const_iterator itQualifiedRead = mapElement->FunctionQualifiedReadProxyMap.begin();
    const FunctionQualifiedReadProxyMapType::const_iterator itQualifiedReadEnd = mapElement->FunctionQualifiedReadProxyMap.end();
    for (; itQualifiedRead != itQualifiedReadEnd; ++itQualifiedRead) {
        function.Name = itQualifiedRead->first;
        function.FunctionProxyId = reinterpret_cast<CommandIDType>(itQualifiedRead->second);
        functionProxyPointers.FunctionQualifiedReadProxies.push_back(function);
    }

    return true;
}

bool mtsComponentProxy::GetEventGeneratorProxyPointer(
    const std::string & clientComponentName, const std::string & requiredInterfaceName,
    mtsComponentInterfaceProxy::EventGeneratorProxyPointerSet & eventGeneratorProxyPointers)
{
    mtsManagerLocal * localManager = mtsManagerLocal::GetInstance();
    mtsDevice * clientComponent = localManager->GetComponent(clientComponentName);
    if (!clientComponent) {
        CMN_LOG_CLASS_RUN_ERROR << "GetEventGeneratorProxyPointer: no client component found: " << clientComponentName << std::endl;
        return false;
    }

    mtsInterfaceRequired * requiredInterface = clientComponent->GetInterfaceRequired(requiredInterfaceName);
    if (!requiredInterface) {
        CMN_LOG_CLASS_RUN_ERROR << "GetEventGeneratorProxyPointer: no required interface found: " << requiredInterfaceName << std::endl;
        return false;
    }
    mtsInterfaceProvided * providedInterface = requiredInterface->GetConnectedInterface();
    if (!providedInterface) {
        CMN_LOG_CLASS_RUN_ERROR << "GetEventGeneratorProxyPointer: no connected provided interface found" << std::endl;
        return false;
    }

    mtsComponentInterfaceProxy::EventGeneratorProxyElement element;
    mtsCommandBase * eventGenerator = NULL;

    std::vector<std::string> namesOfEventHandlersVoid = requiredInterface->GetNamesOfEventHandlersVoid();
    for (unsigned int i = 0; i < namesOfEventHandlersVoid.size(); ++i) {
        element.Name = namesOfEventHandlersVoid[i];
        eventGenerator = providedInterface->EventVoidGenerators.GetItem(element.Name);
        if (!eventGenerator) {
            CMN_LOG_CLASS_RUN_ERROR << "GetEventGeneratorProxyPointer: no event void generator found: " << element.Name << std::endl;
            return false;
        }
        element.EventGeneratorProxyId = reinterpret_cast<CommandIDType>(eventGenerator);
        eventGeneratorProxyPointers.EventGeneratorVoidProxies.push_back(element);
    }

    std::vector<std::string> namesOfEventHandlersWrite = requiredInterface->GetNamesOfEventHandlersWrite();
    for (unsigned int i = 0; i < namesOfEventHandlersWrite.size(); ++i) {
        element.Name = namesOfEventHandlersWrite[i];
        eventGenerator = providedInterface->EventWriteGenerators.GetItem(element.Name);
        if (!eventGenerator) {
            CMN_LOG_CLASS_RUN_ERROR << "GetEventGeneratorProxyPointer: no event write generator found: " << element.Name << std::endl;
            return false;
        }
        element.EventGeneratorProxyId = reinterpret_cast<CommandIDType>(eventGenerator);
        eventGeneratorProxyPointers.EventGeneratorWriteProxies.push_back(element);
    }

    return true;
}

std::string mtsComponentProxy::GetInterfaceProvidedUserName(
    const std::string & processName, const std::string & componentName)
{
    return std::string(processName + ":" + componentName);
}

//-------------------------------------------------------------------------
//  Utilities
//-------------------------------------------------------------------------
void mtsComponentProxy::ExtractInterfaceProvidedDescription(
    mtsInterfaceProvided * providedInterface, unsigned int userId, InterfaceProvidedDescription & providedInterfaceDescription)
{
    if (!providedInterface) return;

    // Serializer initialization
    std::stringstream streamBuffer;
    cmnSerializer serializer(streamBuffer);

    // Extract void commands
    /*
    CommandVoidElement elementCommandVoid;
    mtsInterfaceProvided::CommandVoidMapType::MapType::const_iterator itVoid = providedInterface->CommandsVoid.begin();
    const mtsInterfaceProvided::CommandVoidMapType::MapType::const_iterator itVoidEnd = providedInterface->CommandsVoid.end();
    for (; itVoid != itVoidEnd; ++itVoid) {
        elementCommandVoid.Name = itVoid->second->GetName();
        providedInterfaceDescription.CommandsVoid.push_back(elementCommandVoid);
    }
    */
    mtsCommandVoidBase * voidCommand;
    CommandVoidElement elementCommandVoid;
    const std::vector<std::string> namesOfVoidCommand = providedInterface->GetNamesOfCommandsVoid();
    for (size_t i = 0; i < namesOfVoidCommand.size(); ++i) {
        // Get void command.  Note that there are two different kinds of void
        // command depending on a type of the provided interface.  If a device
        // owns the provided interface, a void command is non-queued command.
        // if a task owns the provided interface, a void command is a queued one.
        voidCommand = providedInterface->GetCommandVoid(namesOfVoidCommand[i], userId);
        if (!voidCommand) {
            // If special case
            if (userId == 0) {
                mtsTaskInterface * interfaceProvided = dynamic_cast<mtsTaskInterface *>(providedInterface);
                if (interfaceProvided) {
                    voidCommand = interfaceProvided->CommandsQueuedVoid.begin()->second;
                } else {
                    voidCommand = providedInterface->CommandsVoid.begin()->second;
                }
            } else {
                CMN_LOG_RUN_ERROR << "ExtractInterfaceProvidedDescription: null void command: " 
                    << namesOfVoidCommand[i] << std::endl;
                return;
            }
        }
        elementCommandVoid.Name = voidCommand->GetName();
        providedInterfaceDescription.CommandsVoid.push_back(elementCommandVoid);
    }

    // Extract write commands
    /*
    CommandWriteElement elementCommandWrite;
    mtsInterfaceProvided::CommandWriteMapType::MapType::const_iterator itWrite = providedInterface->CommandsWrite.begin();
    const mtsInterfaceProvided::CommandWriteMapType::MapType::const_iterator itWriteEnd = providedInterface->CommandsWrite.end();
    for (; itWrite != itWriteEnd; ++itWrite) {
        elementCommandWrite.Name = itWrite->second->GetName();
        elementCommandWrite.Category = 0;
        // argument serialization
        streamBuffer.str("");
        serializer.Serialize(*(itWrite->second->GetArgumentPrototype()));
        elementCommandWrite.ArgumentPrototypeSerialized = streamBuffer.str();
        providedInterfaceDescription.CommandsWrite.push_back(elementCommandWrite);
    }
    */
    mtsCommandWriteBase * writeCommand;
    CommandWriteElement elementCommandWrite;
    const std::vector<std::string> namesOfWriteCommand = providedInterface->GetNamesOfCommandsWrite();
    for (size_t i = 0; i < namesOfWriteCommand.size(); ++i) {
        // Get write command.  Note that there are two different kinds of write
        // command depending on a type of the provided interface.  If a device
        // owns the provided interface, a write command is a non-queued command.
        // if a task owns the provided interface, a write command is a queued one.
        writeCommand = providedInterface->GetCommandWrite(namesOfWriteCommand[i], userId);
        if (!writeCommand) {
            // If special case
            if (userId == 0) {
                mtsTaskInterface * interfaceProvided = dynamic_cast<mtsTaskInterface *>(providedInterface);
                if (interfaceProvided) {
                    writeCommand = interfaceProvided->CommandsQueuedWrite.begin()->second;
                } else {
                    writeCommand = providedInterface->CommandsWrite.begin()->second;
                }
            } else {
                CMN_LOG_RUN_ERROR << "ExtractInterfaceProvidedDescription: null write command: " 
                    << namesOfWriteCommand[i] << std::endl;
                return;
            }
        }
        elementCommandWrite.Name = writeCommand->GetName();
        // serialize argument
        streamBuffer.str("");
        serializer.Serialize(*(writeCommand->GetArgumentPrototype()));
        elementCommandWrite.ArgumentPrototypeSerialized = streamBuffer.str();
        providedInterfaceDescription.CommandsWrite.push_back(elementCommandWrite);
    }

    // Extract filtered write commands: mtsInterfaceProvided::CommandsInternals 
    // have two different types of command--filter command and write command.
    // Since only the write command should be exposed to clients, we extract 
    // write commands.
    // See mtsTaskInterface::AddCommandFilteredWrite() for more details.
    /*
    size_t pos;
    mtsCommandWriteBase * writeCommand;
    mtsInterfaceProvided::CommandInternalMapType::MapType::const_iterator itFilteredWrite = providedInterface->CommandsInternal.begin();
    const mtsInterfaceProvided::CommandInternalMapType::MapType::const_iterator itFilteredWriteEnd = providedInterface->CommandsInternal.end();
    for (; itFilteredWrite != itFilteredWriteEnd; ++itFilteredWrite) {
        elementCommandWrite.Name = itFilteredWrite->second->GetName();
        elementCommandWrite.Category = 1;
        pos = elementCommandWrite.Name.rfind("Write");
        if (pos != std::string::npos && 
            elementCommandWrite.Name.substr(pos).compare("Write") == 0) 
        {
            writeCommand = dynamic_cast<mtsCommandWriteBase*>(itFilteredWrite->second);
            if (!writeCommand) {
                CMN_LOG_RUN_ERROR << "ExtractInterfaceProvidedDescription: invalid write command in filtered write command: "
                    << elementCommandWrite.Name << std::endl;
                continue;
            }
            // argument serialization
            streamBuffer.str("");
            serializer.Serialize(*(writeCommand->GetArgumentPrototype()));
            elementCommandWrite.ArgumentPrototypeSerialized = streamBuffer.str();
            providedInterfaceDescription.CommandsWrite.push_back(elementCommandWrite);
        }
    }
    */

    // Extract read commands
    /*
    CommandReadElement elementCommandRead;
    mtsInterfaceProvided::CommandReadMapType::MapType::const_iterator itRead = providedInterface->CommandsRead.begin();
    const mtsInterfaceProvided::CommandReadMapType::MapType::const_iterator itReadEnd = providedInterface->CommandsRead.end();
    for (; itRead != itReadEnd; ++itRead) {
        elementCommandRead.Name = itRead->second->GetName();
        // argument serialization
        streamBuffer.str("");
        serializer.Serialize(*(itRead->second->GetArgumentPrototype()));
        elementCommandRead.ArgumentPrototypeSerialized = streamBuffer.str();
        providedInterfaceDescription.CommandsRead.push_back(elementCommandRead);
    }
    */
    mtsCommandReadBase * readCommand;
    CommandReadElement elementCommandRead;
    const std::vector<std::string> namesOfReadCommand = providedInterface->GetNamesOfCommandsRead();
    for (size_t i = 0; i < namesOfReadCommand.size(); ++i) {
        readCommand = providedInterface->GetCommandRead(namesOfReadCommand[i]);
        elementCommandRead.Name = readCommand->GetName();
        // serialize argument
        streamBuffer.str("");
        serializer.Serialize(*(readCommand->GetArgumentPrototype()));
        elementCommandRead.ArgumentPrototypeSerialized = streamBuffer.str();
        providedInterfaceDescription.CommandsRead.push_back(elementCommandRead);
    }

    // Extract qualified read commands
    /*
    CommandQualifiedReadElement elementCommandQualifiedRead;
    mtsInterfaceProvided::CommandQualifiedReadMapType::MapType::const_iterator itQualifiedRead = providedInterface->CommandsQualifiedRead.begin();
    const mtsInterfaceProvided::CommandQualifiedReadMapType::MapType::const_iterator itQualifiedReadEnd = providedInterface->CommandsQualifiedRead.end();
    for (; itQualifiedRead != itQualifiedReadEnd; ++itQualifiedRead) {
        elementCommandQualifiedRead.Name = itQualifiedRead->second->GetName();
        // argument1 serialization
        streamBuffer.str("");
        serializer.Serialize(*(itQualifiedRead->second->GetArgument1Prototype()));
        elementCommandQualifiedRead.Argument1PrototypeSerialized = streamBuffer.str();
        // argument2 serialization
        streamBuffer.str("");
        serializer.Serialize(*(itQualifiedRead->second->GetArgument2Prototype()));
        elementCommandQualifiedRead.Argument2PrototypeSerialized = streamBuffer.str();
        providedInterfaceDescription.CommandsQualifiedRead.push_back(elementCommandQualifiedRead);
    }
    */
    mtsCommandQualifiedReadBase * qualifiedReadCommand;
    CommandQualifiedReadElement elementCommandQualifiedRead;
    const std::vector<std::string> namesOfQualifiedReadCommand = providedInterface->GetNamesOfCommandsQualifiedRead();
    for (size_t i = 0; i < namesOfQualifiedReadCommand.size(); ++i) {
        qualifiedReadCommand = providedInterface->GetCommandQualifiedRead(namesOfQualifiedReadCommand[i]);
        elementCommandQualifiedRead.Name = qualifiedReadCommand->GetName();
        // serialize argument1
        streamBuffer.str("");
        serializer.Serialize(*(qualifiedReadCommand->GetArgument1Prototype()));
        elementCommandQualifiedRead.Argument1PrototypeSerialized = streamBuffer.str();
        // serialize argument2
        streamBuffer.str("");
        serializer.Serialize(*(qualifiedReadCommand->GetArgument2Prototype()));
        elementCommandQualifiedRead.Argument2PrototypeSerialized = streamBuffer.str();
        providedInterfaceDescription.CommandsQualifiedRead.push_back(elementCommandQualifiedRead);
    }

    // Extract void events
    EventVoidElement elementEventVoid;
    mtsInterfaceProvided::EventVoidMapType::MapType::const_iterator itEventVoid = providedInterface->EventVoidGenerators.begin();
    const mtsInterfaceProvided::EventVoidMapType::MapType::const_iterator itEventVoidEnd = providedInterface->EventVoidGenerators.end();
    for (; itEventVoid != itEventVoidEnd; ++itEventVoid) {
        elementEventVoid.Name = itEventVoid->second->GetName();
        providedInterfaceDescription.EventsVoid.push_back(elementEventVoid);
    }

    // Extract write events
    EventWriteElement elementEventWrite;
    mtsInterfaceProvided::EventWriteMapType::MapType::const_iterator itEventWrite = providedInterface->EventWriteGenerators.begin();
    const mtsInterfaceProvided::EventWriteMapType::MapType::const_iterator itEventWriteEnd = providedInterface->EventWriteGenerators.end();
    for (; itEventWrite != itEventWriteEnd; ++itEventWrite) {
        elementEventWrite.Name = itEventWrite->second->GetName();
        // serialize argument
        streamBuffer.str("");
        serializer.Serialize(*(itEventWrite->second->GetArgumentPrototype()));
        elementEventWrite.ArgumentPrototypeSerialized = streamBuffer.str();
        providedInterfaceDescription.EventsWrite.push_back(elementEventWrite);
    }
}

void mtsComponentProxy::ExtractInterfaceRequiredDescription(
    mtsInterfaceRequired * requiredInterface, InterfaceRequiredDescription & requiredInterfaceDescription)
{
    // Serializer initialization
    std::stringstream streamBuffer;
    cmnSerializer serializer(streamBuffer);

    // Extract void functions
    requiredInterfaceDescription.FunctionVoidNames = requiredInterface->GetNamesOfFunctionsVoid();
    // Extract write functions
    requiredInterfaceDescription.FunctionWriteNames = requiredInterface->GetNamesOfFunctionsWrite();
    // Extract read functions
    requiredInterfaceDescription.FunctionReadNames = requiredInterface->GetNamesOfFunctionsRead();
    // Extract qualified read functions
    requiredInterfaceDescription.FunctionQualifiedReadNames = requiredInterface->GetNamesOfFunctionsQualifiedRead();

    // Extract void event handlers
    CommandVoidElement elementEventVoidHandler;
    mtsInterfaceRequired::EventHandlerVoidMapType::MapType::const_iterator itVoid = requiredInterface->EventHandlersVoid.begin();
    const mtsInterfaceRequired::EventHandlerVoidMapType::MapType::const_iterator itVoidEnd = requiredInterface->EventHandlersVoid.end();
    for (; itVoid != itVoidEnd; ++itVoid) {
        elementEventVoidHandler.Name = itVoid->second->GetName();
        requiredInterfaceDescription.EventHandlersVoid.push_back(elementEventVoidHandler);
    }

    // Extract write event handlers
    CommandWriteElement elementEventWriteHandler;
    mtsInterfaceRequired::EventHandlerWriteMapType::MapType::const_iterator itWrite = requiredInterface->EventHandlersWrite.begin();
    const mtsInterfaceRequired::EventHandlerWriteMapType::MapType::const_iterator itWriteEnd = requiredInterface->EventHandlersWrite.end();
    for (; itWrite != itWriteEnd; ++itWrite) {
        elementEventWriteHandler.Name = itWrite->second->GetName();
        // argument serialization
        streamBuffer.str("");
        serializer.Serialize(*(itWrite->second->GetArgumentPrototype()));
        elementEventWriteHandler.ArgumentPrototypeSerialized = streamBuffer.str();
        requiredInterfaceDescription.EventHandlersWrite.push_back(elementEventWriteHandler);
    }
}

bool mtsComponentProxy::AddConnectionInformation(const unsigned int providedInterfaceProxyInstanceID,
    const std::string & clientProcessName, const std::string & clientComponentName, const std::string & clientInterfaceRequiredName,
    const std::string & serverProcessName, const std::string & serverComponentName, const std::string & serverInterfaceProvidedName)
{
    mtsComponentInterfaceProxyServer * interfaceProxyServer = InterfaceProvidedNetworkProxies.GetItem(serverInterfaceProvidedName);
    if (!interfaceProxyServer) {
        CMN_LOG_CLASS_RUN_ERROR << "AddConnectionInformation: no interface proxy server found: " << serverInterfaceProvidedName << std::endl;
        return false;
    }

    return interfaceProxyServer->AddConnectionInformation(providedInterfaceProxyInstanceID,
        clientProcessName, clientComponentName, clientInterfaceRequiredName,
        serverProcessName, serverComponentName, serverInterfaceProvidedName);
}
