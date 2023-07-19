#include <QCoreApplication>
#include <iostream>
#include <animhostcore.h>

#include <QtNodes/ConnectionStyle>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeData>
#include <QtNodes/NodeDelegateModelRegistry>

#include <QtGui/QScreen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QVBoxLayout>

#include <QtGui/QScreen>

using QtNodes::ConnectionStyle;
using QtNodes::DataFlowGraphicsScene;
using QtNodes::DataFlowGraphModel;
using QtNodes::GraphicsView;
using QtNodes::NodeDelegateModelRegistry;

//!
//! \brief set the UI style for the nodes
//!
static void setStyle()
{
    ConnectionStyle::setConnectionStyle(
        R"(
  {
    "ConnectionStyle": {
      "ConstructionColor": "gray",
      "NormalColor": "black",
      "SelectedColor": "gray",
      "SelectedHaloColor": "deepskyblue",
      "HoveredColor": "deepskyblue",

      "LineWidth": 3.0,
      "ConstructionLineWidth": 2.0,
      "PointDiameter": 10.0,

      "UseDataDefinedColors": true
    }
  }
  )");
}

//!
//! \brief main function being the entry point of the app
//! \param argc commandline arguments given
//! \param argv commandline arguments given
//! \return
//!
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //grab command line arguments
    QStringList cmdlineArgs = QCoreApplication::arguments();

    //handle faulty command line arguments
    /*if(cmdlineArgs.length() != 2)
    {
        std::cout << "Wrong amount of command line arguments! Aborting..." << std::endl;
        return a.exec();
    }*/
    //run animHost


    AnimHost* animHost = new AnimHost();


    //QTNodes
    setStyle();

    //TODO replace!
    std::shared_ptr<NodeDelegateModelRegistry> registry = animHost->nodes;

    QWidget mainWidget;

    QMenuBar* menuBar = new QMenuBar();
    QMenu *menu = menuBar->addMenu("File");
    QAction* saveAction = menu->addAction("Save Scene");
    QAction* loadAction = menu->addAction("Load Scene");

    QVBoxLayout *l = new QVBoxLayout(&mainWidget);

    DataFlowGraphModel dataFlowGraphModel(registry);

    l->addWidget(menuBar);
    auto scene = new DataFlowGraphicsScene(dataFlowGraphModel, &mainWidget);

    auto view = new GraphicsView(scene);
    l->addWidget(view);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    QObject::connect(saveAction, &QAction::triggered, scene, &DataFlowGraphicsScene::save);
    QObject::connect(loadAction, &QAction::triggered, scene, &DataFlowGraphicsScene::load);
    QObject::connect(scene, &DataFlowGraphicsScene::sceneLoaded, view, &GraphicsView::centerScene);

    mainWidget.setWindowTitle("AnimHost");
    mainWidget.resize(800, 600);

    // Center window.
    mainWidget.move(QApplication::primaryScreen()->availableGeometry().center()
                    - mainWidget.rect().center());
    mainWidget.showNormal();

    return a.exec();
}
