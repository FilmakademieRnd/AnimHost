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

 
#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QVariant>
#include <commondatatypes.h>
#include "plugininterface_global.h"

//!
//! \brief Interface for plugins for the AnimHost
//!
class PLUGININTERFACESHARED_EXPORT PluginInterface : public QObject
{

public:
    // main function of the plugin generating outputs based on given inputs
    virtual void run(QVariantList in, QVariantList& out) = 0;
    // provide the name of the plugin
    virtual QString name();

    //QTNodes
    virtual QString category() = 0;  // Returns a category for the node
    virtual QList<QMetaType> inputTypes() = 0;  // Returns input data types
    virtual QList<QMetaType> outputTypes() = 0;  // Returns output data types

    QList<QMetaType> inputs;
    QList<QMetaType> outputs;

};

#define PluginInterface_iid "de.animhost.PluginInterface"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGININTERFACE_H
