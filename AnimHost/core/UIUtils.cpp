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

 
#include "UIUtils.h"
#include "animhosthelper.h"
#include <commondatatypes.h>

#include <QPainter>
#include <QTimer>
#include <QBrush>
#include <QLinearGradient>
#include <QTime>


BoneSelectionWidget::BoneSelectionWidget(QWidget* parent ) : QWidget(parent)
{
	layout = new QHBoxLayout();
	//layout->setSizeConstraint(QLayout::SetNoConstraint);

	label = new QLabel("Root Bone",parent);

	comboBox = new QComboBox(parent);

	comboBox->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
	comboBox->view()->window()->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
	comboBox->view()->window()->setAttribute(Qt::WA_TranslucentBackground);


	layout->addWidget(label);
	layout->addWidget(comboBox);

	this->setLayout(layout);

	QObject::connect(comboBox, &QComboBox::activated, this, &BoneSelectionWidget::currentBoneChanged);
}

void BoneSelectionWidget::UpdateBoneSelection(const Skeleton& skeleton)
{
	comboBox->clear();

	for (const auto& [key, value] : skeleton.bone_names) {
		comboBox->addItem(QString(key.c_str()));
	}

	adjustSize();
	parentWidget()->adjustSize();
	updateGeometry();
}

QString BoneSelectionWidget::GetSelectedBone()
{
	return comboBox->currentText();
}

FolderSelectionWidget::FolderSelectionWidget(QWidget* parent, SelectionType selectionType, QString fileSuffix, QString fileFilter)
{
	_selectionType = selectionType;
	_fileSuffix = fileSuffix;
	_fileFilter = fileFilter;

	_filePathLayout = new QHBoxLayout();

	_label = new QLabel("Select new path");

	_pushButton = new QPushButton("Select Path");
	_pushButton->resize(QSize(30, 30));

	_filePathLayout->addWidget(_label);
	_filePathLayout->addWidget(_pushButton);

	this->setLayout(_filePathLayout);
	QObject::connect(_pushButton, &QPushButton::released, this, &FolderSelectionWidget::UpdateDirectory);

	this->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
		"QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
		"QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
		"QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
	);

}


void FolderSelectionWidget::UpdateDirectory()
{

	QFileDialog dialog(this);

	if (_selectionType == SelectionType::Directory) {
		dialog.setFileMode(QFileDialog::Directory);
	}
	else if (_selectionType == SelectionType::File) {
		dialog.setFileMode(QFileDialog::ExistingFile);
		// Set the filter to only allow .json files
		dialog.setNameFilter(_fileFilter);
		dialog.setDefaultSuffix(_fileSuffix);
	}


	QString filePath;

	if (dialog.exec())
	{
		filePath = dialog.selectedFiles().at(0);
		if (!filePath.isEmpty()) {

			// Get the application's directory
			QDir appDir(QApplication::applicationDirPath());

			// Make the path relative to the application's directory
			QString relativePath = appDir.relativeFilePath(filePath);

			SetDirectory(relativePath);
			Q_EMIT directoryChanged();
		}
	}


	
}

QString FolderSelectionWidget::GetSelectedDirectory()
{
	return _selectedDirectory;
}

void FolderSelectionWidget::SetDirectory(QString dir)
{
	if (!dir.isEmpty()) {
		_selectedDirectory = dir;
		
		QString shorty = AnimHostHelper::shortenFilePath(dir, 10);
		_label->setText(shorty);

		adjustSize();
		parentWidget()->adjustSize();
		updateGeometry();
	}
}

///!=========================
/// Matrix Editor Widget
///!=========================

MatrixEditorWidget::MatrixEditorWidget(QWidget* parent)
{
	layout = new QGridLayout();

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			lineEdits[i][j] = new QLineEdit();
			lineEdits[i][j]->setValidator(new QDoubleValidator(this));
			lineEdits[i][j]->setFixedWidth(50);
			layout->addWidget(lineEdits[i][j], i, j);
		}
	}

	this->setLayout(layout);

	this->SetMatrix(glm::mat4(1.0f));


	this->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
		"QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
		"QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
		"QLineEdit{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
	);
}

glm::mat4 MatrixEditorWidget::GetMatrix() const
{
	glm::mat4 matrix;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			matrix[i][j] = lineEdits[i][j]->text().toFloat();
		}
	}

	return matrix;
}

void MatrixEditorWidget::SetMatrix(const glm::mat4& matrix)
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			lineEdits[i][j]->setText(QString::number(matrix[i][j]));
		}
	}
}


///!=========================
/// Plot Widget
///!=========================

PlotWidget::PlotWidget(QWidget* parent) : QWidget(parent)
{
	setFixedSize(400, 300);
}

void PlotWidget::addPoint(float x, float y)
{
	points.push_back(QPointF(x, y));
	update(); // Trigger repaint

}

void PlotWidget::clearPlot()
{
	points.clear();
}

void PlotWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);

	painter.fillRect(rect(), QColor(39, 45, 45));

	// Draw the coordinate axes
	painter.setPen(QColor(163, 155, 168));
	painter.drawLine(0, height() / 2, width(), height() / 2); // X-axis
	painter.drawLine(width() / 2, 0, width() / 2, height()); // Y-axis

	// Draw the points
	painter.setPen(QColor(35, 206, 107));
	painter.setBrush(QColor(35, 206, 107));
	for (const QPointF& point : points) {
		int pixelX = static_cast<int>(width() / 2 + point.x() * scale);
		int pixelY = static_cast<int>(height() / 2 - point.y() * scale);
		painter.drawEllipse(pixelX - pointSize / 2, pixelY - pointSize / 2, pointSize, pointSize);
	}
}



///!=========================
/// Signal Light
///!=========================


SignalLightWidget::SignalLightWidget(QWidget* parent)
	: QWidget(parent), currentColor(Qt::red), alpha(255), timeElapsed(0), fadeDuration(1000)
{

	setAttribute(Qt::WA_StyledBackground, false);
	setFixedSize(30, 30);
	connect(&fadeTimer, &QTimer::timeout, this, &SignalLightWidget::updateFade);
}

void SignalLightWidget::setColor(const QColor& color)
{
	currentColor = color;
	alpha = 255; // Reset alpha when changing the color
	update();
}

void SignalLightWidget::setDefaultColor(const QColor& color)
{
	defaultColor = color;
	alpha = 255; // Reset alpha when changing the color
	update();
}

void SignalLightWidget::startFadeOut(int duration, const QColor& resetColor)
{
	fadeDuration = duration;
	defaultColor = resetColor;
	timeElapsed = 0;
	alpha = 255;
	fadeTimer.start(30); // Start a timer to update every 30ms
}

void SignalLightWidget::updateFade()
{
	timeElapsed += 30;

	if (timeElapsed >= fadeDuration) {
		fadeTimer.stop();
		alpha = 255;
		currentColor = defaultColor;
	}
	else {
		alpha = 255 - (255 * timeElapsed / fadeDuration);
	}

	update(); // Trigger a repaint
}

void SignalLightWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::NoPen);

	drawLight(painter);
}

void SignalLightWidget::drawLight(QPainter& painter)
{
	int radius = width() / 4;
	QPoint center(width() / 2, height() / 2);

	// Draw the glow effect (shine)
	QRadialGradient radialGrad(center, radius * 2);
	QColor glowColor = currentColor;
	glowColor.setAlpha(alpha / 3); // Fade glow with time
	radialGrad.setColorAt(0, glowColor);
	radialGrad.setColorAt(1, QColor(0, 0, 0, 0));
	painter.setBrush(radialGrad);
	painter.drawEllipse(center, radius * 2, radius * 2);

	// Draw the main signal light
	QColor lightColor = currentColor;
	lightColor.setAlpha(alpha); // Fade light with time
	painter.setBrush(lightColor);
	painter.setPen(Qt::NoPen);
	painter.drawEllipse(center, radius, radius);
}