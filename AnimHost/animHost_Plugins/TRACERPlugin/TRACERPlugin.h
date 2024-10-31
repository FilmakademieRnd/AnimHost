/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *    
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *    
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program; 
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */


#ifndef TRACERPLUGIN_H
#define TRACERPLUGIN_H

#include "TRACERPlugin_global.h"
#include "TRACERUpdateMessage.h"

#include <QMetaType>
#include <QObject>
#include <QApplication>
#include <PluginNodeCollectionInterface.h>
#include <commondatatypes.h>
#include <nodedatatypes.h>
#include <ZMQMessageHandler.h>

#include "TRACERGlobalTimer.h"
#include "TRACERUpdateReceiver.h"

#include "CharacterSelector/CharacterSelectorNode.h"
#include "UpdateReceiver/UpdateReceiverNode.h"
#include "SceneReceiver/SceneReceiverNode.h"
#include "AnimationSender/AnimationSenderNode.h"


class TRACERPLUGINSHARED_EXPORT TRACERPlugin : public PluginNodeCollectionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginNodeCollectionInterface" FILE "TRACERPlugin.json")
    Q_INTERFACES(PluginNodeCollectionInterface)


        CollectionMetaData _collectionMetaData = {
            "TRACER",
            "Collection description",
            "0.1",
            "AUTHOR",
            "LICENSE"
        };

    // Add shared data here

    std::shared_ptr<TRACERGlobalTimer> _globalTimer = nullptr;
    std::shared_ptr<zmq::context_t> _zmqContext = nullptr;

    std::shared_ptr<TRACERUpdateReceiver> _updateReceiver = nullptr;
    QThread _updateReceiverThread;




    public:
        TRACERPlugin() { 
            qRegisterMetaType<std::shared_ptr<ParameterUpdate>>("ParameterUpdate");
            qRegisterMetaType<std::shared_ptr<RPCUpdate>>("RPCUpdate");
        };
        TRACERPlugin(const TRACERPlugin& p) {};

        ~TRACERPlugin() { 
        };

       void PreNodeCollectionRegistration() override {
            // Initialize here
           _globalTimer = std::make_shared<TRACERGlobalTimer>();
           _globalTimer->startTimer();

           _zmqContext = std::make_shared<zmq::context_t>(1);

           _updateReceiver = std::make_shared<TRACERUpdateReceiver>(true, _zmqContext.get(), _globalTimer);
           _updateReceiver->moveToThread(&_updateReceiverThread);

           ZMQMessageHandler::setClientID(212);

           // Connect signals and slots for the TRACERUpdateReceiver
           //connect(&_updateReceiverThread, &QThread::started, _updateReceiver.get(), &TRACERUpdateReceiver::initializeUpdateReceiverSocket);
           connect(_updateReceiver.get(), &TRACERUpdateReceiver::shutdown, &_updateReceiverThread, &QThread::quit);
           //connect(&_updateReceiverThread, &QThread::finished, _updateReceiver.get(), &TRACERUpdateReceiver::deleteLater);
           connect(&_updateReceiverThread, &QThread::finished, &_updateReceiverThread, &QThread::deleteLater);

           connect(qApp, &QCoreApplication::aboutToQuit, this, &TRACERPlugin::cleanUp);

           //_updateReceiver->requestStart();
           _updateReceiverThread.start();

       };

       void RegisterNodeCollection(NodeDelegateModelRegistry& nodeRegistry) override {
           // Register nodes here
           nodeRegistry.registerModel<CharacterSelectorNode>([this](){ return  std::make_unique<CharacterSelectorNode>();}, "TRACER");
           nodeRegistry.registerModel<SceneReceiverNode>([this]() { return  std::make_unique<SceneReceiverNode>(_zmqContext); }, "TRACER");
           nodeRegistry.registerModel<AnimationSenderNode>([this]() { return  std::make_unique<AnimationSenderNode>(_globalTimer, _zmqContext); }, "TRACER");
           nodeRegistry.registerModel<UpdateReceiverNode>([this]() { return  std::make_unique<UpdateReceiverNode>(_updateReceiver); }, "TRACER");
           nodeRegistry.registerModel<RPCTriggerNode>([this]() { return  std::make_unique<RPCTriggerNode>(); }, "TRACER");
           nodeRegistry.registerModel<ControlPathDecoderNode>([this]() { return  std::make_unique<ControlPathDecoderNode>(); }, "TRACER");
       };

       void PostNodeCollectionRegistration() override {};


       QString GetNodeCollectionName() override {
		   return _collectionMetaData.name;
	   };

       QString GetNodeCollectionDescription() override {
           return _collectionMetaData.description;
       };

       QString GetNodeCollectionMetaData() override {
		   return _collectionMetaData.version;
	   };

       void cleanUp() {
           _updateReceiver->requestStop();

           _globalTimer.reset();

           //wait for the signal finished from the thread
           QThread::msleep(100);

           _zmqContext->shutdown();
           _zmqContext->close();


           QThread::msleep(100);
           _updateReceiver.reset();

           _updateReceiverThread.quit();
           _updateReceiverThread.wait();
       }

};




#endif // TRACERPLUGIN_H