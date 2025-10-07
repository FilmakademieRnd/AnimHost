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

 
#ifndef PLUGINNODECOLLECTIONINTERFACE_H
#define PLUGINNODECOLLECTIONINTERFACE_H

#include <animhostcore_global.h>

#include <QObject>
#include <QList>
#include <QString>
#include <QVariant>
#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <nodedatatypes.h>

using QtNodes::NodeDelegateModelRegistry;

//!
//! \brief Interface for plugins for the AnimHost
//!
class ANIMHOSTCORESHARED_EXPORT PluginNodeCollectionInterface : public QObject
{

protected:
    struct CollectionMetaData
    {
        QString name = "";
        QString description = "";
        QString version = "";
        QString author = "";
        QString license = "";
    };

    static CollectionMetaData _collectionMetaData;

public:

    PluginNodeCollectionInterface() {};
    PluginNodeCollectionInterface(const PluginNodeCollectionInterface& p) {};


    virtual ~PluginNodeCollectionInterface() {};

    virtual void PreNodeCollectionRegistration() = 0;

    virtual void RegisterNodeCollection(NodeDelegateModelRegistry& nodeRegistry) = 0;

    virtual void PostNodeCollectionRegistration() = 0;

    virtual QString GetNodeCollectionName() = 0;

    virtual QString GetNodeCollectionDescription() = 0;

    virtual QString GetNodeCollectionMetaData() = 0;    

};

#define PluginNodeCollectionInterface_iid "de.animhost.PluginNodeCollectionInterface"

Q_DECLARE_INTERFACE(PluginNodeCollectionInterface, PluginNodeCollectionInterface_iid)

#endif // PLUGINNODEINTERFACE_H
