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

 
#include "animhostcore.h"
#include "animhostnode.h"
#include "animhostoperationnode.h"
#include "sourcedatanode.h"

#include "commondatatypes.h"

#include <iostream>
#include <QCoreApplication>
#include <QPluginLoader>
#include <QCryptographicHash>

//!
//! \brief Constructor of AnimHost class
//!
AnimHost::AnimHost()
{

    qRegisterMetaType<std::shared_ptr<Pose>>("Pose");
    qRegisterMetaType<std::shared_ptr<PoseSequence>>("PoseSequence");

    qRegisterMetaType<std::shared_ptr<Skeleton>>("Skeleton");

    qRegisterMetaType<std::shared_ptr<Bone>>("Bone");

    qRegisterMetaType<std::shared_ptr<Animation>>("Animation");

    qRegisterMetaType<std::shared_ptr<JointVelocity>>("JointVelocity");
    qRegisterMetaType<std::shared_ptr<JointVelocitySequence>>("JointVelocitySequence");

    qRegisterMetaType<std::shared_ptr<CharacterObject>>("CharacterObject");
    qRegisterMetaType<std::shared_ptr<CharacterObjectSequence>>("CharacterObjectSequence");
 
    qRegisterMetaType<std::shared_ptr<RunSignal>>("RunSignal");


    //initalize list for nodes
    nodes = std::make_shared<NodeDelegateModelRegistry>();
    //load existing Plugins
    loadPlugins();

    nodes->registerModel<SourceDataNode>("TestData");
    //nodes->registerModel<AssimpLoaderNode>("Data Loader");
}

//!
//! \brief register a plugin to animHost
//! @param plugin the plugin to be registered based on PluginInterface
//!
void AnimHost::registerPlugin(std::shared_ptr<PluginInterface> plugin)
{
    //add plugin to list
    plugins.insert(plugin->name(),plugin);

    //connect "done" signal of plugin to slot
    //QObject::connect(plugin, &PluginInterface::done, xxx, xxx);
}

//!
//! load all available plugins based on dynamic libraries
//!
bool AnimHost::loadPlugins()
{
    QDir pluginsDir(QCoreApplication::applicationDirPath());

    const QStringList entries = pluginsDir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        
        qDebug()<< "Try loading as plugin: " << fileName;

        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            PluginInterface* pluginInterface = qobject_cast<PluginInterface* >(plugin);
            if (pluginInterface)
            {
                //register analyser plugin to requested file types
                /*QList<QVariant> inputs = pluginInterface->inputs;
                QList<QVariant>* outputs = pluginInterface->outputs*/;


                std::shared_ptr<PluginInterface> sp_PluginInterface(pluginInterface);

                registerPlugin(sp_PluginInterface);
                createNodeFromPlugin(sp_PluginInterface);
                //return true;
            }
            else
            {
                PluginNodeInterface* pluginNodeInterface = qobject_cast<PluginNodeInterface*>(plugin);
                if (pluginNodeInterface)
                {
                    std::shared_ptr<PluginNodeInterface> sp_PluginNodeInterface(pluginNodeInterface);

                    createNodeFromNodePlugin(sp_PluginNodeInterface);
                }
            }
            //pluginLoader.unload();
        }
    }

    return false;
}

void AnimHost::createNodeFromNodePlugin(std::shared_ptr<PluginNodeInterface> plugin)
{

    NodeDelegateModelRegistry::RegistryItemCreator creator = [p = plugin]() {  return p->Init(); };
    nodes->registerModel<PluginNodeInterface>(std::move(creator), plugin->category());

}

//!
//! \brief create a UI node from a plugin
//! \param plugin the plugin to be represented
//!
void AnimHost::createNodeFromPlugin(std::shared_ptr<PluginInterface> plugin)
{

    NodeDelegateModelRegistry::RegistryItemCreator creator = [p=plugin]() { return std::make_unique<AnimHostOperationNode>(p); };
    nodes->registerModel<AnimHostOperationNode>(std::move(creator), plugin->category());

}
