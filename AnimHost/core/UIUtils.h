#ifndef ANIMHOST_UI_UTIL_H
#define ANIMHOST_UI_UTIL_H

#include <commondatatypes.h>
#include <QtWidgets>
#include <QObject>
#include <QWidget>
#include <QPainter>
#include <QVector>
#include <cmath>


class ANIMHOSTCORESHARED_EXPORT BoneSelectionWidget : public QWidget {

	Q_OBJECT

private: 

	QHBoxLayout* layout = nullptr;
	QLabel* label = nullptr;
	QComboBox* comboBox = nullptr;

public:
	BoneSelectionWidget(QWidget* parent = nullptr);
	~BoneSelectionWidget() {};

	void UpdateBoneSelection(const Skeleton& skeleton);

	QString GetSelectedBone();

Q_SIGNALS:
	void currentBoneChanged(const int text);

};


class ANIMHOSTCORESHARED_EXPORT FolderSelectionWidget : public QWidget {

	Q_OBJECT

public:
	enum SelectionType {
		Directory,
		File
	};

private:
	QString _selectedDirectory = "";

	SelectionType _selectionType = SelectionType::Directory;

	QHBoxLayout* _filePathLayout = nullptr;
	QLabel* _label = nullptr;
	QPushButton* _pushButton = nullptr;

public:
	FolderSelectionWidget(QWidget* parent = nullptr, SelectionType selectionType = SelectionType::Directory);
	~FolderSelectionWidget() {};

	void UpdateDirectory();

	QString GetSelectedDirectory();
	void SetDirectory(QString dir);

Q_SIGNALS:
	void directoryChanged();


};

class ANIMHOSTCORESHARED_EXPORT PlotWidget : public QWidget {

	Q_OBJECT
private:

	QVector<QPointF> points;
	int pointSize = 8;
	double scale = 20.0;

public:

	PlotWidget(QWidget* parent = nullptr);

	void addPoint(float x, float y);

	void clearPlot();

protected:
	void paintEvent(QPaintEvent* event) override;

Q_SIGNALS:
	void pointsChanged();





};


#endif // ANIMHOST_UI_UTIL_H