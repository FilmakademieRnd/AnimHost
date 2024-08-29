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



 //!
 //! @file "CharacterSelectorNode.h"
 //! @implements PluginNodeInterface
 //! @brief Plugin allowing the user to select which character in the received scene will receive the Parameter Update message
 //! @param[in]  _characterListIn    The list of character in the scene to choose from
 //! @param[out] _characterOut       The selected character to which the animation is going to be applied
 //! @author Francesco Andreussi
 //! @version 0.5
 //! @date 26.01.2024
 //!
 /*!
  * ###Plugin class with UI elements for selecting the character that is going to be animated
  * Using the UI elements, it's possible to select an a character out of the list of characters that are present in the scene
  */




#ifndef CHARACTERSELECTOR_H
#define CHARACTERSELECTOR_H

#include "../TRACERPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>

class QComboBox;
class QPushButton;
class QVBoxLayout;
class QWidget;

class TRACERPLUGINSHARED_EXPORT CharacterSelectorNode : public PluginNodeInterface
{
    Q_OBJECT
private:
    QWidget* _widget;               //!< UI container element
    QVBoxLayout* _selectionLayout;  //!< UI layout element
    QComboBox* _selectionMenu;      //!< UI drop down menu to select the character

    std::weak_ptr<AnimNodeData<CharacterObjectSequence>> _characterListIn;  //!< The list of characters in the scene - **Data set by UI PortIn**
    std::shared_ptr<AnimNodeData<CharacterObject>> _characterOut;           //!< The selected character to which the animation is going to be applied - **Data sent via UI PortOut**

public:
    CharacterSelectorNode();
    ~CharacterSelectorNode();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<CharacterSelectorNode>(new CharacterSelectorNode()); };

    static QString Name() { return QString("CharacterSelectorNode"); }

    QString category() override { return "TRACER"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    //! Public function called by Qt Application returning number of in and out ports
    /*!
    * \param  portType (enum - 0: IN, 1: OUT, 2: NONE)
    * \return number of IN and OUT ports
    */
    unsigned int nDataPorts(QtNodes::PortType portType) const override;

    //! Public function called by Qt Application returning which datatype is associated to a specific port
    /*!
    * \param  portType (enum - 0: IN, 1: OUT, 2: NONE)
    * \param  portIndex (unsinged int with additional checks)
    * \return datatype associated to the portIndex-th IN or OUT port
    */
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    bool  isDataAvailable() override;

    void run() override;


    //! Initializes the plugin's UI elements
   /*!
   * Connects the various signals emitted by the UI elements to slots and functions of this class:
   * - \c QComboBox::currentIndexChanged signal is connected to \c CharacterSelectorPlugin::onChangedSelection
   * \returns A pointer to the UI elements' container
   */
    QWidget* embeddedWidget() override;

private Q_SLOTS:
    
    //! Slot called when the drop-down menu selection changes
    /*!
    * Overwrite _characterOut with new selected element
    * \param index Index of the selected element in the list
    */
    void onChangedSelection(int index);

};

#endif // CHARACTERSELECTOR_H