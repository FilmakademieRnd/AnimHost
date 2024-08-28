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

 
#ifndef PLUGINNODEINTERFACE_H
#define PLUGINNODEINTERFACE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QVariant>
#include "plugininterface.h"
#include <QtNodes/NodeDelegateModel>
#include <nodedatatypes.h>

//!
//! \brief Interface for plugins for the AnimHost
//!
class ANIMHOSTCORESHARED_EXPORT PluginNodeInterface : public QtNodes::NodeDelegateModel
{

private:
    std::shared_ptr<AnimNodeData<RunSignal>> _runSignal = nullptr;

public:

    PluginNodeInterface() {};
    PluginNodeInterface(const PluginNodeInterface& p) {};

    virtual std::unique_ptr<NodeDelegateModel>  Init() { throw; };

    QString name() const override { return metaObject()->className(); };
    virtual QString category() { throw; }; 

    
    /**
     * Return the caption of the node.
     * The caption is rendered on the node header in the GUI.
     * Can be turned off by overriding captionVisible() to false.
     * \return 
     */
    QString caption() const override { return this->name(); }

    /**
	 * Return if the caption should be visible in the GUI.
	 * \return 
	 */
    bool captionVisible() const override { return true; }

    unsigned int nPorts(QtNodes::PortType portType) const override;

    /**
     * Return the number of data ports in- and output of the node. 
     * 
     * \param portType Selector of in- or output type
     * \return 
     */
    virtual unsigned int nDataPorts(QtNodes::PortType portType) const = 0;

    /**
     * Return if the node has a run signal as input.
     * 
     * \return 
     */
    virtual bool hasInputRunSignal() const { return true; }

    /**
	 * Return if the node has a run signal as output.
	 * 
	 * \return 
	 */
    virtual bool hasOutputRunSignal() const { return true; }


    NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    
    /**
     * Return the data type of data in - or output on given port..
     * @param[in] portIndex = 0 is reserved for run signal and handeled before dataPortType is called.
     * 
     * \return
     */
    virtual NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const = 0;

    std::shared_ptr<NodeData> outData(QtNodes::PortIndex port) override;

    /**
	 * Return the data of the requested output port.
	 * 
	 * \return 
	 */
    virtual std::shared_ptr<NodeData>  processOutData(QtNodes::PortIndex port) = 0;

     
    void setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex);

    /**
    * Process the data of the input port.
    */
    virtual void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) = 0;

    /**
     * Emit this whenever the data of the specific node output has been updated.
     * 
     * \param portIndex
     */
    void emitDataUpdate(QtNodes::PortIndex portIndex);

    /**
	 * Emit this whenever the node has finished processing and is ready to run the next node.
	 */
    void emitRunNextNode();

    /**
    * Emit this whenever the data of the specific node output has been invalidated.
    * 
    * \param portIndex
    */
    void emitDataInvalidated(QtNodes::PortIndex portIndex);

    /**
    * Check if the inbound data is available.
    * \returns true if data is available and can be processed.
    */
    virtual bool isDataAvailable() = 0;

    /**
    * Main function to run the node. Triggered by the run signal of the previous node.
	*/
    virtual void run() = 0;


    /**
    * Return the embedded widget of the node.
    * \return QWidget* or nullptr if no widget is embedded.
	*/
    QWidget* embeddedWidget() override { throw; };

};

#define PluginNodeInterface_iid "de.animhost.PluginNodeInterface"

Q_DECLARE_INTERFACE(PluginNodeInterface, PluginNodeInterface_iid)

#endif // PLUGINNODEINTERFACE_H
