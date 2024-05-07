
#ifndef GNNPLUGINPLUGIN_H
#define GNNPLUGINPLUGIN_H

#include "GNNPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>

template<class T>
class GNNPLUGINSHARED_EXPORT HistoryBuffer
{
private:
    int bufferSize;
    int currentPos;

    std::vector<T> buffer;

public:
    HistoryBuffer(int capacity): bufferSize(capacity), currentPos(0) {

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

#endif // GNNPLUGINPLUGIN_H
