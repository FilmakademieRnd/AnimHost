 #include "animhost.h"
#include <iostream>
#include <QCoreApplication>
#include <QPluginLoader>
#include <QCryptographicHash>

AnimHost::AnimHost(QString filePath)
{
    //process filePath String
    basePath = QFileInfo(filePath);

    //load existing Plugins
    loadPlugins();
}

//run the animHost
void AnimHost::run()
{
    //check file path
    if (!basePath.exists())
    {
        return;
    }
}

//register a plugin to animHost
void AnimHost::registerPlugin(PluginInterface* plugin, QList<QVariant> inputs, QList<QVariant>* outputs)
{
    //add plugin to list
    plugins.insert(plugin->name(),plugin);

    //connect "done" signal of plugin to slot
    //QObject::connect(plugin, &PluginInterface::done, xxx, xxx);
}

//load all available plugins
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

                registerPlugin(pluginInterface, inputs, outputs);
                return true;
            }
            pluginLoader.unload();
        }
    }

    return false;
}
