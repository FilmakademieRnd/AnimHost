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

 
#ifndef EXAMPLEPLUGIN_H
#define EXAMPLEPLUGIN_H

#include "TestDataSourcePlugin_global.h"
#include <QMetaType>
#include <plugininterface.h>


class TESTDATASOURCEPLUGINSHARED_EXPORT TestDataSourcePlugin : public PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginInterface" FILE "TestDataSourcePlugin.json")
    Q_INTERFACES(PluginInterface)

public:
    TestDataSourcePlugin();
    ~TestDataSourcePlugin();
    void run(QVariantList in, QVariantList& out) override;
    QObject* getObject() { return this; }

    //QTNodes
    QString name() override { return "CSV Export"; }
    QString category() override;  // Returns a category for the node
    QList<QMetaType> inputTypes() override;  // Returns input data types
    QList<QMetaType> outputTypes() override;  // Returns output data types


};

#endif // EXAMPLEPLUGIN_H
