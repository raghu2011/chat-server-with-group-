#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QHostAddress>
#include <QSet>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void joinChat();
    void sendMessage();
    void creategroup();
    void joinedgroups();
    void readPendingDatagrams();
    void readMulticastMessages();

private:
    Ui::MainWindow *ui;
    QUdpSocket *udpSocket;
    QUdpSocket *multicastSocket;

    // Persistent username variable to fix the empty text box validation issue
    QString currentUsername;

};

#endif // MAINWINDOW_H
