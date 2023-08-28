#include "OnnxModelViewWidget.h"

OnnxModelViewWidget::OnnxModelViewWidget()
{

    
    qDebug() << "BasicOnnxPlugin getWidget";

    mainLayout = new QVBoxLayout();


    inputModel = new QStandardItemModel();
    outputModel = new QStandardItemModel();

    inputModel->setHorizontalHeaderLabels({ "Input", "Shape" });
    outputModel->setHorizontalHeaderLabels({ "Output", "Shape" });

    //V1
    label = new QLabel("Select Model Path");
    button = new QPushButton("Example Widget");
  //  connect(button, &QPushButton::released, this, &BasicOnnxPlugin::onButtonClicked);

    //button->resize(QSize(10, 10));
    button->setFixedSize(QSize(30, 30));
    modelPathLayout = new QHBoxLayout();

    modelPathLayout->addWidget(label);
    modelPathLayout->addWidget(button);

    mainLayout->addLayout(modelPathLayout);

    //V2
    inputView = new QTableView();

    inputView->setModel(inputModel);
    inputView->verticalHeader()->hide();
    inputView->horizontalHeader()->setStretchLastSection(true);
 
    outputView = new QTableView();

    outputView->setModel(outputModel);
    outputView->verticalHeader()->hide();
    outputView->horizontalHeader()->setStretchLastSection(true);

    inputOutputLayout = new QHBoxLayout();
    inputOutputLayout->addWidget(inputView);
    inputOutputLayout->addWidget(outputView);


    //mainLayout->addLayout(inputOutputLayout);


    this->setLayout(mainLayout);

    this->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
        "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
        "QPushButton{border: 1px solid white; border-radius: 4px; min-width:30px;}"
    );
    

}

void OnnxModelViewWidget::UpdateModelDescription(const std::vector<std::string>& rTensorNames, const std::vector<std::string>& rTensorShape, bool bIsInput)
{
    if (bIsInput) {
        inputModel->clear();
        inputModel->setHorizontalHeaderLabels({ "Output", "Shape" });


        for(int i = 0; i < rTensorNames.size(); i++ )
        {
            QList<QStandardItem*> Items;
            Items.append(new QStandardItem(rTensorNames.at(i).c_str()));
            Items.append(new QStandardItem(rTensorShape.at(i).c_str()));

            inputModel->appendRow(Items);            
        }
    }
    else {
        outputModel->clear();
        outputModel->setHorizontalHeaderLabels({ "Output", "Shape" });

        for (int i = 0; i < rTensorNames.size(); i++)
        {
            QList<QStandardItem*> Items;
            Items.append(new QStandardItem(rTensorNames.at(i).c_str()));
            Items.append(new QStandardItem(rTensorShape.at(i).c_str()));

            outputModel->appendRow(Items);
        }
    }
}
