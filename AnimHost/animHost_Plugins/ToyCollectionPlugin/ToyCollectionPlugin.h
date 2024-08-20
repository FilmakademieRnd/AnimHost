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


#ifndef TOYCOLLECTIONPLUGIN_H
#define TOYCOLLECTIONPLUGIN_H

#include "ToyCollectionPlugin_global.h"

#include <QMetaType>
#include <QtCore/QObject>
#include <PluginNodeCollectionInterface.h>
#include <commondatatypes.h>
#include <nodedatatypes.h>

#include "ToyAlphaNode.h"
#include "ToyBetaNode.h"

class TOYCOLLECTIONPLUGINSHARED_EXPORT ToyCollectionPlugin : public PluginNodeCollectionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginNodeCollectionInterface" FILE "ToyCollectionPlugin.json")
    Q_INTERFACES(PluginNodeCollectionInterface)


        CollectionMetaData _collectionMetaData = {
            "ToyCollection",
            "A collection of toy nodes",
            "1.0",
            "Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs",
            "MIT"
        };

    public:
        ToyCollectionPlugin() { qDebug() << "ToyCollectionPlugin created"; };
        ToyCollectionPlugin(const ToyCollectionPlugin& p) {};

        ~ToyCollectionPlugin() { qDebug() << "~ToyCollectionPlugin()"; };

       void PreNodeCollectionRegistration() override {};

       void RegisterNodeCollection(NodeDelegateModelRegistry& nodeRegistry) override {
           // Register nodes here
           nodeRegistry.registerModel<ToyAlphaNode>(QString("ToyCollection"));
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

#endif // TOYCOLLECTIONPLUGIN_H
