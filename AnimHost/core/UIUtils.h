#ifndef ANIMHOST_UI_UTIL_H
#define ANIMHOST_UI_UTIL_H

#include <commondatatypes.h>
#include <QtWidgets>
#include <QObject>

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
	void currentBoneChanged(const QString& text);

};
#endif // ANIMHOST_UI_UTIL_H