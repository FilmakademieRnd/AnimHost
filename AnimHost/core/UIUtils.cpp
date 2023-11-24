#include "UIUtils.h"
#include <commondatatypes.h>

BoneSelectionWidget::BoneSelectionWidget(QWidget* parent) : QWidget(parent)
{
	layout = new QHBoxLayout();

	label = new QLabel("Root Bone");

	comboBox = new QComboBox();

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
}

QString BoneSelectionWidget::GetSelectedBone()
{
	return comboBox->currentText();
}


