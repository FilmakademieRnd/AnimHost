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

 
#ifndef JOINTPOSITIONPLUGIN_H
#define JOINTPOSITIONPLUGIN_H

#include "JointPositionPlugin_global.h"
#include <QMetaType>
#include <plugininterface.h>


class JOINTPOSITIONPLUGINSHARED_EXPORT JointPositionPlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginInterface" FILE "JointPositionPlugin.json")
    Q_INTERFACES(PluginInterface)

public:
    JointPositionPlugin();
    ~JointPositionPlugin();

    void run(QVariantList in, QVariantList& out) override;
    QObject* getObject() { return this; }



    //QTNodes
    QString name() override { return "Global Joint Positions"; };
    QString category() override;  // Returns a category for the node
    QList<QMetaType> inputTypes() override;  // Returns input data types
    QList<QMetaType> outputTypes() override;  // Returns output data types


};

#endif // JOINTPOSITIONPLUGIN_H
