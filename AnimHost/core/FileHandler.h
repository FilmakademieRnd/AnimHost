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
