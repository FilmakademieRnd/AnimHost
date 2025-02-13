.. _custom_node_collection_plugin: 

==============================
Creating a Custom NodeCollection Plugin
==============================

This guide will walk you through the steps required to create a custom NodeCollection plugin for the AnimHost framework.

:ref:`custom_nodes`

.. contents:: Table of Contents
   :local:

Prerequisites
================

Before you start, ensure you have the following:

- Familiarity with C++ and Qt framework.
- AnimHost development environment setup.
- Basic understanding of the AnimHost plugin architecture.


Overview
================

In this guide, we'll create a custom NodeCollection plugin named `ToyCollectionPlugin` 
that includes two custom nodes `ToyAlphaNode` and `ToyBetaNode`. As the name suggests, this collection will contain toy nodes that demonstrate basic functionality.
They don't have any real-world application but are useful for understanding the creation of custom nodes and their integration into AnimHost.
The process includes:

1. Purpose of the :class:`PluginNodeCollectionInterface`.
2. Implementing the custom NodeCollection plugin.
3. Implementing the custom nodes.
4. Registering the plugin nodes.
5. Packaging the plugin for use in AnimHost.

Step 1: Define the PluginNodeCollectionInterface
================================================

The `PluginNodeCollectionInterface` serves as the base class for creating NodeCollection plugins. 
This interface provides several methods that must be implemented by any custom plugin. 
The node collection serves as a container for multiple nodes that can be used within AnimHost.
This allows you to group related nodes together and provide a cohesive set of functionality. 
In addition to the nodes themselves, the collection allows you to define pre and post-registration steps,
metadata, and other information about the collection.
This might be useful for nodes that require specific initialization, cleanup steps or access to shared resources.
Shared resources could be a timer, a database connection, or any other resource that needs to be shared among multiple nodes.
These resources can be initialized inside the collection and passed to the nodes during their creation through dependency injection.

Here's a breakdown of the interface:

.. code-block:: cpp

   //! \brief Interface for plugins for the AnimHost
   class ANIMHOSTCORESHARED_EXPORT PluginNodeCollectionInterface : public QObject
   {
   protected:
       struct CollectionMetaData
       {
           QString name = "";
           QString description = "";
           QString version = "";
           QString author = "";
           QString license = "";
       };

       static CollectionMetaData _collectionMetaData;

   public:
       PluginNodeCollectionInterface() {};
       virtual ~PluginNodeCollectionInterface() {};

       virtual void PreNodeCollectionRegistration() = 0;
       virtual void RegisterNodeCollection(NodeDelegateModelRegistry& nodeRegistry) = 0;
       virtual void PostNodeCollectionRegistration() = 0;
       virtual QString GetNodeCollectionName() = 0;
       virtual QString GetNodeCollectionDescription() = 0;
       virtual QString GetNodeCollectionMetaData() = 0;
   };

Step 2: Implement the Custom NodeCollection Plugin
===================================================

Next, implement your custom plugin by extending the `PluginNodeCollectionInterface`. 
We provide a helper script to generate the necessary boilerplate code for the plugin.
It can be found in the `AnimHost/animHost_Plugins` directory. Run the `NewPluginGenerator.py` to create a new plugin.

Here's our example implementation:

.. code-block:: cpp

   class TOYCOLLECTIONPLUGINSHARED_EXPORT ToyCollectionPlugin : public PluginNodeCollectionInterface
   {
       Q_OBJECT
       Q_PLUGIN_METADATA(IID "de.animhost.PluginNodeCollectionInterface" FILE "ToyCollectionPlugin.json")
       Q_INTERFACES(PluginNodeCollectionInterface)

       CollectionMetaData _collectionMetaData = {
           "ToyCollection",
           "A collection of toy nodes",
           "1.0",
           "Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs",
           "MIT"
       };

       QTimer* localTick = nullptr;

   public:
       ToyCollectionPlugin() { qDebug() << "ToyCollectionPlugin created"; };
       ~ToyCollectionPlugin() { qDebug() << "~ToyCollectionPlugin()"; };

       void PreNodeCollectionRegistration() override {
           localTick = new QTimer();
           localTick->setTimerType(Qt::PreciseTimer);
           localTick->setSingleShot(false);
           localTick->setInterval(2000);
           localTick->start();
       };

       void RegisterNodeCollection(NodeDelegateModelRegistry& nodeRegistry) override {
           nodeRegistry.registerModel<ToyAlphaNode>([this](){ return  std::make_unique<ToyAlphaNode>(*localTick); });
       };

       QString GetNodeCollectionName() override {
           return _collectionMetaData.name;
       };

       QString GetNodeCollectionDescription() override {
           return _collectionMetaData.description;
       };

       QString GetNodeCollectionMetaData() override {
           return _collectionMetaData.version;
       };
   };

Besides the `PluginNodeCollectionInterface` methods, you can add any additional functionality to the plugin, 
such as shared resources or initialization steps. 
Our example plugin includes a `QTimer` object that is used to provide a shared timer resource to the nodes.

Step 3: Implement the Custom Nodes
===================================
Next, implement the custom nodes that will be part of the collection. 
Follow this example to create a custom node:
:ref:`custom_nodes`


Step 4: Register the Plugin Nodes
=================================

In the `RegisterNodeCollection` method, you register each node that is part of your collection. For example:

.. code-block:: cpp

   void RegisterNodeCollection(NodeDelegateModelRegistry& nodeRegistry) override {
       nodeRegistry.registerModel<ToyAlphaNode>([this](){ return  std::make_unique<ToyAlphaNode>(*localTick); });
   };

Each node in the collection should be a subclass of `PluginNodeInterface`, tailored to your specific requirements.

Step 5: Packaging the Plugin
============================

To make the plugin usable within AnimHost, you need to compile it and ensure that it is placed in the correct directory where AnimHost loads plugins.


Testing the Plugin
==================

Once the plugin is compiled and installed, you can load it into AnimHost to test its functionality.
Ensure that all nodes behave as expected and that the plugin is registered correctly.

Conclusion
==========

By following this guide, you should now be able to create and integrate a custom NodeCollection plugin into AnimHost. Customize and expand on this example to fit your specific needs.

For further assistance, refer to the AnimHost GitHub repository for more detailed examples of already existing plugins and community support.

