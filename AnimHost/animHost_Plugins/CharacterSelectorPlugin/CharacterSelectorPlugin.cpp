
#include "CharacterSelectorPlugin.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>

CharacterSelectorPlugin::CharacterSelectorPlugin()
{
    _widget = new QWidget();
    _selectionLayout = new QVBoxLayout();
    _selectionMenu = new QComboBox();
    connect(_selectionMenu, &QComboBox::currentIndexChanged, this, &CharacterSelectorPlugin::onChangedSelection);

    _characterOut = std::make_shared<AnimNodeData<CharacterPackage>>();
    
    qDebug() << "CharacterSelectorPlugin created";
}

CharacterSelectorPlugin::~CharacterSelectorPlugin()
{
    qDebug() << "~CharacterSelectorPlugin()";
}

unsigned int CharacterSelectorPlugin::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 1;
    else            
        return 1;
}

NodeDataType CharacterSelectorPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return AnimNodeData<CharacterPackageSequence>::staticType();
    else
        return AnimNodeData<CharacterPackage>::staticType();
}

void CharacterSelectorPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    if (!data) {
        Q_EMIT dataInvalidated(0);
    }
    _characterListIn = std::static_pointer_cast<AnimNodeData<CharacterPackageSequence>>(data);

    if (auto spCharacterList = _characterListIn.lock()) {
        _selectionMenu->clear();
        for (CharacterPackage chpkg : spCharacterList->getData()->mCharacterPackageSequence) {
            qDebug() << "Received Character" << chpkg.objectName << "with ID" << chpkg.sceneObjectID;
            _selectionMenu->addItem(QString::fromStdString(chpkg.objectName));
        }
        _widget->adjustSize();
    } else {
        return;
    }
    qDebug() << "CharacterSelectorPlugin setInData";
}

void CharacterSelectorPlugin::run() { emitRunNextNode(); }

std::shared_ptr<NodeData> CharacterSelectorPlugin::processOutData(QtNodes::PortIndex port)
{
	return _characterOut;
}

QWidget* CharacterSelectorPlugin::embeddedWidget()
{
    
    _selectionLayout->addWidget(_selectionMenu);

    _widget->setLayout(_selectionLayout);
    _widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
                          "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
                          "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
                          "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}");

    return _widget;
}

void CharacterSelectorPlugin::onChangedSelection(int index)
{
	qDebug() << "Selection Changed";
    if (index >= 0) {
        if (auto spCharacterList = _characterListIn.lock()) {
            //! Overwrite _characterOut with new selected element
            CharacterPackage selectedCharacter = spCharacterList->getData()->mCharacterPackageSequence.at(index);

            //auto qvChar = QVariant::fromValue(selectedCharacter);

            //auto whatever = qvChar.value<CharacterPackage>();

            //_characterOut->setVariant(qvChar);
            _characterOut->getData()->fill(selectedCharacter);
            emitDataUpdate(0);
        }
    } else {
        emitDataInvalidated(0);
    }
}
