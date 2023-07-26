import os
import sys

def create_plugin(name):
    script_dir = os.path.dirname(os.path.abspath(__file__))  # Get the directory of the script
    plugin_dir = os.path.join(script_dir, f"{name}Plugin")

    if not os.path.exists(plugin_dir):
        os.makedirs(plugin_dir)

    cmakelists_path = os.path.join(script_dir, "CMakeLists.txt")
    add_subdirectory_line = f"add_subdirectory({name}Plugin)\n"

    with open(cmakelists_path, "r") as f:
        if add_subdirectory_line in f.read():
            print(f"'{name}Plugin' already exists in CMakeLists.txt.")
        else:
            with open(cmakelists_path, "a") as f:
                f.write(add_subdirectory_line)

    json_content = "{}"
    with open(os.path.join(plugin_dir, f"{name}Plugin.json"), "w") as f:
        f.write(json_content)

    global_header_content = f"""
#ifndef {name.upper()}PLUGINSHARED_GLOBAL_H
#define {name.upper()}PLUGINSHARED_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined({name.upper()}PLUGIN_LIBRARY)
#define {name.upper()}PLUGINSHARED_EXPORT Q_DECL_EXPORT
#else
#define {name.upper()}PLUGINSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // {name.upper()}PLUGIN_GLOBAL_H
"""
    with open(os.path.join(plugin_dir, f"{name}Plugin_global.h"), "w") as f:
        f.write(global_header_content)

    header_content = f"""
#ifndef {name.upper()}PLUGINPLUGIN_H
#define {name.upper()}PLUGINPLUGIN_H

#include "{name}Plugin_global.h"
#include <QMetaType>
#include <plugininterface.h>

class {name.upper()}PLUGINSHARED_EXPORT {name}Plugin : public PluginInterface
{{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.{name}" FILE "{name}Plugin.json")
    Q_INTERFACES(PluginInterface)

public:
    {name}Plugin();
    ~{name}Plugin();
    void run(QVariantList in, QVariantList& out) override;
    QObject* getObject() {{ return this; }}

    //QTNodes
    QString category() override;  // Returns a category for the node
    QList<QMetaType> inputTypes() override;  // Returns input data types
    QList<QMetaType> outputTypes() override;  // Returns output data types


}};

#endif // {name.upper()}PLUGINPLUGIN_H
"""
    with open(os.path.join(plugin_dir, f"{name}Plugin.h"), "w") as f:
        f.write(header_content)

    cpp_content = f"""
#include "{name}Plugin.h"
#include "../../core/commondatatypes.h"

{name}Plugin::{name}Plugin()
{{
    qDebug() << "{name}Plugin created";

    //Data
    //inputs.append(QMetaType(QMetaType::Int));
    //outputs.append(QMetaType(QMetaType::Float));
}}

{name}Plugin::~{name}Plugin()
{{
    qDebug() << "~{name}Plugin()";
}}

// execute the main functionality of the plugin
void {name}Plugin::run(QVariantList in, QVariantList& out)
{{
    qDebug() << "Running {name}Plugin";
}}

QString {name}Plugin::category()
{{
    return "XXXX";
}}

QList<QMetaType> {name}Plugin::inputTypes()
{{
    return inputs;
}}

QList<QMetaType> {name}Plugin::outputTypes()
{{
    return outputs;
}}
"""
    with open(os.path.join(plugin_dir, f"{name}Plugin.cpp"), "w") as f:
        f.write(cpp_content)

    cmake_content = f"""
qt_add_library({name}Plugin
    {name}Plugin.cpp {name}Plugin.h
    {name}Plugin_global.h
)
target_include_directories({name}Plugin PUBLIC
    ../PluginInterface
    ../../../glm
)

target_compile_definitions({name}Plugin PUBLIC
    {name.upper()}PLUGIN_LIBRARY
)

target_link_libraries({name}Plugin PUBLIC
    PluginInterface
    AnimHostCore
)
"""
    with open(os.path.join(plugin_dir, "CMakeLists.txt"), "w") as f:
        f.write(cmake_content)

    print(f"Plugin '{name}' created successfully.")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py [PluginName]")
        sys.exit(1)

    plugin_name = sys.argv[1]
    create_plugin(plugin_name)
