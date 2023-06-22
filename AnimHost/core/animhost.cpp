#include "animhost.h"
#include "animhostnode.h"
#include <iostream>
#include <QCoreApplication>
#include <QPluginLoader>
#include <QCryptographicHash>

//!
//! \brief Constructor of AnimHost class
//!
AnimHost::AnimHost()
{
    //load existing Plugins
    loadPlugins();

    //initalize list for nodes
    nodes = std::make_shared<NodeDelegateModelRegistry>();
}

//!
//! \brief register a plugin to animHost
//! @param plugin the plugin to be registered based on PluginInterface
//!
void AnimHost::registerPlugin(PluginInterface* plugin)
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
#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
    pluginsDir.cd("plugins");
    const QStringList entries = pluginsDir.entryList(QDir::Files);
    for (const QString &fileName : entries) {
        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (plugin) {
            PluginInterface* pluginInterface = qobject_cast<PluginInterface *>(plugin);
            if (pluginInterface)
            {
                //register analyser plugin to requested file types
                QList<QVariant> inputs = pluginInterface->inputs;
                QList<QVariant>* outputs = pluginInterface->outputs;

                registerPlugin(pluginInterface);
                createNodeFromPlugin(pluginInterface);
                return true;
            }
            pluginLoader.unload();
        }
    }

    return false;
}

//!
//! \brief create a UI node from a plugin
//! \param plugin the plugin to be represented
//!
void AnimHost::createNodeFromPlugin(PluginInterface* plugin)
{
    //AnimHostNode node = new AnimHostNode();


    //nodes->registerModel<NumberSourceDataModel>("Sources");

    /*ret->registerModel<NumberDisplayDataModel>("Displays");

    ret->registerModel<AdditionModel>("Operators");

    ret->registerModel<SubtractionModel>("Operators");

    ret->registerModel<MultiplicationModel>("Operators");

    ret->registerModel<DivisionModel>("Operators");*/
}
