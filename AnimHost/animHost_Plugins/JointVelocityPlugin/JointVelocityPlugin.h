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

 

#ifndef JOINTVELOCITYPLUGINPLUGIN_H
#define JOINTVELOCITYPLUGINPLUGIN_H

#include "JointVelocityPlugin_global.h"
#include <QMetaType>
#include <plugininterface.h>

class JOINTVELOCITYPLUGINSHARED_EXPORT JointVelocityPlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.JointVelocity" FILE "JointVelocityPlugin.json")
    Q_INTERFACES(PluginInterface)

public:
    JointVelocityPlugin();
    ~JointVelocityPlugin();
    void run(QVariantList in, QVariantList& out) override;
    QObject* getObject() { return this; }

    //QTNodes
    QString category() override;  // Returns a category for the node
    QList<QMetaType> inputTypes() override;  // Returns input data types
    QList<QMetaType> outputTypes() override;  // Returns output data types


};

#endif // JOINTVELOCITYPLUGINPLUGIN_H
