#include "animhostcore.h"
#include "animhostnode.h"
#include "sourcedatanode.h"
#include "assimploadernode.h"
#include <iostream>
#include <QCoreApplication>
#include <QPluginLoader>
#include <QCryptographicHash>


//!
//! \brief Constructor of AnimHost class
//!
AnimHost::AnimHost()
{
    qRegisterMetaType<HumanoidBones>("HumanoidBones");
    qRegisterMetaType<Pose>("Pose");
    qRegisterMetaType<Skeleton>("Skeleton");
    qRegisterMetaType<Animation>("Animation");

    //initalize list for nodes
    nodes = std::make_shared<NodeDelegateModelRegistry>();
    //load existing Plugins
    loadPlugins();

    nodes->registerModel<SourceDataNode>("TestData");
    nodes->registerModel<AssimpLoaderNode>("Data Loader");

    qDebug() << "Hello";



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
            //pluginLoader.unload();
        }
    }

    return false;
}

//!
//! \brief create a UI node from a plugin
//! \param plugin the plugin to be represented
//!
void AnimHost::createNodeFromPlugin(std::shared_ptr<PluginInterface> plugin)
{
    //auto up_plugin = std::unique_ptr<PluginInterface>(plugin);
    //AnimHostNode* node = new AnimHostNode(plugin);
   


    //ret->registerModel<AnimHostNode>("hello");

    NodeDelegateModelRegistry::RegistryItemCreator creator = [p=plugin]() { return std::make_unique<AnimHostNode>(p); };
    nodes->registerModel<AnimHostNode>(std::move(creator), plugin->category());

}
