// Copyright 2011-2014 Johann Duscher (a.k.a. Jonny Dee). All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
//    1. Redistributions of source code must retain the above copyright notice, this list of
//       conditions and the following disclaimer.
//
//    2. Redistributions in binary form must reproduce the above copyright notice, this list
//       of conditions and the following disclaimer in the documentation and/or other materials
//       provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY JOHANN DUSCHER ''AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are those of the
// authors and should not be interpreted as representing official policies, either expressed
// or implied, of Johann Duscher.

#ifndef NZMQT_PUSHPULLWORKER_H
#define NZMQT_PUSHPULLWORKER_H

#include "common/SampleBase.hpp"

#include <nzmqt/nzmqt.hpp>

#include <QByteArray>
#include <QList>


namespace nzmqt
{

namespace samples
{

namespace pushpull
{

class Worker : public SampleBase
{
    Q_OBJECT
    typedef SampleBase super;

public:
    explicit Worker(ZMQContext& context, const QString& ventilatorAddress, const QString& sinkAddress, QObject *parent = 0)
        : super(parent)
        , ventilatorAddress_(ventilatorAddress), sinkAddress_(sinkAddress)
        , ventilator_(0), sink_(0)
    {
        sink_ = context.createSocket(ZMQSocket::TYP_PUSH, this);
        sink_->setObjectName("Worker.Socket.sink(PUSH)");

        ventilator_ = context.createSocket(ZMQSocket::TYP_PULL, this);
        ventilator_->setObjectName("Worker.Socket.ventilator(PULL)");
        connect(ventilator_, SIGNAL(messageReceived(const QList<QByteArray>&)), SLOT(receiveWorkItem(const QList<QByteArray>&)));
    }

signals:
    void workItemReceived(quint32 workload);
    void workItemResultSent();

protected:
    void startImpl()
    {
        sink_->connectTo(sinkAddress_);
        ventilator_->connectTo(ventilatorAddress_);
    }

protected slots:
    void receiveWorkItem(const QList<QByteArray>& message)
    {
        quint32 work = QString(message[0]).toUInt();
        emit workItemReceived(work);

        // Do the work ;-)
        qDebug() << "snore" << work << "msec";
        sleep(work);

        // Send results to sink.
        sink_->sendMessage("");
        emit workItemResultSent();
    }

private:
    QString ventilatorAddress_;
    QString sinkAddress_;

    ZMQSocket* ventilator_;
    ZMQSocket* sink_;
};

}

}

}

#endif // NZMQT_PUSHPULLWORKER_H
