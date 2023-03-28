#include "exampleplugin.h"
#include <iostream>
#include <QFileInfo>
#include <QDateTime>
#include <QImage>
#include <QBuffer>

ExamplePlugin::ExamplePlugin()
{
}

// execute the main functionality of the plugin
void ExamplePlugin::run()
{
    //execute

    //inform
    emit done();
}
