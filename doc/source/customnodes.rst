.. _custom_nodes:

==================================
Creating a Custom Node in AnimHost
==================================

This guide will walk you through the steps required to create a custom Node for the AnimHost framework.

.. contents:: Table of Contents
   :local:

Prerequisites
===============

Before you start, ensure you have the following:

- AnimHost development environment setup.
- Familiarity with C++ and Qt framework.
- Basic understanding of the AnimHost plugin architecture.

Overview
================

In this guide, we'll create a custom node named `ToyAlphaNode` as part of the `ToyCollectionPlugin`. This node will demonstrate basic functionality and integration within AnimHost. The process includes:

1. Understanding the `PluginNodeInterface` class.
2. Implementing the custom node.
3. Integrating the node into a NodeCollection.

Step 1: Understanding the PluginNodeInterface Class
====================================================

The `PluginNodeInterface` serves as the base class for creating custom nodes within AnimHost. This interface provides several methods that must be implemented by any custom node. Nodes in AnimHost typically represent individual operations or components that can be connected together to form more complex behavior.

Here is the interface definition:

.. code-block:: cpp

    class ANIMHOSTCORESHARED_EXPORT PluginNodeInterface : public QtNodes::NodeDelegateModel {
    private:
        std::shared_ptr<AnimNodeData<RunSignal>> _runSignal = nullptr;

    public:
        PluginNodeInterface() {};
        PluginNodeInterface(const PluginNodeInterface& p) {};

        virtual std::unique_ptr<NodeDelegateModel> Init() { throw; };

        QString name() const override { return metaObject()->className(); };

        virtual QString category() { throw; };

        QString caption() const override { return this->name(); }
        bool captionVisible() const override { return true; }

        virtual unsigned int nDataPorts(QtNodes::PortType portType) const = 0;

        virtual bool hasInputRunSignal() const { return true; }
        virtual bool hasOutputRunSignal() const { return true; }

        virtual NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const = 0;

        virtual std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) = 0;

        virtual void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) = 0;

        void emitDataUpdate(QtNodes::PortIndex portIndex);

        void emitRunNextNode();

        void emitDataInvalidated(QtNodes::PortIndex portIndex);

        virtual bool isDataAvailable() = 0;

        virtual void run() = 0;

        QWidget* embeddedWidget() override { throw; };
    };


This base class requires you to implement several methods, including:
- **Init()**: Initializes a new node and returns a unique pointer to the node.
- **name()**: Returns the name of the node. **Notice:** implementing the static method **Name()** is recommended. Prevents inplace construction of the node to access a name, on plugin registration.
- **category()**: Returns the category of the node.
- **caption()**: Returns the caption displayed on the node.
- **save()**: Serializes the node's data to a `QJsonObject`.
- **load()**: Deserializes the node's data from a `QJsonObject`.
- **nDataPorts()**: Returns the number of ports for input or output.
- **dataPortType()**: Specifies the data type for a given port.
- **processOutData()**: Provides the output data for a given port.
- **processInData()**: Accepts input data for a given port.
- **embeddedWidget()**: Returns a widget to be embedded in the node, if any.

These methods provide the core functionality of the node, enabling it to interact with other nodes in the AnimHost environment.

Step 2: Implementing the Custom Node
====================================

To implement a custom node like `ToyAlphaNode`, you need to extend the `PluginNodeInterface` and provide concrete implementations for the abstract methods. Below is an example implementation using the provided files.

**ToyAlphaNode.h**

.. code-block:: cpp

    #ifndef TOYALPHANODE_H
    #define TOYALPHANODE_H

    #include "pluginnodeinterface.h"
    #include <QTimer>

    class TOYCOLLECTIONPLUGINSHARED_EXPORT ToyAlphaNode : public PluginNodeInterface {
        Q_OBJECT

    private:
        QWidget* _widget;
        std::weak_ptr<AnimNodeData<Animation>> _animationIn;
        std::shared_ptr<AnimNodeData<Animation>> _animationOut;

    public:
        ToyAlphaNode(const QTimer& tick);
        ~ToyAlphaNode();

        std::unique_ptr<NodeDelegateModel> Init() override { return nullptr; };
        QString caption() const override { return this->name(); }
        bool captionVisible() const override { return true; }
        static QString Name() { return QString("ToyAlphaNode"); }
        unsigned int nDataPorts(QtNodes::PortType portType) const override;
        NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
        std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
        void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
        bool isDataAvailable() override;
        void run() override;
        QWidget* embeddedWidget() override;
        QString category() override { return "Operator"; };
    };

    #endif // TOYALPHANODE_H

**ToyAlphaNode.cpp**

.. code-block:: cpp

    #include "ToyAlphaNode.h"
    #include <QJsonObject>
    #include <QJsonArray>
    #include <QJsonValue>
    #include <QDebug>

    ToyAlphaNode::ToyAlphaNode(const QTimer& tick)
    {
        /*
        * Initialize all neccessary variables, especially the output data of the node.
        * Leave to widget initialization to embeddedWidget().
        */

        _widget = nullptr;
        _animationOut = std::make_shared<AnimNodeData<Animation>>();
        connect(&tick, &QTimer::timeout, this, &ToyAlphaNode::run);

        qDebug() << "ToyAlphaNode created";
    }

    ToyAlphaNode::~ToyAlphaNode()
    {
        qDebug() << "~ToyAlphaNode()";
    }

    unsigned int ToyAlphaNode::nDataPorts(QtNodes::PortType portType) const
    {
        /*
        * Return the number of data ports for the given port type. In or Out.
        * The number of ports can be dynamic, but must match the dataPortType() function.
        */
        if (portType == QtNodes::PortType::In)
            return 1;
        else
            return 1;
    }

    NodeDataType ToyAlphaNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
    {
        /*
        * Return the data port type for the given port type and port index.
        * This function must match the nDataPorts() function.
        */
        NodeDataType type;
        if (portType == QtNodes::PortType::In)
            return AnimNodeData<Animation>::staticType();
        else
            return AnimNodeData<Animation>::staticType();
    }



    void ToyAlphaNode::run()
    {
        /*
        * Run the main node logic here. run() is called through the incoming run signal of another node.
        * run() can also be called through another signal, like a button press or in our case a timer.
        * But it is recommended to keep user interaction to a minimum. 
        */
        qDebug() << "ToyAlphaNode run";
    }

    std::shared_ptr<NodeData> ToyAlphaNode::processOutData(QtNodes::PortIndex port)
    {
        /*
        * return processed data based on the rquested output port. Returned data type must match the dataPortType() function.
        */
        return _animationOut;
    }

    void ToyAlphaNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
    {
        /*
        * Check if the incoming data is valid and cast it to the correct type based on the previously defined types in dataPortType() function.
        * If the data is valid, store it in a member variable for further processing.
        * If data is invalid, emit dataInvalidated() to notify the downstream nodes.
        * We recommend handling incoming data as weak_ptr.
        */

        if (!data) {
            Q_EMIT dataInvalidated(0);
        }
        _animationIn = std::static_pointer_cast<AnimNodeData<Animation>>(data);
    

        qDebug() << "ToyAlphaNode setInData";
    }

    bool ToyAlphaNode::isDataAvailable() {
        /*
        * Use this function to check if the inbound data is available and can be processed.
        */
        return !_animationIn.expired();
    }

    QWidget* ToyAlphaNode::embeddedWidget()
    {
        /*
        * Return the embedded widget of the node. This can be a QWidget or nullptr if no widget is embedded.
        * To propagate dynamic changes to the widget size, call Q_EMIT embeddedWidgetSizeUpdated() whenever the widget size changes.
        */

        return nullptr;
    }
    

Step 3: Integrating the Node into a NodeCollection
===================================================

Once you have implemented your custom node, the next step is to integrate it into a NodeCollection, such as the `ToyCollectionPlugin`. This was covered in the `RegisterNodeCollection` method within the NodeCollection plugin:

.. code-block:: cpp

    void ToyCollectionPlugin::RegisterNodeCollection(NodeDelegateModelRegistry& nodeRegistry) override {

        nodeRegistry.registerModel<ToyAlphaNode>([this]() { return std::make_unique<ToyAlphaNode>(*localTick); });

    }

This registration process ensures that your custom node is recognized by AnimHost and can be used within the node editor environment.

Conclusion
==========

By following this guide, you should now be able to create and integrate a custom node into AnimHost. Customize and expand on this example to build more complex nodes that meet your specific requirements.

For further assistance, refer to the AnimHost GitHub repository for more detailed examples of already existing nodes and community support.

