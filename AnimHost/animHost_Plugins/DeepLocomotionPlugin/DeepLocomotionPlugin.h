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


#ifndef DEEPLOCOMOTIONPLUGIN_H
#define DEEPLOCOMOTIONPLUGIN_H

#include "DeepLocomotionPlugin_global.h"

#include <QMetaType>
#include <QtCore/QObject>
#include <PluginNodeCollectionInterface.h>
#include <commondatatypes.h>
#include <nodedatatypes.h>

#include "GNN/GNNNode.h"
#include "LocomotionPreprocess/LocomotionPreprocessNode.h"

//#include "YourNodeHeader.h"


class DEEPLOCOMOTIONPLUGINSHARED_EXPORT DeepLocomotionPlugin : public PluginNodeCollectionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginNodeCollectionInterface" FILE "DeepLocomotionPlugin.json")
    Q_INTERFACES(PluginNodeCollectionInterface)


        CollectionMetaData _collectionMetaData = {
            "DeepLocomotion",
            "Collection description",
            "0.1",
            "AUTHOR",
            "LICENSE"
        };

    // Add shared data here

    public:
        DeepLocomotionPlugin() { qDebug() << "DeepLocomotionPlugin created"; };
        DeepLocomotionPlugin(const DeepLocomotionPlugin& p) {};

        ~DeepLocomotionPlugin() { qDebug() << "~DeepLocomotionPlugin()"; };

       void PreNodeCollectionRegistration() override {
            // Initialize here
       };

       void RegisterNodeCollection(NodeDelegateModelRegistry& nodeRegistry) override {
           // Register nodes here
           nodeRegistry.registerModel<GNNNode>([this](){ return  std::make_unique<GNNNode>(); });
		   nodeRegistry.registerModel<LocomotionPreprocessNode>([this]() { return  std::make_unique<LocomotionPreprocessNode>(); });
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

#endif // DEEPLOCOMOTIONPLUGIN_H