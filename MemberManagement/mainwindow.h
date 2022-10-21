#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QTimer>

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
    void onNewMemberResponse(QNetworkReply *pReply);
    void onNewUserResponse(QNetworkReply *pReply);
    void onUpdateUserResponse(QNetworkReply *pReply);
    void onGetDetailsResponse(QNetworkReply *pReply);
    void clearStatus();

    void on_btnNewJoiner_clicked();
    void on_btnGetDetails_clicked();
    void on_btnClear_clicked();

private:
    void onGetDetailsClicked();
    void onSubmitClicked();
    void onUpdateClicked();
    void onSubmitOrUpdate(bool isSubmit);
    void updateStatus(QString pStatus, int delay = 5000);

    QString getPhoto(int32_t pMembershipNo);
    void    setPhoto(QString pPhoto);
    int32_t getMembershipNoFromFileName(QString pFileName);

    static const QString    BTN_GET_DETAILS;
    static const QString    BTN_SUBMIT;
    static const QString    BTN_UPDATE;

    Ui::MainWindow          *ui;
    QNetworkAccessManager   *mpHttpMgr;
    QTimer                  *mpStatusTimer;
    int32_t                 mCurrentId;
    QString                 mImagesDir;
};
#endif // MAINWINDOW_H
