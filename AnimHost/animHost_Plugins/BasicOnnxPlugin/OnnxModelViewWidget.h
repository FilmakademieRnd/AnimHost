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

 

#ifndef ONNXMODELVIEWWIDGET_H
#define ONNXMODELVIEWWIDGET_H

#include <onnxruntime_cxx_api.h>
#include <QtWidgets>


class OnnxModelViewWidget : public QWidget {

    Q_OBJECT

public:

    OnnxModelViewWidget();
    ~OnnxModelViewWidget() {};

    QPushButton* GetButton() { return button; };

    QLabel* label;

    void UpdateModelDescription(const std::vector<std::string>& rTensorNames, const std::vector<std::string>& rTensorShape, bool BIsInput = true);

private:
	//GUI
    QWidget* widget;

    QVBoxLayout* mainLayout; 
    //V1
    //QLabel* label;
    QPushButton* button;
    QHBoxLayout* modelPathLayout;

    //V2
    QHBoxLayout* inputOutputLayout;
    QTableView* inputView;
    QTableView* outputView;

    //In & Out Data Model
    QStandardItemModel* inputModel;
    QStandardItemModel* outputModel;



	
};

#endif // BASICONNXPLUGINPLUGIN_H