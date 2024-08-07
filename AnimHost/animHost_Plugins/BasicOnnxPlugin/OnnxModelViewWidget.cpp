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

 
#include "OnnxModelViewWidget.h"

OnnxModelViewWidget::OnnxModelViewWidget()
{

    
    qDebug() << "BasicOnnxPlugin getWidget";

    mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetMinimumSize);


    inputModel = new QStandardItemModel();
    outputModel = new QStandardItemModel();

    inputModel->setHorizontalHeaderLabels({ "Input", "Shape" });
    outputModel->setHorizontalHeaderLabels({ "Output", "Shape" });

    //V1
    label = new QLabel("Path");
    button = new QPushButton("Import Model");
  //  connect(button, &QPushButton::released, this, &BasicOnnxPlugin::onButtonClicked);

    //button->resize(QSize(10, 10));
    //button->resize(QSize(30, 30));
    modelPathLayout = new QHBoxLayout();

    modelPathLayout->addWidget(label);
    modelPathLayout->addWidget(button);

    //modelPathLayout->setSizeConstraint(QLayout::SetMinimumSize);


    mainLayout->addLayout(modelPathLayout);

    mainLayout->setSizeConstraint(QLayout::SetMinimumSize);

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
    //this->setSizeConstraint(QLayout::SetMinimumSize);

    this->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
        "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
        "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
        "QLabel{background-color:rgb(25, 25, 25); border-width: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px; min-width: 200px}"
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
