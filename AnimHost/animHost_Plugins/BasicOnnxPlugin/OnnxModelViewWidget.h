
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