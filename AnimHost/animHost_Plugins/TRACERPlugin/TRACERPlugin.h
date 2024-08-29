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

#include <QMetaType>
#include <QtCore/QObject>
#include <PluginNodeCollectionInterface.h>
#include <commondatatypes.h>
#include <nodedatatypes.h>

//#include "YourNodeHeader.h"


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

    public:
        TRACERPlugin() { qDebug() << "TRACERPlugin created"; };
        TRACERPlugin(const TRACERPlugin& p) {};

        ~TRACERPlugin() { qDebug() << "~TRACERPlugin()"; };

       void PreNodeCollectionRegistration() override {
            // Initialize here
       };

       void RegisterNodeCollection(NodeDelegateModelRegistry& nodeRegistry) override {
           // Register nodes here
           //nodeRegistry.registerModel<YourNode>([this](){ return  std::make_unique<YourNode>(YourSharedResources); });
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

};

#endif // TRACERPLUGIN_H