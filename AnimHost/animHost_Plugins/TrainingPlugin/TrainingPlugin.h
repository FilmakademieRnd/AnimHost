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


#ifndef TRAININGPLUGIN_H
#define TRAININGPLUGIN_H

#include "TrainingPlugin_global.h"

#include <QMetaType>
#include <QtCore/QObject>
#include <PluginNodeCollectionInterface.h>
#include <commondatatypes.h>
#include <nodedatatypes.h>

#include "Training/TrainingNode.h"


class TRAININGPLUGINSHARED_EXPORT TrainingPlugin : public PluginNodeCollectionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginNodeCollectionInterface" FILE "TrainingPlugin.json")
    Q_INTERFACES(PluginNodeCollectionInterface)


        CollectionMetaData _collectionMetaData = {
            "Training",
            "Plugin for Python training framework integration",
            "0.1",
            "Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs",
            "MIT"
        };

    // Add shared data here

    public:
        TrainingPlugin() {};
        TrainingPlugin(const TrainingPlugin& p) {};

        ~TrainingPlugin() {};

       void PreNodeCollectionRegistration() override {
            // Initialize here
       };

       void RegisterNodeCollection(NodeDelegateModelRegistry& nodeRegistry) override {
           // Register nodes here
           nodeRegistry.registerModel<TrainingNode>([this](){ return std::make_unique<TrainingNode>(); });
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

#endif // TRAININGPLUGIN_H