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

FolderSelectionWidget::FolderSelectionWidget(QWidget* parent, SelectionType selectionType)
{
	_selectionType = selectionType;

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
	}


	QString filePath;

	if (dialog.exec())
	{
		filePath = dialog.selectedFiles().at(0);
		if (!filePath.isEmpty()) {
			SetDirectory(filePath);
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


