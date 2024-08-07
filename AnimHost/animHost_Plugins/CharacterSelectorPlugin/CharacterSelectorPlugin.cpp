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

 

#include "CharacterSelectorPlugin.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>

CharacterSelectorPlugin::CharacterSelectorPlugin()
{
    _widget = nullptr;
    _selectionLayout = nullptr;
    _selectionMenu = nullptr;
    
    _characterOut = std::make_shared<AnimNodeData<CharacterObject>>();
    
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
        return AnimNodeData<CharacterObjectSequence>::staticType();
    else
        return AnimNodeData<CharacterObject>::staticType();
}

void CharacterSelectorPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    if (!data) {
        Q_EMIT dataInvalidated(0);
    }
    _characterListIn = std::static_pointer_cast<AnimNodeData<CharacterObjectSequence>>(data);

    if (auto spCharacterList = _characterListIn.lock()) {
        _selectionMenu->clear();
        for (CharacterObject chpkg : spCharacterList->getData()->mCharacterObjectSequence) {
            qDebug() << "Received Character" << chpkg.objectName << "with ID" << chpkg.sceneObjectID;
            _selectionMenu->addItem(QString::fromStdString(chpkg.objectName));
        }
        _widget->adjustSize();
        _widget->updateGeometry();
    } else {
        return;
    }
    qDebug() << "CharacterSelectorPlugin setInData";
}

bool CharacterSelectorPlugin::isDataAvailable() {
    return !_characterListIn.expired();
}

void CharacterSelectorPlugin::run() { emitRunNextNode(); }

std::shared_ptr<NodeData> CharacterSelectorPlugin::processOutData(QtNodes::PortIndex port)
{
	return _characterOut;
}

QWidget* CharacterSelectorPlugin::embeddedWidget()
{
    if (!_widget) {
        _selectionLayout = new QVBoxLayout();
        _selectionMenu = new QComboBox();
        _selectionMenu->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
        connect(_selectionMenu, &QComboBox::currentIndexChanged, this, &CharacterSelectorPlugin::onChangedSelection);

        _selectionLayout->addWidget(_selectionMenu);

        _selectionLayout->setSizeConstraint(QLayout::SetMinimumSize);
        

      

        _widget = new QWidget();
        _widget->setLayout(_selectionLayout);
        _widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
                               "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
                               "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
                               "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}");
    }

    return _widget;
}

void CharacterSelectorPlugin::onChangedSelection(int index)
{
	qDebug() << "Character Selection Changed";
    if (index >= 0) {
        if (auto spCharacterList = _characterListIn.lock()) {
            // Overwrite _characterOut with new selected element
            CharacterObject selectedCharacter = spCharacterList->getData()->mCharacterObjectSequence.at(index);
            _characterOut->getData()->fill(selectedCharacter);
            emitDataUpdate(0);
        }
    } else {
        emitDataInvalidated(0);
    }
}
