#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QTimer>

#include "user.h"

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
    void on_lnEdtMemberNo_returnPressed();
    void onUserResponse(QNetworkReply *pReply);
    void onAttendanceMarked(QNetworkReply *pReply);
    void clearStatus();

private:
    void    updateStatus(QString pStatus, int delay = 5000);
    QString getPhoto(int32_t pMembershipNo);
    void    setPhoto(QString pPhoto);
    int32_t getMembershipNoFromFileName(QString pFileName);

    Ui::MainWindow          *ui;
    QNetworkAccessManager   *mpHttpMgr;
    QTimer                  *mpStatusTimer;
    User::Ptr               mpUser;
    QString                 mImagesDir;
};
#endif // MAINWINDOW_H
