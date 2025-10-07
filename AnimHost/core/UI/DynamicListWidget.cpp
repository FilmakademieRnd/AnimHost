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
#include "DynamicListWidget.h"


MetaDataListItem::MetaDataListItem(QWidget* parent) : QWidget(parent)
{
	layout = new QGridLayout();
	layout->setSpacing(5);
	layout->setContentsMargins(0, 0, 0, 0);

	sBoxParameterID = new QSpinBox(parent);
	sBoxParameterID->setMinimum(0);
	sBoxParameterID->setMaximum(255);
	sBoxParameterID->setSingleStep(1);
	sBoxParameterID->setAlignment(Qt::AlignCenter);

	cBoxNodes = new QComboBox(parent);
	cBoxNodes->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
	cBoxNodes->view()->window()->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
	cBoxNodes->view()->window()->setAttribute(Qt::WA_TranslucentBackground);

	cBoxProperties = new QComboBox(parent);
	cBoxProperties->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
	cBoxProperties->view()->window()->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
	cBoxProperties->view()->window()->setAttribute(Qt::WA_TranslucentBackground);

	triggerRun = new QCheckBox(parent);

	layout->addWidget(sBoxParameterID, 0, 0);
	layout->addWidget(cBoxNodes, 0, 1);
	layout->addWidget(cBoxProperties, 0, 2);
	layout->addWidget(triggerRun, 0, 3);

	this->setLayout(layout);

	QObject::connect(sBoxParameterID, &QSpinBox::valueChanged, this, &MetaDataListItem::OnParameterIDChanged);
	QObject::connect(cBoxNodes, &QComboBox::currentTextChanged, this, &MetaDataListItem::OnNodeChanged);
	QObject::connect(cBoxProperties, &QComboBox::currentTextChanged, this, &MetaDataListItem::OnPropertyChanged);
	QObject::connect(triggerRun, &QCheckBox::stateChanged, this, &MetaDataListItem::OnTriggerRunChanged);

}

MetaDataListItem::MetaDataListItem(int id, const QList<QPair<QString, QStringList>>& nodeElements, int trigger, QWidget* parent) : MetaDataListItem(parent)
{
	/*sBoxParameterID->setValue(id);

	QStringList properties;
	
	this->nodeElements = &nodeElements;

	for (const auto& [node, properties] : nodeElements) {

		qDebug() << "Node: " << node << " contains " << properties.size() << " properties";

		cBoxNodes->addItem(node);
	}

	triggerRun->setChecked(trigger);*/
	

	qDebug() << this->sizeHint();
}

void MetaDataListItem::SetupMetaDataListItem(int id, const QList<QPair<QString, QStringList>>& nodeElements, int trigger)
{
	sBoxParameterID->setValue(id);

	QStringList properties;

	this->nodeElements = &nodeElements;

	for (const auto& [node, properties] : nodeElements) {

		qDebug() << "Node: " << node << " contains " << properties.size() << " properties";

		cBoxNodes->addItem(node);
	}

	triggerRun->setChecked(trigger);


	qDebug() << this->sizeHint();
}

void MetaDataListItem::notifyDataChanged()
{

	Q_EMIT MappingChanged(sBoxParameterID->value(), cBoxNodes->currentText(), cBoxProperties->currentText(), triggerRun->isChecked());
}

void MetaDataListItem::OnParameterIDChanged(int id) {
	notifyDataChanged();
}

void MetaDataListItem::OnNodeChanged(QString node) {

	for (const auto& [nodeElem, properties] : *nodeElements) {
		if (nodeElem == node) {
			cBoxProperties->clear();
			cBoxProperties->addItems(properties);
		}
	}

	notifyDataChanged();
}

void MetaDataListItem::OnPropertyChanged(QString property) {



	notifyDataChanged();
}
void MetaDataListItem::OnTriggerRunChanged(bool trigger) {
	notifyDataChanged();
}



DynamicListWidget::DynamicListWidget(QWidget* parent) : QWidget(parent)
{
	layout = new QVBoxLayout();
	layout->setSpacing(5);
	layout->setContentsMargins(0, 0, 0, 0);

	listWidget = new QListWidget(parent);
	listWidget->setStyleSheet("border: 1px solid red;");
	//listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Disable horizontal scrollbars
	listWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents); // Adjust size based on content
	//listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	//listWidget->setResizeMode(QListView::Adjust);
	layout->addWidget(listWidget);

	addButton = new QPushButton("Add Parameter", parent);
	layout->addWidget(addButton);

	removeButton = new QPushButton("Remove Parameter", parent);
	layout->addWidget(removeButton);

	QObject::connect(addButton, &QPushButton::clicked, this, &DynamicListWidget::AddItem);
	QObject::connect(removeButton, &QPushButton::clicked, this, &DynamicListWidget::RemoveItem);

	this->setLayout(layout);
}

void DynamicListWidget::SetModifyableElements(QList<QPair<QString, QStringList>> elements)
{
	this->elements = elements;
}



void DynamicListWidget::RemoveItem()
{	
	int rowIndex = listWidget->currentRow();
	if (rowIndex < 0) {
		return;
	}
	else {
		auto item = listWidget->takeItem(rowIndex);
		delete item;

		Q_EMIT elementRemoved(rowIndex);
	}

}

void DynamicListWidget::onItemMappingChanged(int paramId, QString node, QString property, int trigger)
{
	MetaDataListItem* itemWidget = qobject_cast<MetaDataListItem*>(sender());

	int row = -1;

	if(itemWidget) {
		// Iterate through all items to find the one that contains the widget
		for (int i = 0; i < listWidget->count(); ++i) {
			if (listWidget->itemWidget(listWidget->item(i)) == itemWidget) {
				row =  listWidget->row(listWidget->item(i));
			}
		}

		qDebug() << "Item Mapping Changed in row" << row;
		Q_EMIT ItemMappingChanged(row, paramId, node, property, trigger);
	}

	
}

void DynamicListWidget::AddItem()
{
	auto item = new QListWidgetItem();

	this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred); // For DynamicListWidget
	listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // F

	MetaDataListItem* widget = new MetaDataListItem(this);

	item->setSizeHint(widget->sizeHint());

	listWidget->addItem(item);
	listWidget->setItemWidget(item, widget);

	Q_EMIT elementAdded();

	connect(widget, &MetaDataListItem::MappingChanged, this, &DynamicListWidget::onItemMappingChanged);

	widget->SetupMetaDataListItem(0, elements, 0);

	//qDebug() << "This" << this->sizeHint();
	//qDebug() << "listWidget size:" << listWidget->size();
	//qDebug() << "MetaDataListItem size:" << widget->sizeHint();
	//qDebug() << "listWidget viewport size:" << listWidget->viewport()->size();
	//qDebug() << "listWidget size:" << listWidget->size();

	adjustSize();

	if (auto parent = parentWidget()) {
		if (auto layout = parent->layout()) {
			layout->invalidate();
		}
		parent->adjustSize();
	}

	qDebug() << "This" << this->sizeHint();


}
