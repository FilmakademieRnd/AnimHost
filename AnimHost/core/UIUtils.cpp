#include "UIUtils.h"
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

	QObject::connect(comboBox, &QComboBox::currentTextChanged, this, &BoneSelectionWidget::currentBoneChanged);
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


