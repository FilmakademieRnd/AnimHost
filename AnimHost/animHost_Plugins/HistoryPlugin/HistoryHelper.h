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

 

#ifndef HISTORYHELPERPLUGIN_H
#define HISTORYHELPERPLUGIN_H

#include "HistoryPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>

template<class T>
class HISTORYPLUGINSHARED_EXPORT RingBuffer
{
private:
    int bufferSize;
    int currentPos;

    std::vector<T> buffer;

public:
    RingBuffer(int capacity): bufferSize(capacity), currentPos(0) {

        buffer.resize(capacity, T());
    };


    void updateBuffer(T inData) {
        buffer[currentPos] = inData;
        currentPos = (currentPos + 1) % bufferSize; //Wrap around when full
    }

    std::vector<T> getPastFrames(int numFrames) {
        std::vector<T> pastFrames;

        for (int i = 0; i < numFrames; i++) {
            int index = (currentPos - i - 1 + bufferSize) % bufferSize;

            pastFrames.push_back(buffer[index]);
        }
        return pastFrames;
    }

};

#endif // HISTORYHELPERPLUGIN_H
