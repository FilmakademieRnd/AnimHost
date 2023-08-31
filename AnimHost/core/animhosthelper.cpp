#include "animhosthelper.h"

#include <QFileInfo>
#include <QDir>

QString AnimHostHelper::shortenFilePath(const QString& filePath, int maxLength)
{
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    QString path = fileInfo.path();

    if (filePath.length() <= maxLength) {
        return filePath;
    }
    else {
        int ellipsisLen = 3;
        QString shortPath = "..."+path.right(maxLength - ellipsisLen);
        
        return  shortPath + QDir::separator() + fileName;
    }
    
    
    return QString();
}
