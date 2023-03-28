#include <QCoreApplication>
#include <iostream>
#include <animhost.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //grab command line arguments
    QStringList cmdlineArgs = QCoreApplication::arguments();

    //handle faulty command line arguments
    if(cmdlineArgs.length() != 2)
    {
        std::cout << "Wrong amount of command line arguments! Aborting..." << std::endl;
        return a.exec();
    }

    //interprete command line arguments
    QString filePath = cmdlineArgs[1];

    //run animHost
    AnimHost* animHost = new AnimHost(filePath);
    animHost->run();

    return a.exec();
}
