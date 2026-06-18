#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress>
#include <QDebug>
#include <QNetworkInterface>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(0);

    multicastSocket = new QUdpSocket(this);
    qDebug() << multicastSocket->bind(
        QHostAddress::AnyIPv4,
        45454,
        QUdpSocket::ShareAddress |
            QUdpSocket::ReuseAddressHint);
    multicastSocket->setSocketOption(
        QAbstractSocket::MulticastLoopbackOption, 1);
    qDebug() << multicastSocket->errorString();

    // Connect slots
    bool x=connect(multicastSocket, &QUdpSocket::readyRead, this, &MainWindow::readMulticastMessages);
    qDebug() << "connected readyRead"<<x;
    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::readPendingDatagrams);

    connect(ui->buttonjoin, &QPushButton::clicked, this, &MainWindow::joinChat);
    connect(ui->buttonsend, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(ui->crtbutton, &QPushButton::clicked, this, &MainWindow::creategroup);
    connect(ui->joinbtn, &QPushButton::clicked, this, &MainWindow::joinedgroups);

    connect(ui->listWidget, &QListWidget::itemClicked, this, [this]() {
        ui->listWidget_3->clearSelection();
        ui->listWidget_3->setCurrentItem(nullptr);
    });
    connect(ui->listWidget_3, &QListWidget::itemClicked, this, [this]() {
        ui->listWidget->clearSelection();
        ui->listWidget->setCurrentItem(nullptr);
    });
}

void MainWindow::joinChat()
{
    currentUsername = ui->lineeditjoin->text().trimmed();

    if(currentUsername.isEmpty()) {
        return;
    }

    QString msg = "JOIN," + currentUsername;
    udpSocket->writeDatagram(msg.toUtf8(), QHostAddress::LocalHost, 12345);
    ui->lineeditjoin->setEnabled(false);
    ui->buttonjoin->setEnabled(false);
}

void MainWindow::sendMessage()
{
    QString text = ui->lineedittext->text().trimmed();

    if (text.isEmpty() || currentUsername.isEmpty()) {
        qDebug()<<"hii1";
        return;
    }
    if (ui->listWidget_3->currentItem() != nullptr) {
        qDebug()<<"hii2";
        QString grp = ui->listWidget_3->currentItem()->text();
        QString msg = "GRPMSG," + grp + "," + currentUsername + "," + text;

        udpSocket->writeDatagram(msg.toUtf8(), QHostAddress::LocalHost, 12345);
        ui->lineedittext->clear();
    }
    else if (ui->listWidget->currentItem() != nullptr) {
        qDebug()<<"hii3";
        QString receiver = ui->listWidget->currentItem()->text();
        QString msg = "MSG," + currentUsername + "," + receiver + "," + text;

        udpSocket->writeDatagram(msg.toUtf8(), QHostAddress::LocalHost, 12345);

        ui->textEdit->append("Me -> " + receiver + ": " + text);
        ui->lineedittext->clear();
    }else{
        qDebug()<<"hii";
    }
}

void MainWindow::readPendingDatagrams()
{
    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray data;
        data.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(data.data(), data.size());

        QString msg = QString::fromUtf8(data);
        QStringList p = msg.split(",");
        if(p.isEmpty() || p[0].isEmpty()) continue;

        QString header = p[0];

        if(header == "joined") {
            if(ui->listWidget->findItems(p[1], Qt::MatchExactly).isEmpty()) {
                ui->listWidget->addItem(p[1]);
            }
        }
        else if(header == "grp") {
            if(ui->listWidget_2->findItems(p[1], Qt::MatchExactly).isEmpty()) {
                ui->listWidget_2->addItem(p[1]);
            }
        }
        else if(header == "ginf") {
            if(p.size() < 4) continue;
            QString name = p[1];
            QString mip  = p[2];
            quint16 mport = p[3].toUShort();


            QNetworkInterface iface;
            for (const QNetworkInterface &netInterface : QNetworkInterface::allInterfaces()) {
                if (netInterface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
                    iface = netInterface;
                    break;
                }
            }

            bool ok = multicastSocket->joinMulticastGroup(QHostAddress(mip), iface);

            qDebug() << "join =" << ok;
            ui->listWidget_3->addItem(name);
            qDebug() << "Join multicast:" << mip << ok;
        }
        else {
            ui->textEdit->append(msg);
        }
    }
}

void MainWindow::creategroup()
{
    QString x = ui->grpcrt->text().trimmed();
    if(x.isEmpty()) return;

    QString msg = "CREATE," + x;
    udpSocket->writeDatagram(msg.toUtf8(), QHostAddress::LocalHost, 12345);
    ui->grpcrt->clear();
}

void MainWindow::joinedgroups()
{
    if(!ui->listWidget_2->currentItem()) return;

    QString y = ui->listWidget_2->currentItem()->text();
    QString msg = "joinedgrp," + y;
    udpSocket->writeDatagram(msg.toUtf8(), QHostAddress::LocalHost, 12345);
}

void MainWindow::readMulticastMessages()
{
    while(multicastSocket->hasPendingDatagrams())
    {
        qDebug()<<"raghu";
        QByteArray data;
        data.resize(multicastSocket->pendingDatagramSize());
        multicastSocket->readDatagram(data.data(), data.size());

        ui->textEdit->append(QString::fromUtf8(data));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
