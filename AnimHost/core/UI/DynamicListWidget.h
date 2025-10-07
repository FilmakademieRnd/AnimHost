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

 
#ifndef ANIMHOST_DYNAMICLIST_H
#define ANIMHOST_DYNAMICLIST_H

#include <commondatatypes.h>
#include <QtWidgets>
#include <QObject>
#include <QWidget>
#include <QPainter>
#include <QVector>
#include <cmath>


class ANIMHOSTCORESHARED_EXPORT MetaDataListItem : public QWidget {

	Q_OBJECT

	QGridLayout* layout = nullptr;

	QSpinBox* sBoxParameterID = nullptr;

	QComboBox* cBoxNodes = nullptr;
	QComboBox* cBoxProperties = nullptr;
	QCheckBox* triggerRun = nullptr;

	const QList<QPair<QString, QStringList>>* nodeElements = nullptr;

public:
	MetaDataListItem(QWidget* parent = nullptr);
	MetaDataListItem(int id, const QList<QPair<QString, QStringList>>& nodeElements, int trigger, QWidget* parent = nullptr);
	~MetaDataListItem() {};

	void SetupMetaDataListItem(int id, const QList<QPair<QString, QStringList>>& nodeElements, int trigger);

	void SetParameterID(int id);
	void SetNodes(const QStringList& nodes);
	void SetProperties(const QStringList& properties);
	void SetTriggerRun(bool trigger);

	void notifyDataChanged();

	QSize sizeHint() const override { return layout->sizeHint(); }

private Q_SLOT:

	void OnParameterIDChanged(int id);
	void OnNodeChanged(QString node);
	void OnPropertyChanged(QString property);
	void OnTriggerRunChanged(bool trigger);


Q_SIGNALS:

	void MappingChanged(int paramId, QString node, QString property, int trigger);

};

class ANIMHOSTCORESHARED_EXPORT DynamicListWidget : public QWidget {

	Q_OBJECT

private:
	QVBoxLayout* layout = nullptr;
	QListWidget* listWidget = nullptr;
	QPushButton* addButton = nullptr;
	QPushButton* removeButton = nullptr;


	QList<QPair<QString, QStringList>> elements;

public:
	DynamicListWidget(QWidget* parent = nullptr);
	~DynamicListWidget() {};

	void SetModifyableElements(QList<QPair<QString, QStringList>> elements);
	
private Q_SLOTS:

	void AddItem();
	void RemoveItem();

	void onItemMappingChanged(int paramId, QString node, QString property, int trigger);

Q_SIGNALS:
	void elementAdded();
	void elementRemoved(int index);

	void ItemMappingChanged(int itemIdx, int paramId, QString node, QString property, int trigger);


};
#endif // ANIMHOST_DYNAMICLIST_H

