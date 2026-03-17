#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTableView;
class QLabel;
class QLineEdit;
class QSplitter;
class QSystemTrayIcon;
class SessionManager;
class TorrentModel;
class TorrentFilter;
class QStackedWidget;
class DetailsPanel;
class SpeedGraph;
class BatWidget;
class SplashWidget;
class Updater;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(SessionManager *session, QWidget *parent = nullptr);
    ~MainWindow();

    void addTorrentFromCli(const QString &filePath);
    void addMagnetFromCli(const QString &uri);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void openTorrent();
    void openMagnet();
    void createTorrent();
    void importQBittorrent();
    void removeSelected();
    void removeSelectedWithFiles();
    void pauseSelected();
    void resumeSelected();
    void pauseAll();
    void resumeAll();
    void updateStatusBar();
    void onSelectionChanged();
    void onTorrentFinished(const QString &name);
    void onTorrentError(const QString &message);
    void trayActivated();
    void openSettings();
    void showWelcome();
    void showAbout();
    void retranslateUi();
    void filterByState(const QString &state);
    void showContextMenu(const QPoint &pos);
    void checkForUpdate(bool silent = true);

private:
    void applyTheme();
    void setupMenuBar();
    void setupToolBar();
    void setupCentralWidget();
    void setupStatusBar();
    void setupTrayIcon();
    void saveSettings();
    void loadSettings();
    QList<int> selectedRows() const;
    void addTorrentFile(const QString &filePath);

    SessionManager *m_session;
    TorrentModel *m_model;
    TorrentFilter *m_proxyModel;
    QTableView *m_tableView;
    QLineEdit *m_searchEdit;
    DetailsPanel *m_detailsPanel;
    SpeedGraph *m_speedGraph;
    BatWidget *m_batWidget;
    QStackedWidget *m_topStack;
    SplashWidget *m_splash = nullptr;
    QLabel *m_statusLabel;
    QSystemTrayIcon *m_trayIcon;
    QString m_lastSavePath;
    Updater *m_updater;
    bool m_startMinimized = false;
    bool m_useDefaultPath = false;
};

#endif
