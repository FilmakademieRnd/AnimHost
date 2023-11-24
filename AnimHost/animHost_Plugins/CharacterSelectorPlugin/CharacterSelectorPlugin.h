
#ifndef CHARACTERSELECTORPLUGINPLUGIN_H
#define CHARACTERSELECTORPLUGINPLUGIN_H

#include "CharacterSelectorPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>

class QComboBox;
class QPushButton;
class QVBoxLayout;
class QWidget;

class CHARACTERSELECTORPLUGINSHARED_EXPORT CharacterSelectorPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.CharacterSelector" FILE "CharacterSelectorPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    QWidget* _widget;
    QVBoxLayout* _selectionLayout;
    QComboBox* _selectionMenu;
    std::weak_ptr<AnimNodeData<CharacterPackageSequence>> _characterListIn;
    std::shared_ptr<AnimNodeData<CharacterPackage>> _characterOut;

public:
    CharacterSelectorPlugin();
    ~CharacterSelectorPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<CharacterSelectorPlugin>(new CharacterSelectorPlugin()); };

    QString category() override { return "Operator"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
    void run() override;

    QWidget* embeddedWidget() override;

private Q_SLOTS:
    void onChangedSelection(int index);

};

#endif // CHARACTERSELECTORPLUGINPLUGIN_H
