import os
import sys
import argparse

def write_content_plugin(plugin_dir, name):
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

target_compile_definitions({name}Plugin PRIVAT
    {name.upper()}PLUGIN_LIBRARY
)

target_link_libraries({name}Plugin PUBLIC
    PluginInterface
    AnimHostCore
)
"""
    with open(os.path.join(plugin_dir, "CMakeLists.txt"), "w") as f:
        f.write(cmake_content)


def write_content_node_plugin(plugin_dir, name):
    global_header_content = f"""
#ifndef {name.upper()}PLUGINSHARED_GLOBAL_H
#define {name.upper()}PLUGINSHARED_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined({name.upper()}PLUGIN_LIBRARY)
#define {name.upper()}PLUGINSHARED_EXPORT Q_DECL_EXPORT
#else
#define {name.upper()}PLUGINSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // {name.upper()}PLUGINSHARED_GLOBAL_H
"""
    with open(os.path.join(plugin_dir, f"{name}Plugin_global.h"), "w") as f:
        f.write(global_header_content)

    header_content = f"""
#ifndef {name.upper()}PLUGINPLUGIN_H
#define {name.upper()}PLUGINPLUGIN_H

#include "{name}Plugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>

class QPushButton;

class {name.upper()}PLUGINSHARED_EXPORT {name}Plugin : public PluginNodeInterface
{{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.{name}" FILE "{name}Plugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    QPushButton* _pushButton;

public:
    {name}Plugin();
    ~{name}Plugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override {{ return  std::unique_ptr<{name}Plugin>(new {name}Plugin()); }};

    QString category() override {{ return "Undefined Category"; }};
    QString caption() const override {{ return this->name(); }}
    bool captionVisible() const override {{ return true; }}

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
    void run() override;

    QWidget* embeddedWidget() override;

private Q_SLOTS:
    void onButtonClicked();

}};

#endif // {name.upper()}PLUGINPLUGIN_H
"""
    with open(os.path.join(plugin_dir, f"{name}Plugin.h"), "w") as f:
        f.write(header_content)

    cpp_content = f"""
#include "{name}Plugin.h"
#include <QPushButton>

{name}Plugin::{name}Plugin()
{{
    _pushButton = nullptr;
    qDebug() << "{name}Plugin created";
}}

{name}Plugin::~{name}Plugin()
{{
    qDebug() << "~{name}Plugin()";
}}

unsigned int {name}Plugin::nDataPorts(QtNodes::PortType portType) const
{{
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 0;
}}

NodeDataType {name}Plugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return type;
}}

void {name}Plugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{{
    qDebug() << "{name}Plugin setInData";
}}

std::shared_ptr<NodeData> {name}Plugin::processOutData(QtNodes::PortIndex port)
{{
	return nullptr;
}}

QWidget* {name}Plugin::embeddedWidget()
{{
	if (!_pushButton) {{
		_pushButton = new QPushButton("Example Widget");
		_pushButton->resize(QSize(100, 50));

		connect(_pushButton, &QPushButton::released, this, &{name}Plugin::onButtonClicked);
	}}

	return _pushButton;
}}

void {name}Plugin::onButtonClicked()
{{
	qDebug() << "Example Widget Clicked";
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
    ../PluginNodeInterface
    ../../../glm
)

target_compile_definitions({name}Plugin PRIVATE
    {name.upper()}PLUGIN_LIBRARY
)

target_link_libraries({name}Plugin PUBLIC
    PluginNodeInterface
    AnimHostCore
)
"""
    with open(os.path.join(plugin_dir, "CMakeLists.txt"), "w") as f:
        f.write(cmake_content)

def create_plugin(name, is_node):
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
    if(is_node):
        write_content_node_plugin(plugin_dir, name)
    else:
        write_content_plugin(plugin_dir, name)


    print(f"Plugin '{name}' created successfully.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate files to build your own AnimHost plugin")
    parser.add_argument('pluginname', help="name of the plugin")
    parser.add_argument('--node', dest="node", action='store_true', help="generate a node plugin (default: generate a functional plugin)")

    args = parser.parse_args()

    plugin_name = args.pluginname
    is_node = args.node
    create_plugin(plugin_name, is_node)
