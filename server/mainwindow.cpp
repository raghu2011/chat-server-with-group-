#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QNetworkInterface>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    udpSocket = new QUdpSocket(this);

    if (udpSocket->bind(QHostAddress::AnyIPv4, 12345)) {
        ui->textEdit->append("Server successfully initialized on UDP port 12345.");
        udpSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);
    }else {
        ui->textEdit->append("Error: Failed to bind server to port 12345!");
    }

    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readPendingDatagrams);
}

void MainWindow::readPendingDatagrams()
{
    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray data;
        data.resize(udpSocket->pendingDatagramSize());

        QHostAddress senderAddress;
        quint16 senderPort;

        udpSocket->readDatagram(data.data(), data.size(), &senderAddress, &senderPort);

        QString msg = QString::fromUtf8(data);
        QStringList parts = msg.split(",");
        if(parts.isEmpty() || parts[0].isEmpty()) continue;

        QString command = parts[0];
        if(command == "JOIN")
        {
            if(parts.size() < 2) continue;
            QString username = parts[1].trimmed();
            if(username.isEmpty()) continue;

            users[username] = qMakePair(senderAddress, senderPort);

            ui->textEdit->append(username + " joined from " + senderAddress.toString() + ":" + QString::number(senderPort));
            if(ui->textEdit_2) ui->textEdit_2->append(username + " logged online.");
            for(auto it = users.begin(); it != users.end(); ++it) {
                if(it.key() != username) {
                    QString joinMsg = "joined," + username;
                    udpSocket->writeDatagram(joinMsg.toUtf8(), it.value().first, it.value().second);
                }
            }
            for(auto it = users.begin(); it != users.end(); ++it) {
                QString onlineMsg = "joined," + it.key();
                udpSocket->writeDatagram(onlineMsg.toUtf8(), senderAddress, senderPort);
            }
            for(auto it = groups.begin(); it != groups.end(); ++it) {
                QString grpMsg = "grp," + it.key();
                udpSocket->writeDatagram(grpMsg.toUtf8(), senderAddress, senderPort);
            }
        }
        else if(command == "MSG")
        {
            if(parts.size() < 4) continue;
            QString sender   = parts[1];
            QString receiver = parts[2];
            QString text     = parts[3];

            if(ui->textEdit_2) ui->textEdit_2->append(sender + " -> " + receiver + " (Unicast Routing Request)");

            if(users.contains(receiver)) {
                auto target = users[receiver];
                QString forward = sender + ": " + text;
                udpSocket->writeDatagram(forward.toUtf8(), target.first, target.second);
            }
        }
        else if(command == "CREATE")
        {
            if(parts.size() < 2) continue;
            QString name = parts[1].trimmed();
            if(name.isEmpty()) continue;
            QHostAddress multicastAddr(QString("239.1.1.%1").arg(groups.size() + 1));
            quint16 multicastport = 45454;

            groups[name] = qMakePair(multicastAddr, multicastport);

            if(ui->textEdit_2) ui->textEdit_2->append("Group Created: " + name);
            if(ui->textEdit_3) ui->textEdit_3->append("Group Created: " + name);
            QString ginf = "ginf," + name + "," + multicastAddr.toString() + "," + QString::number(multicastport);
            udpSocket->writeDatagram(ginf.toUtf8(), senderAddress, senderPort);

            for(auto it = users.begin(); it != users.end(); ++it) {
                QString x = "grp," + name;
                udpSocket->writeDatagram(x.toUtf8(), it.value().first, it.value().second);
            }
        }
        else if(command == "joinedgrp")
        {
            if(parts.size() < 2) continue;
            QString gn = parts[1];
            if(groups.contains(gn)) {
                auto grp = groups[gn];
                QString msg = "ginf," + gn + "," + grp.first.toString() + "," + QString::number(grp.second);
                udpSocket->writeDatagram(msg.toUtf8(), senderAddress, senderPort);
            }
        }
        else if(command == "GRPMSG")
        {
            if(parts.size() < 4) continue;
            QString groupName = parts[1];
            QString sender    = parts[2];
            QString text      = parts[3];

            if(groups.contains(groupName)) {
                QString multicastMsg = "[" + groupName + "] " + sender + ": " + text;
                QHostAddress multicastAddr = groups[groupName].first;
                quint16 multicastPort = groups[groupName].second;

                // Clean interface lookup
                QNetworkInterface multicastIface;
                for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
                    if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
                        iface.flags().testFlag(QNetworkInterface::CanMulticast) &&
                        !iface.addressEntries().isEmpty()) {

#ifdef Q_OS_WIN
                        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack)) continue;
#endif

                        multicastIface = iface;
                        break;
                    }
                }

                // Only set it if we actually found a valid, active hardware interface
                if (multicastIface.isValid()) {
                    udpSocket->setMulticastInterface(multicastIface);
                } else {
                    // If no interface matches, clear it so the OS uses its default routing table
                    udpSocket->setMulticastInterface(QNetworkInterface());
                }

                qint64 n = udpSocket->writeDatagram(multicastMsg.toUtf8(), multicastAddr, multicastPort);
                qDebug() << "bytes sent =" << n;

            }
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
