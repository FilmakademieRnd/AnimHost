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

class ANIMHOSTCORESHARED_EXPORT MatrixEditorWidget : public QWidget {

	Q_OBJECT



private:

	QGridLayout* layout = nullptr;
	QLabel* header = nullptr;
	QLineEdit* lineEdits[4][4];

public:
	MatrixEditorWidget(QWidget* parent = nullptr);
	~MatrixEditorWidget() {};

	glm::mat4 GetMatrix() const;

	void SetMatrix(const glm::mat4& matrix);



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

class ANIMHOSTCORESHARED_EXPORT SignalLightWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SignalLightWidget(QWidget* parent = nullptr);
	void setColor(const QColor& color);
	void setDefaultColor(const QColor& color);
	void startFadeOut(int duration = 1000, const QColor& defaulColor= QColor(50, 255, 50)); // duration in ms

protected:
	void paintEvent(QPaintEvent* event) override;

private slots:
	void updateFade();

private:
	QColor currentColor;
	QColor defaultColor;
	int alpha; // To manage the fade effect
	QTimer fadeTimer;
	int fadeDuration;
	int timeElapsed;

	void drawLight(QPainter& painter);
};


#endif // ANIMHOST_UI_UTIL_H