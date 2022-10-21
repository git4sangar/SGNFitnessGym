#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QTimer>

#include "fees.h"

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
    void clearStatus();
    void onAddFeeResponse(QNetworkReply *pReply);
    void feeDetailsResponse(QNetworkReply *pReply);
    void onMemberNoResponse(QNetworkReply *pReply);
    void onSpouseNoResponse(QNetworkReply *pReply);
    void on_btnClear_clicked();
    void on_btnSubmit_clicked();
    void on_lnEdtMembershipNo_returnPressed();
    void on_lnEdtSpouseNo_returnPressed();

    void on_lnEdtSpouseNo_textChanged(const QString &arg1);

    void on_lnEdtMembershipNo_textChanged(const QString &arg1);

private:
    void    getUserFromDB(QString pMembershipNo, bool isMember);
    QString getColumnStringValue(QString jsonUser, QString pField, bool showPhoto = false);
    void    updateStatus(QString pStatus, int delay = 5000);
    QString getPhoto(int32_t pMembershipNo);
    void    setPhoto(QString pPhoto);
    void    fillUpUI(Fees::Ptr pFees);
    int32_t getMembershipNoFromFileName(QString pFileName);

    Ui::MainWindow          *ui;
    QNetworkAccessManager   *mpHttpMgr;
    QTimer                  *mpStatusTimer;
    QString                 mImagesDir;
};
#endif // MAINWINDOW_H
