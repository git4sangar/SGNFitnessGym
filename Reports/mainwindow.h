#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QTimer>
#include <QMainWindow>

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
    void clearStatus();
    void onEpochResponse(QNetworkReply *pReply);
    void onQueryResponse(QNetworkReply *pReply);
    void on_btnQuery_clicked();
    void on_btnRenewals_clicked();

    void on_btnAbsentees_clicked();

    void on_btnBDay_clicked();

    void on_tblWdgtReport_cellClicked(int row, int column);

private:
    void    queryDB(QString pSQL);
    void    updateStatus(QString pStatus, int delay = 5000);
    QString getPhoto(int32_t pMembershipNo);
    void    setPhoto(QString pPhoto);
    int32_t getMembershipNoFromFileName(QString pFileName);
    void    loadTableWidget(const QVector<User::Ptr>& pUsers);

    Ui::MainWindow          *ui;
    QNetworkAccessManager   *mpHttpMgr;
    QTimer                  *mpStatusTimer;
    QString                 mImagesDir;
    time_t                  mCurEpoch;
    QStringList             mReportOptions;
};
#endif // MAINWINDOW_H
