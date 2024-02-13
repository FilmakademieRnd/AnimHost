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

FolderSelectionWidget::FolderSelectionWidget(QWidget* parent)
{
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
	QString directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, "Select Directory"));
	if (!directory.isEmpty()) {
		SetDirectory(directory);
		Q_EMIT directoryChanged();
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
