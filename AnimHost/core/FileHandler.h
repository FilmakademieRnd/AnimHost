#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "animhostcore_global.h"

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>


template <typename StreamType>
class FileHandler {
private:
    QFile file;
    StreamType stream;

public:
    FileHandler(const QString& filePath, bool isText = false) : file(filePath){

        if (file.exists()) {
            // If the file exists, open for appending
            if (file.open(QIODevice::Append | (isText ? QIODevice::Text : QIODevice::NotOpen))) {
                stream.setDevice(&file);
            }
            else {
                qDebug() << "Error opening file for append.";
            }
        }
        else {
            // If the file doesn't exist, create and open for writing
            if (file.open(QIODevice::WriteOnly | (isText ? QIODevice::Text : QIODevice::NotOpen))) {
                stream.setDevice(&file);
            }
            else {
                qDebug() << "Error creating file and writing.";
            }
        }
    }

    StreamType& getStream() {
        return stream;
    }

    static bool deleteFile(const QString& filePath) {
        QFile delFile(filePath);
        return delFile.remove();
    }

    ~FileHandler() {
        file.close();
    }
};

#endif  // FILEHANDLER_H
